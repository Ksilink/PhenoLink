
#include <QtCore>
#ifdef WIN32
#include <QApplication>
#endif

#include <QtNetwork>
#include <QDebug>

#include <fstream>
#include <iostream>
#include <limits>

#include "Core/pluginmanager.h"

#include <Core/checkoutprocessplugininterface.h>

#include <Core/checkoutprocess.h>

#include "Core/config.h"

#ifdef WIN32
#include <windows.h>
#include <wincon.h>
#endif

#include <QtConcurrent>

#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"

using namespace qhttp::server;

#include "checkouqueuserver.h"
#include <Core/networkprocesshandler.h>


std::ofstream outfile("c:/temp/CheckoutServer_log.txt");

void myMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray date = QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz").toLocal8Bit();
    switch (type) {
    case QtInfoMsg:
        outfile << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtDebugMsg:
        outfile << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        outfile  << date.constData() << " Warning  : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr  << date.constData() << " Warning  : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        outfile   << date.constData()  << " Critical : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr   << date.constData()  << " Critical : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        outfile  << date.constData() << " Fatal    : "<< localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr  << date.constData() << " Fatal    : "<< localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        abort();

    }

    outfile.flush();
}

#if WIN32

void show_console() {
    AllocConsole();

    FILE *newstdin = nullptr;
    FILE *newstdout = nullptr;
    FILE *newstderr = nullptr;

    freopen_s(&newstdin, "conin$", "r", stdin);
    freopen_s(&newstdout, "conout$", "w", stdout);
    freopen_s(&newstderr, "conout$", "w", stderr);
}


#endif

void startup_execute(QString file)
{
    QFile f(file);

    QString ex=QString(f.readLine());
    while (!ex.isEmpty())
    {
        QStringList line = ex.split(" ");
        QString prog = line.front();
        line.pop_front();
        QProcess::execute(prog, line);


        ex = QString(f.readLine());
    }

}

int main(int ac, char** av)
{
#ifdef WIN32
    QApplication app(ac, av);
#else
    QCoreApplication app(ac, av);
#endif

    app.setApplicationName("Checkout");
    app.setApplicationVersion(CHECKOUT_VERSION);
    app.setOrganizationDomain("WD");
    app.setOrganizationName("WD");



    QSettings sets;
    uint port = sets.value("ServerPort", 13378).toUInt();
    if (!sets.contains("ServerPort")) sets.setValue("ServerPort", 13378);


    QStringList data;
    for (int i = 1; i < ac; ++i) { data << av[i];  }
    if (data.contains("-p"))
    {
        int idx = data.indexOf("-p")+1;
        if (data.size() > idx) port = data.at(idx).toInt();
        qDebug() << "Changing server port to :" << port;
    }
    else
        qDebug() << "Server port :" << port;


    if (data.contains("-Crashed"))
    { // recovering from crashed session, how to tell the user the queue crashed???

    }


#if WIN32

    if (data.contains("-d"))
        show_console();
#endif

    Server server;
    server.setPort(port);

    return server.start(port);
}



#ifndef WIN32

void Server::setDriveMap(QString map)
{
    CheckoutProcess::handler().setDriveMap(map);
}

#endif


QString stringIP(quint32 ip)
{
    return QString("%4.%3.%2.%1").arg(ip & 0xFF)
            .arg((ip >> 8) & 0xFF)
            .arg((ip >> 16) & 0xFF)
            .arg((ip >> 24) & 0xFF);
}

Server::Server(): QHttpServer(), client(nullptr)
{
#ifdef WIN32

    _control = new Control(this);

#endif
}

int Server::start(quint16 port)
{
    connect(this, &QHttpServer::newConnection, [this](QHttpConnection*){
        Q_UNUSED(this);
        //        qDebug() << "a new connection has occured!";
    });



    bool isListening = listen(
                QString::number(port),
                [this]( qhttp::server::QHttpRequest* req,  qhttp::server::QHttpResponse* res){
            req->collectData();
            req->onEnd([this, req, res](){
        //        qDebug() << "Processing query";
        process(req, res);
    });
});

    if ( !isListening ) {
        qDebug("can not listen on %d!\n", port);
        return -1;
    }

    qDebug() << "Starting monitor thread";
    QtConcurrent::run(this, &Server::WorkerMonitor);
    qDebug() << "Done";

    return qApp->exec();
}

void Server::setHttpResponse(QJsonObject& ob,  qhttp::server::QHttpResponse* res, bool binary)
{
    QByteArray body =  binary ? QCborValue::fromJsonValue(ob).toCbor() :
                                QJsonDocument(ob).toJson();
    //    res->addHeader("Connection", "keep-alive");

    if (binary)
        res->addHeader("Content-Type", "application/cbor");
    else
        res->addHeader("Content-Type", "application/json");

    res->addHeaderValue("Content-Length", body.length());
    res->setStatusCode(qhttp::ESTATUS_OK);
    res->end(body);
}


void Server::HTMLstatus(qhttp::server::QHttpResponse* res, QString mesg)
{

    QString message;

    QString body;

    // jobs : priority to server  / Process name [call params]

    body += QString("<h2>Connected Users : %1 / Number of Pending Jobs %2</h2>").arg(nbUsers()).arg(njobs());
    if (!mesg.isEmpty())
        body += QString("<p style='color:red;'>Info: %1</p>").arg(mesg);

    QMap<QString, int > servers;

    for (auto& srv: workers)
        if (!rmWorkers.contains(qMakePair(srv.first, srv.second)))
        {

            auto id = QString("%1:%2").arg(srv.first).arg(srv.second);
            servers[id] ++;
        }

    body += "<h3>Cores Listing</h3>";
    //for (auto it = runni)
    for (auto it = workers_status.begin(), e = workers_status.end(); it != e; ++it)
    {
        auto t = it.key().split(":");
        if (rmWorkers.contains(qMakePair(t.front(), t.back().toInt()))) continue;
        QStringList  aff;
        for (auto af = project_affinity.begin(), e = project_affinity.end(); af != e; ++af)
            if (af.value() == it.key()) aff << af.key();

        body += QString("Server %1 => %2 Cores %5&nbsp;<a href='/rm?host=%3&port=%4'>remove server</a><br>").arg(it.key()).arg(it.value()).arg(t.front(), t.back()).arg(QString("(%1)").arg(aff.join(",")));
    }

    body += "<h3>Cores Availability</h3>";

    for (auto it = servers.begin(), e = servers.end(); it != e; ++it)
    {
        body += QString("Server %1 => %2 Cores<br>").arg(it.key()).arg(it.value());
    }

    body += "<h3>Awaiting Jobs</h3>";

    body += pendingTasks().join("<br>") + "<br>";

    body += "<h3>Running Jobs</h3>";

    QMap<QString, int> runs;
    for (auto task = running.begin(); task != running.end(); ++task)
        runs[task.key().split("!").first()]++;

    for (auto it = runs.begin(), e = runs.end(); it != e; ++it)
        body += QString("%1 => %2 <a href='/Cancel/?proc=%1'>Cancel</a>&nbsp;<a href='/Restart'>Force Restart</a><br>").arg(it.key()).arg(it.value());

    message = QString("<html><title>Checkout Queue Status %2</title><body>%1</body></html>").arg(body).arg(CHECKOUT_VERSION);


    res->setStatusCode(qhttp::ESTATUS_OK);
    res->addHeaderValue("content-length", message.size());
    res->end(message.toUtf8());
}

QMutex workers_lock;

unsigned int Server::njobs()
{
    unsigned int count = 0;
    QMutexLocker lock(&workers_lock);

    for (auto& srv: jobs)
    {
        for (auto& q : srv)
        {
            count += q.size();
        }


    }

    return count;
}


unsigned int Server::nbUsers()
{
    QSet<QString> names;

    QMutexLocker lock(&workers_lock);

    for (auto& srv: jobs)
    {
        for (auto& q: srv)
            for (auto& proc: q)
            {
                names.insert(QString("%1%2").arg(proc["Username"].toString())
                        .arg(proc["Computer"].toString()));
            }

    }
    return names.size();
}


QStringList Server::pendingTasks()
{
    QMutexLocker lock(&workers_lock);

    QMap<QString, int> names;
    for (auto& srv: jobs)
    {
        for (auto& q: srv)
            for (auto& proc: q)
            {
                names[QString("%1@%2 : %3 (%4)")
                        .arg(proc["Username"].toString(),
                        proc["Computer"].toString(),
                        proc["Path"].toString(),
                        proc["WorkID"].toString())
                        ]++;
            }

    }

    QStringList res;
    for (auto it = names.begin(), e = names.end(); it != e; ++it)
    {
        res << QString("%1 => %2").arg(it.key()).arg(it.value());
    }

    return res;
}




QQueue<QJsonObject>& Server::getHighestPriorityJob(QString server)
{
    // QMutexLocker locker(&workers_lock);

    if (jobs.contains(server)) // does this server have some affinity with the current process ?
    { // if yes we dequeue from this one
        int key = jobs[server].lastKey();

        if (jobs[server].contains(key) && jobs[server][key].isEmpty())
        {
            jobs[server].remove(key);
            return getHighestPriorityJob(server);
        }

        return jobs[server][key];
    }
    // retreive first available object

    // Find highest priority job:
    int maxpriority = 0;
    QString maxserv;
    for (auto serv = jobs.begin(), e = jobs.end(); serv != e; ++serv)
    {
        if (maxpriority < serv.value().lastKey())
        {
            maxpriority = serv.value().lastKey();
            maxserv = serv.key();
        }
    }

    if (jobs[maxserv].contains(maxpriority) && jobs[maxserv][maxpriority].isEmpty())
    {
        jobs[maxserv].remove(maxpriority);
        return getHighestPriorityJob(server);
    }

    return jobs[maxserv][maxpriority];
}

void sendByteArray(qhttp::client::QHttpClient& iclient, QUrl& url, QByteArray ob)
{
    iclient.request(qhttp::EHTTP_POST, url,
                    [ob]( qhttp::client::QHttpRequest* req){
        auto body = ob;
        req->addHeader("Content-Type", "application/cbor");
        req->addHeaderValue("content-length", body.length());
        req->end(body);
    },

    []( qhttp::client::QHttpResponse* res) {
        res->collectData();
    });
}
#include <Core/networkprocesshandler.h>
void Server::WorkerMonitor()
{

    QMap<QString, CheckoutHttpClient* > clients;//


    while (true) // never ending run
    {
        if (njobs() && ! workers.isEmpty()) // If we have some jobs left and some workers available
        {
            workers_lock.lock();
            auto next_worker = pickWorker();
            if (!next_worker.first.isEmpty())
            {
                QQueue<QJsonObject>& queue = getHighestPriorityJob(next_worker.first);

                // Hey hey look what we have here: the job to be run by next_worker let's call the start func then :
                // start it :)
                if (queue.size())
                {

                    QJsonObject pr = queue.dequeue();
                    QString srv = QString("%1:%2").arg(next_worker.first).arg(next_worker.second); // Set proxy name
                    if (!proc_params[pr["Path"].toString()].contains(srv))
                    {
                        next_worker = pickWorker(pr["Path"].toString());

                    }

                    CheckoutHttpClient* sr = nullptr;
                    if (clients[srv])
                        sr = clients[srv];
                    else
                    {
                        clients[srv] = new CheckoutHttpClient(next_worker.first, next_worker.second);
                        sr = clients[srv];
                    }
                    //                    sr->setCollapseMode(false);
                    QString taskid = QString("%1@%2#%3#%4!%5#%6:%7")
                            .arg(pr["Username"].toString(), pr["Computer"].toString(),
                            pr["Path"].toString(), pr["WorkID"].toString(),
                            pr["XP"].toString(), pr["Pos"].toString(), pr["Process_hash"].toString());

                    pr["TaskID"] = taskid;

                    //  qDebug()  << "Taskid" << taskid << pr;
                    QJsonArray ar; ar.append(pr);

                    sr->send(QString("/Start/%1").arg(pr["Path"].toString()), QString(""), ar);

                    workers.removeOne(next_worker);
                    workers_status[QString("%1:%2").arg(next_worker.first).arg(next_worker.second)]--;
                    pr["SendTime"] = QDateTime::currentDateTime().toSecsSinceEpoch();
                    running[taskid] = pr;
                }

            }
            workers_lock.unlock();
            QThread::msleep(2);
        }
        else
        {
            QThread::msleep(300); // Wait for 300ms at least
        }

        qApp->processEvents();
    }
}

QPair<QString, int> Server::pickWorker(QString )
{
    //    if (process.isEmpty())
    //    {

    static QString lastsrv;

    //    QMutexLocker locker(&workers_lock);
    if (lastsrv.isEmpty())

    {

        auto next_worker = workers.back();
        int p = 1;
        while (rmWorkers.contains(next_worker) && p < workers.size())  next_worker = workers.at(p++);
        if (p==workers.size()) return qMakePair(QString(), 0);
        lastsrv=next_worker.first;
        return next_worker;
    }
    for(auto& nxt: workers)
        if (lastsrv != nxt.first && !rmWorkers.contains(nxt) )
        {
            lastsrv = nxt.first;
            return nxt;
        }

    auto next = workers.back();
    lastsrv = next.first;
    return next;
    //    }
    //    else
    //    {




    //    }

}

#undef max
#undef min
#undef signals

#include <arrow/api.h>
#include <arrow/filesystem/filesystem.h>
#include <arrow/util/iterator.h>
#include <arrow/ipc/writer.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/api.h>


namespace fs = arrow::fs;

template<class K,class V>
struct QMapWrapper {
    const QMap<K,V> map;
    QMapWrapper(const QMap<K,V>& map) : map(map) {}
    auto begin() { return map.keyValueBegin(); }
    auto end()   { return map.keyValueEnd();   }
};
template<class K,class V>
QMapWrapper<K,V> wrapQMap(const QMap<K,V>& map) {
    return QMapWrapper<K,V>(map);
}
#define ArrowGet(var, id, call, msg) \
    auto id = call; if (!id.ok()) { qDebug() << msg; return ; } auto var = id.ValueOrDie();


template <class T>
struct mytrait
{
    typedef typename T::value_type   value_type;
};

template<>
struct mytrait<class arrow::StringArray>
{
    typedef std::string value_type;
};

template <class T>
typename mytrait<T>::value_type  _get(std::shared_ptr<T> ar, int s)
{
    return ar->Value(s);
}


template<>
mytrait<arrow::StringArray>::value_type _get(std::shared_ptr<arrow::StringArray> ar, int s)
{
    return ar->GetString(s);
}


template <class Bldr, class ColType>
std::shared_ptr<arrow::Array> concat(const QList<std::shared_ptr<arrow::Array> >& list)
{

    std::shared_ptr<arrow::Array> res;

    Bldr bldr;

    for (auto& ar: list)
    {
        auto c = std::static_pointer_cast<ColType>(ar);
        for (int s = 0; s < c->length(); ++s)
        {
            if (c->IsValid(s))
                bldr.Append(_get(c, s));
            else
                bldr.AppendNull();
        }

    }


    bldr.Finish(&res);

    return res;

}


double AggregateSum(QList<float>& f)
{

    if (f.size() == 1) return f.at(0);

    double r = 0;
    foreach(double ff, f)
        r += ff;

    return r;
}
double AggregateMean(QList<float>& f)
{
    if (f.size() == 1) return f.at(0);
    double r = 0;
    foreach(double ff, f)
        r += ff;
    r /= (double)f.size();
    return r;
}




double AggregateMedian(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    std::sort(f.begin(), f.end());
    return f.at(f.size() / 2);
}

double AggregateMin(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    double r = std::numeric_limits<float>::max();
    foreach(double ff, f)
        if (ff < r) r = ff;
    return r;
}

double AggregateMax(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    double r = std::numeric_limits<float>::min();
    foreach(double ff, f)
        if (ff > r) r = ff;
    return r;
}


float Aggregate(QList<float>& f, QString& ag)
{
    if (ag == "Sum") return AggregateSum(f);
    if (ag == "Mean") return AggregateMean(f);
    if (ag == "Median") return AggregateMedian(f);
    if (ag == "Min") return AggregateMin(f);
    if (ag == "Max") return AggregateMax(f);

    qDebug() << "Aggregation error!" << ag << "Not found";
    return -1.;
}


void fuseArrow(QString bp, QStringList files, QString out)
{

    qDebug() << "Fusing" << files << "to" << out;

    QMap<std::string, QList<  std::shared_ptr<arrow::Array> > > datas;
    QMap<std::string, std::shared_ptr<arrow::Field> >     fields;
    QMap<std::string, QMap<std::string, QList<float> > >  agg;

    int counts = 0;
    for (auto file: files)
    {
        QList<std::string> lists;

        std::string uri = (bp+"/"+file).toStdString(), root_path;
        ArrowGet(fs, r0, fs::FileSystemFromUriOrPath(uri, &root_path), "Arrow File not loading" << file);

        ArrowGet(input, r1, fs->OpenInputFile(uri), "Error openning arrow file" << bp+file);
        ArrowGet(reader, r2, arrow::ipc::RecordBatchFileReader::Open(input), "Error Reading records");

        auto schema = reader->schema();

        ArrowGet(rowC, r3, reader->CountRows(), "Error reading row count");


        for (int record = 0; record < reader->num_record_batches(); ++record)
        {
            ArrowGet(rb, r4, reader->ReadRecordBatch(record), "Error Get Record");

            auto well = std::static_pointer_cast<arrow::StringArray>(rb->GetColumnByName("Well"));


            for (auto f : schema->fields())

                {
                    if (!fields.contains(f->name()))
                        fields[f->name()] = f;
                    if (counts != 0 && !datas.contains(f->name()))
                    { // Assure that we add non existing cols with empty data if in case
                        std::shared_ptr<arrow::Array> ar;

                        arrow::NumericBuilder<arrow::FloatType> bldr;
                        bldr.AppendNulls(counts);
                        bldr.Finish(&ar);
                        datas[f->name()].append(ar);
                    }
                    datas[f->name()].append(rb->GetColumnByName(f->name()));

                    auto array = std::static_pointer_cast<arrow::FloatArray>(rb->GetColumnByName(f->name()));
                    if (array &&   (f->name() != "Well"))
                        for (int s = 0; s < array->length(); ++s)
                            agg[f->name()][well->GetString(s)].append(array->Value(s));

                    lists << f->name();
                }
        }

        for (auto& f: datas.keys())
        { // Make sure we add data to empty columns
            if (!lists.contains(f))
                datas[f].append(std::shared_ptr<arrow::NullArray>(new arrow::NullArray(rowC)));
        }

        counts += rowC;
    }



    std::vector<std::shared_ptr<arrow::Field> > ff;
    for (auto& item: datas.keys())
        ff.push_back(fields[item]);


    std::vector<std::shared_ptr<arrow::Array> > dat(ff.size());

    int p = 0;
    for (auto wd: wrapQMap(datas))
    {
        if (std::static_pointer_cast<arrow::FloatArray>(wd.second.first()))
            dat[p] = concat<arrow::NumericBuilder<arrow::FloatType> , arrow::FloatArray >(wd.second);
        if (std::static_pointer_cast<arrow::Int16Array>(wd.second.first()))
            dat[p] = concat<arrow::NumericBuilder<arrow::Int16Type> , arrow::Int16Array >(wd.second);
        if (std::static_pointer_cast<arrow::StringArray>(wd.second.first()))
            dat[p] = concat<arrow::StringBuilder, arrow::StringArray > (wd.second);
    }


    auto schema =
            arrow::schema(ff);
    auto table = arrow::Table::Make(schema, dat);


    std::string uri = out.toStdString();
    std::string root_path;

    auto r0 = fs::FileSystemFromUriOrPath(uri, &root_path);
    if (!r0.ok())
    {
        qDebug() << "Arrow Error not able to load" << out;
        return;
    }

    auto fs = r0.ValueOrDie();

    auto r1 = fs->OpenOutputStream(uri);

    if (!r1.ok())
    {
        qDebug() << "Arrow Error to Open Stream" << out;
        return;
    }
    auto output = r1.ValueOrDie();
    arrow::ipc::IpcWriteOptions options = arrow::ipc::IpcWriteOptions::Defaults();
    //        options.codec = arrow::util::Codec::Create(arrow::Compression::LZ4).ValueOrDie(); //std::make_shared<arrow::util::Codec>(codec);

    auto r2 = arrow::ipc::MakeFileWriter(output.get(), table->schema(), options);

    if (!r2.ok())
    {
        qDebug() << "Arrow Unable to make file writer";
        return;
    }

    auto writer = r2.ValueOrDie();

    writer->WriteTable(*table.get());
    writer->Close();

    // Let's handle agg
    {
        std::vector<std::shared_ptr<arrow::Field> > ff;
        ff.push_back(fields["Well"]);
        for (auto& item: agg.keys())
            ff.push_back(fields[item]);

        std::vector<std::shared_ptr<arrow::Array> > dat(1+agg.size());

        arrow::StringBuilder wells;
        std::vector<arrow::NumericBuilder<arrow::FloatType> > data(agg.size());

        QList<std::string> ws = agg.first().keys(), fie = agg.keys();

        for (auto& w: ws)
        {
            wells.Append(w);
            int f = 0;
            for (auto& name: fie)
            {
                auto method = fields[name]->metadata()->Contains("Aggregation") ? QString::fromStdString(fields[name]->metadata()->Get("Aggregation").ValueOrDie()) : QString();
                QList<float>& dat = agg[name][w];
                data[f].Append(Aggregate(dat, method));
                f++;
            }
        }

        wells.Finish(&dat[0]);
        for (int i = 0; i < agg.size(); ++i)
            data[i].Finish(&dat[i+1]);

        auto schema =
                arrow::schema(ff);
        auto table = arrow::Table::Make(schema, dat);

        QStringList repath = out.split("/");
        repath.last() = QString("ag%1").arg(repath.last());

        qDebug() << "Aggregating to " << (repath.join("/"));
        std::string uri = (repath.join("/")).toStdString();
        std::string root_path;

        auto r0 = fs::FileSystemFromUriOrPath(uri, &root_path);
        if (!r0.ok())
        {
            qDebug() << "Arrow Error not able to load" << out;
            return;
        }

        auto fs = r0.ValueOrDie();

        auto r1 = fs->OpenOutputStream(uri);

        if (!r1.ok())
        {
            qDebug() << "Arrow Error to Open Stream" << out;
            return;
        }
        auto output = r1.ValueOrDie();
        arrow::ipc::IpcWriteOptions options = arrow::ipc::IpcWriteOptions::Defaults();
        //        options.codec = arrow::util::Codec::Create(arrow::Compression::LZ4).ValueOrDie(); //std::make_shared<arrow::util::Codec>(codec);

        auto r2 = arrow::ipc::MakeFileWriter(output.get(), table->schema(), options);

        if (!r2.ok())
        {
            qDebug() << "Arrow Unable to make file writer";
            return;
        }

        auto writer = r2.ValueOrDie();

        writer->WriteTable(*table.get());
        writer->Close();

    }


// We are lucky enough to get up to here... let's remove the file

    QDir dir(bp);

    for (auto f: files)
        dir.remove(f);



}


void Server::process( qhttp::server::QHttpRequest* req,  qhttp::server::QHttpResponse* res)
{

    const QByteArray data = req->collectedData();
    QString urlpath = req->url().path(), query = req->url().query();



    CheckoutProcess& procs = CheckoutProcess::handler();

    if (urlpath == "/ListProcesses")
    {
        // If not using cbor outputs the version also
        QStringList prcs = procs.pluginPaths(query.contains("json"));
        int processor_count = (int)std::thread::hardware_concurrency();

        QJsonObject ob;

        ob["CPU"] = processor_count;
        ob["Processes"] = QJsonArray::fromStringList(QStringList(proc_list.begin(), proc_list.end()));
        // Need to keep track of processes list from servers
        setHttpResponse(ob, res, !query.contains("json"));
        return;
    }

    if (urlpath.startsWith("/Process/"))
    {
        QString path = urlpath.mid(9);


        QJsonObject& ob = proc_params[path].first();
        setHttpResponse(ob, res, !query.contains("json"));

    }

    if (urlpath.startsWith("/Images/"))
    {
        QString fp = urlpath.mid(8);
        qDebug() << "Exposing file" << fp;


        QFile of(fp);
        if (of.open(QFile::ReadOnly))
        {
            QByteArray body = of.readAll();

            res->addHeader("Connection", "close");
            res->addHeader("Content-Type", "image/jpeg");

            res->addHeaderValue("Content-Length", body.length());
            res->setStatusCode(qhttp::ESTATUS_OK);
            res->end(body);
        }
        return;
    }


    if (urlpath== "/status.html" || urlpath=="/index.html")
    {
        HTMLstatus(res);
        return;
    }

    qDebug() << qhttp::Stringify::toString(req->method())
             << qPrintable(urlpath)
             << qPrintable(query)
             << data.size();


    if (urlpath.startsWith("/Ready")) // Server is ready /Ready/{port}
    {
        //        qDebug() << proc_params;
        QString serverIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());

        uint16_t port;

        QStringList queries = query.split("&");
        QString workid, cpu;

        bool reset = false;
        bool avail = true;
        bool crashed = false;
        for (auto q : queries)
        {
            if (q.startsWith("affinity"))
            {
                QStringList projects = q.replace("affinity=", "").split(",");
                for (auto p: projects) // Adjust project affinity on first run of servers
                {
                    project_affinity[p] = serverIP;
                }

            }
            if (q.startsWith("cpu="))
            {
                cpu = q.replace("cpu=","");
            }

            if (q.startsWith("port"))
            {
                port = q.replace("port=","").toUInt();
            }
            if (q.startsWith("workid"))
            {
                workid = q.replace("workid=","");
            }
            if (q.startsWith("available="))
                avail = (q == "available=1");
            if (q.startsWith("crashed="))
            {
                crashed = (q=="crashed=true");
                // We need to tell all the users with processes that went to the originating server that it crashed,
                // We need either to cancel the process in that case
            }

            if (q=="reset")
                reset = true;

        }

        qDebug() << reset << avail << cpu << workid;
        QMutexLocker lock(&workers_lock);
        if (reset) // Occurs if servers reconnect
        {
            workers.removeAll(qMakePair(serverIP, port));
            workers_status[QString("%1:%2").arg(serverIP).arg(port)]=0;
        }
        auto ob = QCborValue::fromCbor(data).toJsonValue().toArray();

        if (avail && ob.size() == 0 )
        {
            if (workers_status[QString("%1:%2").arg(serverIP).arg(port)] >= 0)
            {
                if (cpu.isEmpty())
                    workers.enqueue(qMakePair(serverIP, port));
                else
                    for (int i= 0; i< cpu.toInt(); ++i)
                        workers.enqueue(qMakePair(serverIP, port));
            }
        }


        if (!cpu.isEmpty())
            workers_status[QString("%1:%2").arg(serverIP).arg(port)]+=cpu.toInt();
        else if (ob.size() == 0)
            workers_status[QString("%1:%2").arg(serverIP).arg(port)]++;

        if (!workid.isEmpty())
        {
            qDebug() << "Finished " << workid;
            auto t = QDateTime::currentDateTime().toSecsSinceEpoch() - running[workid]["SendTime"].toInt();
            QString ww = workid.split("!")[0];

            run_time[ww] += t;
            run_count[ww] ++;

            running.remove(workid);
        }

        for (int i = 0; i < ob.size(); ++i)
        {
            auto obj = ob[i].toObject();
            if (obj.contains("TaskID"))
            {
                auto t = QDateTime::currentDateTime().toSecsSinceEpoch() - running[obj["TaskID"].toString()]["SendTime"].toInt();
                QString ww = obj["TaskID"].toString().split("!")[0];

                run_time[ww] += t;
                run_count[ww] ++;
                work_count[ww] --;

                //                  qDebug() << "Work ID finished" << obj["TaskID"] << ww << work_count.value(ww);
                if (work_count.value(ww) == 0)
                {
                    qDebug() << "Work ID finished" << ww << "Aggregate & collate data";
                    QSettings set;
                    QString dbP = set.value("databaseDir", "L:").toString();

#ifndef  WIN32
                    if (dbP.contains(":"))
                        dbP = QString("/mnt/shares/") + dbP.replace(":","");
#endif

                    dbP.replace("\\", "/").replace("//", "/");
                    auto agg = running[obj["TaskID"].toString()];

                    QString path = QString("%1/PROJECTS/%2/Checkout_Results/%3").arg(dbP,
                                                                                     agg["Project"].toString(),
                            agg["CommitName"].toString()).replace("\\", "/").replace("//", "/");;

                    QThread::sleep(5); // Let time to sync

                    QDir dir(path);

                    QStringList files = dir.entryList(QStringList() << QString("%4_[0-9]*[0-9][0-9][0-9][0-9].fth").arg(agg["XP"].toString().replace("/", "")), QDir::Files);

//                    if (files.size() == 1)
//                    {
//                        dir.rename(QString("%1/%2").arg(path,files[0]), QString("%1/%2.fth").arg(path,agg["XP"].toString().replace("/","")));
//                    }
//                    else
                    { // Heavy Arrow factoring here
                        fuseArrow(path, files, QString("%1/%2.fth").arg(path,agg["XP"].toString().replace("/","")));
                    }
                }

                running.remove(obj["TaskID"].toString());

                workers.enqueue(qMakePair(serverIP, port));
                workers_status[QString("%1:%2").arg(serverIP).arg(port)]++;
            }
        }

    }

    if (urlpath.startsWith("/Affinity"))
    {
        QStringList projects;
        QString srv, port;
        QStringList queries = query.split("&");

        for (auto& q : queries)
        {
            if (q.startsWith("project="))
                projects = q.replace("project=", "").split(",");

            if (q.startsWith("port="))
                port = q.replace("port=","");

            if (q.startsWith("server="))
                srv = q.replace("server=", "");
        }


        for (auto& project: projects)
            project_affinity[project]=QString("%1:%2").arg(srv, port);

    }


    if (urlpath.startsWith("/setProcessList")) // Server is ready /Ready/{port}
    {
        QString serverIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());
        uint16_t port = req->connection()->tcpSocket()->peerPort();//urlpath.replace("/Ready/", "");

        //        proc_list.append();
        auto ob = QCborValue::fromCbor(data).toArray();
        for (int i = 0; i < ob.size(); ++i)
        {
            auto obj = ob.at(i).toMap().toJsonObject();
            QString pr = obj["Path"].toString();
            proc_list.insert(pr);
            proc_params[pr][QString("%1:%2").arg(serverIP).arg(port)] = obj;
        }

        //        qDebug() << proc_params;

    }

    if (urlpath.startsWith("/Proxy"))
    {
        QStringList queries = query.split("&");
        QString srv, port;
        srv = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());
        bool self = true;

        for (auto q : queries)
        {
            if (q.startsWith("port="))
                port = q.replace("port=", "");
            if (q.startsWith("host="))
            {
                srv = q.replace("host=", "");
                self = false;
            }
        }

        if (client)
        {
            qDebug() << "Reseting proxy";
            delete client;
        }

        rmWorkers.remove(qMakePair(srv, port.toInt()));

        client = new CheckoutHttpClient(srv, port.toUInt());
        client->send("/Proxy", QString("port=%1").arg(dport), QJsonArray());
    }

    if (urlpath.startsWith("/rm"))
    {
        QStringList queries = query.split("&");
        QString srv, port;

        for (auto& q : queries)
        {
            if (q.startsWith("port="))
                port = q.replace("port=", "");
            if (q.startsWith("host="))
                srv = q.replace("host=", "");
        }
        {
            //            QMutexLocker lock(&workers_lock);

            rmWorkers.insert(qMakePair(srv, port.toInt()));
            //workers_status.remove(QString("%1:%2").arg(srv,port));
        }

        HTMLstatus(res, QString("Removed server %1:%2").arg(srv,port));
        return;
    }

    if (urlpath.startsWith("/Status"))
    {
        QJsonObject ob;
        procs.getStatus(ob);
        //            qDebug() << path << ob;
        setHttpResponse(ob, res, !query.contains("json"));
    }

    if (urlpath.startsWith("/ServerDone"))
    { // if all servers are done & the task list is not empty
        // empty the task list & re-run
        //next_worker
        QString serverIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());
        QString port;
        int CPU;

        QStringList queries = query.split("&");
        for (auto q : queries)
        {
            if (q.startsWith("port="))
                port = q.replace("port=", "");
            if (q.startsWith("cpu="))
                CPU = q.replace("cpu=", "").toInt();
        }

        workers_status[QString("%1:%2").arg(serverIP, port)] = CPU;

        int sum = 0;
        for (auto it = workers_status.begin(), e = workers_status.end(); it != e; ++it)
            sum += it.value();

        if (sum == 0 && running.size() > 0)
        { // re start
            qDebug() << "Restarting" << running.size() << "Jobs";
            workers_lock.lock();
            for (auto& obj: running.values())
            {
                auto project = obj["Project"].toString();
                int priority = obj.contains("Priority") ? obj["Priority"].toInt() : 1;

                if (project_affinity.contains(project))
                {
                    jobs[project_affinity[project]][priority].enqueue(obj);
                }
                else
                    jobs[""][priority].enqueue(obj); // Unmapped project goes to "global" path

            }
            running.clear();
            workers_lock.unlock();
        }
    }

    if (urlpath.startsWith("/Restart"))
    {
        qDebug() << "Restarting" << running.size() << "Jobs";
        workers_lock.lock();
        for (auto& obj: running.values())
        {
            auto project = obj["Project"].toString();
            int priority = obj.contains("Priority") ? obj["Priority"].toInt() : 1;

            if (project_affinity.contains(project))
            {
                jobs[project_affinity[project]][priority].enqueue(obj);
            }
            else
                jobs[""][priority].enqueue(obj); // Unmapped project goes to "global" path


        }
        running.clear();
        workers_lock.unlock();
        return;
    }


    if (urlpath.startsWith("/Cancel/"))
    {

        QStringList queries = query.split("&");
        int cancelProcs = 0;
        QString proc;
        for (auto& q : queries)
        {
            if (q.startsWith("proc="))
            {
                proc = q.replace("proc=","");

                QMutexLocker lock(&workers_lock);
                // Clear up the jobs
                QStringList job = proc.split("#");
                if (job.size() < 2)
                    break;
                QStringList name = job[0].split("@");
                QString jobname  = job[1];
                QString workid = job.back();

                qDebug() << "rm job command:" << name << jobname << workid;
                for (auto& srv: jobs)
                {
                    for (auto& q: srv)
                    {
                        QList<QJsonObject> torm;

                        for (auto& item: q)
                        {
                            if (item["Username"].toString() == name[0] &&
                                    item["Computer"].toString() == name[1] &&
                                    item["Path"].toString() == jobname &&
                                    item["WorkID"].toString() == workid)
                                torm << item;
                        }
                        cancelProcs = (int)torm.size();

                        qDebug() << "Trying to remove" << cancelProcs;
                        for (auto& it : torm)
                            q.removeAll(it);


                    }
                }

            }
        }
        HTMLstatus(res, QString("Canceled processes : %1 %2").arg(cancelProcs).arg(proc));
        return;
    }

    if (urlpath.startsWith("/UsedCPU") )
    {
        uint16_t port;
        QString serverIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());
        int cpus = 1;

        QStringList queries = query.split("&");
        for (auto q : queries)
        {
            if (q.startsWith("port"))
            {
                port = q.replace("port=","").toUInt();
            }
            if (q.startsWith("cpus"))
            {
                cpus = q.replace("cpus=","").toInt();
                if (cpus < 1) cpus = 1;
            }
        }

        qDebug() << "Suspending" << serverIP << "CPU on port" << port;
        QMutexLocker lock(&workers_lock);
        //        workers.enqueue(qMakePair(serverIP, port));
        for (int i = 0; i < cpus; ++i) workers.removeOne(qMakePair(serverIP, port));
        workers_status[QString("%1:%2").arg(serverIP).arg(port)] -= cpus;;

    }

    if (urlpath.startsWith("/Start/"))
    {


        QString refIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());
        QString proc = urlpath.mid(7);

        auto ob = QCborValue::fromCbor(data).toJsonValue().toArray();
        QJsonArray Core,Run;

        for (int i = 0; i < ob.size(); ++i)
        {
            QJsonObject obj = ob.at(i).toObject();


            QByteArray arr = QCborValue::fromJsonValue(obj).toCbor();
            arr += QDateTime::currentDateTime().toMSecsSinceEpoch();

            Core.append(obj["CoreProcess_hash"]);
            Run.append(obj["CoreProcess_hash"].toString());

            obj["Process_hash"] = obj["CoreProcess_hash"];
            if (req->connection()->tcpSocket()->peerAddress() ==
                    req->connection()->tcpSocket()->localAddress())
                obj["LocalRun"] = true;
            obj["ReplyTo"] = refIP; // Address to push results to !
            auto project = obj["Project"].toString();
            int priority = obj.contains("Priority") ? obj["Priority"].toInt() : 10000000 - ob.size();

            workers_lock.lock();
            if (project_affinity.contains(project))
            {
                jobs[project_affinity[project]][priority].enqueue(obj);
            }
            else
                jobs[""][priority].enqueue(obj); // Unmapped project goes to "global" path

            workers_lock.unlock();

            QString workid = QString("%1@%2#%3#%4")
                    .arg(obj["Username"].toString(), obj["Computer"].toString(),
                    obj["Path"].toString(), obj["WorkID"].toString());

            work_count[workid]++;

            ob.replace(i, obj);
        }

        QJsonObject obj;
        obj["ArrayCoreProcess"] = Core;
        obj["ArrayRunProcess"] = Run;

        setHttpResponse(obj, res, !query.contains("json"));
        //        QtConcurrent::run(&procs, &CheckoutProcess::startProcessServer,
        //                          proc, ob);

    }

    if (data.size())
    {
        auto root = QJsonDocument::fromJson(data).object();

        if ( root.isEmpty()  ||  root.value("name").toString() != QLatin1String("add") ) {
            const static char KMessage[] = "Invalid json format!";
            res->setStatusCode(qhttp::ESTATUS_BAD_REQUEST);
            res->addHeader("connection", "close");
            res->addHeaderValue("content-length", strlen(KMessage));
            res->end(KMessage);
            return;
        }

        int total = 0;
        auto args = root.value("args").toArray();
        for ( const auto jv : args ) {
            total += jv.toInt();
        }
        root["args"] = total;

        QByteArray body = QJsonDocument(root).toJson();
        //        res->addHeader("connection", "keep-alive");
        res->addHeaderValue("content-length", body.length());
        res->setStatusCode(qhttp::ESTATUS_OK);
        res->end(body);
    }
    else
    {
        QString body = QString("Server Query received, with empty content (%1)").arg(urlpath);
        //        res->addHeader("connection", "close");
        res->addHeaderValue("content-length", body.length());
        res->setStatusCode(qhttp::ESTATUS_OK);
        res->end(body.toLatin1());
    }
}

uint Server::serverPort()
{
    return this->tcpServer()->serverPort();
}

void Server::finished(QString hash, QJsonObject ob)
{
    //    qDebug() << "Finishing on server side";



    NetworkProcessHandler::handler().finishedProcess(hash, ob);
}


void Server::exit_func()
{

#ifdef WIN32
    _control->quit();
#endif
    //  qApp->quit();
}





#ifdef WIN32

Control::Control(Server* serv): QWidget(), _serv(serv), lastNpro(0)
{
    hide();

    quitAction = new QAction("Quit Checkout Server", this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));
    QSettings sets;


    trayIconMenu = new QMenu(this);
    _cancelMenu = trayIconMenu->addMenu("Cancel Processes:");
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/ServerIcon.png"));
    trayIcon->setToolTip(QString("Checkout Proxy %2 (%1)").arg(sets.value("ServerPort", 13378).toUInt()).arg(CHECKOUT_VERSION));
    trayIcon->show();

    startTimer(2000);

}

void Control::timerEvent(QTimerEvent * event)
{
    Q_UNUSED(event); // No need for the event timer object in this function

    QSettings sets;

    CheckoutProcess& procs = CheckoutProcess::handler();
    int npro = procs.numberOfRunningProcess();
    QString tooltip;
    if (npro != 0)
        tooltip = QString("CheckoutQueueServer %2 (%1 requests").arg(npro).arg(CHECKOUT_VERSION);
    else
        tooltip = QString("CheckoutQueueServer %2 (%1)").arg(_serv->serverPort()).arg(CHECKOUT_VERSION);

    QStringList missing_users;
    for (auto user : procs.users())
    {
        tooltip = QString("%1\r\n%2").arg(tooltip).arg(user);
        bool missing = true;
        for (auto me: _users)
            if (me->text() == user)
            {
                missing = false;
                break;
            }
        if (missing) missing_users << user;
    }

    for (auto old: _users)
    {
        bool rem = true;
        for (auto ne: procs.users())
        {
            if (ne == old->text())
                rem = false;
        }
        if (rem)
            _users.removeOne(old);
    }
    trayIcon->setToolTip(tooltip);

    if (lastNpro != npro && npro == 0)
        trayIcon->showMessage("Checkout Server", "Checkout server has finished all his process");

    lastNpro = npro;

    for (auto user: missing_users)
        _users.append(_cancelMenu->addAction(user));
}



#endif


