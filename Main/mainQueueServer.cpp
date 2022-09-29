
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


QString storage_path;

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

QProcessEnvironment python_config;

#include <checkout_python.h>

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

    if (data.contains("-h") || data.contains("-help"))
    {
        qDebug() << "CheckoutQueue Serveur Help:";
        qDebug() << "\t-p <port>: specify the port to run on";
        qDebug() << "\t-d : Run in debug mode (windows only launches a console)";
        qDebug() << "\t-Crashed: Reports that the process has been restarted after crashing";
        qDebug() << "\t-c <config>: Specify a config file for python env setting json dict";
        qDebug() << "\t-t <path> :  Set the storage path for plugins postprocesses";
        qDebug() << "\t\t shall contain env. variable with list of values,";
        qDebug()<< "$NAME will be substituted by current env value called 'NAME'";
        ;

        exit(0);
    }

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


    if (data.contains("-c"))
    {
        QString config;
        int idx = data.indexOf("-c")+1;
        if (data.size() > idx) config = data.at(idx);
        qDebug() << "Using server config:" << config;

        QFile loadFile(config);

        parse_python_conf(loadFile, python_config);

    }

    if (data.contains("-t"))
    {
        int idx = data.indexOf("-t")+1;
        QString file;
        if (data.size() > idx) storage_path = data.at(idx);
        qDebug() << "Setting Storage path :" << storage_path;
    }
    else
    {
#if WIN32
        storage_path = "L:/";
#else
        storage_path = "/mnt/shares/L/";
#endif
    }


#if WIN32

    if (data.contains("-d"))
        show_console();
#endif




    Server server;
    server.setPort(port);
    if (QFile::exists(QString("%1/Temp/checkout_queue_pendingjobs.json").arg(storage_path)))
    {
        server.recover(QString("%1/Temp/checkout_queue_pendingjobs.json").arg(storage_path));
    }


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
        if (!rmWorkers.contains(srv))
        {
            servers[srv] ++;
        }

    body += "<h3>Cores Listing</h3>";
    //for (auto it = runni)
    for (auto it = workers_status.begin(), e = workers_status.end(); it != e; ++it)
    {
        auto t = it.key().split(":");
        if (rmWorkers.contains(it.key())) continue;
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

    body += pendingTasks(true).join("<br>") + "<br>";

    body += "<h3>Running Jobs</h3>";

    QMap<QString, int> runs;
    for (auto task = running.begin(); task != running.end(); ++task)
        runs[task.key().split("!").first()]++;

    for (auto it = runs.begin(), e = runs.end(); it != e; ++it)
        body += QString("%1 => %2  <a href='/Details/?proc=%1'>Details</a>&nbsp;<a href='/Restart'>Force Restart</a><br>").arg(it.key()).arg(it.value());

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


QStringList Server::pendingTasks(bool html)
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
        QString key = it.key();
        res << QString("%1 => %2").arg((html ? key + "<a href='/Cancel/?proc=" + key.replace(" ", "")
                                               +"'>Cancel</a>&nbsp;" : key)).arg(it.value());
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
            if (!next_worker.isEmpty())
            {
                QQueue<QJsonObject>& queue = getHighestPriorityJob(next_worker);


                // Hey hey look what we have here: the job to be run by next_worker let's call the start func then :
                // start it :)
                if (queue.size())
                {

                    QJsonObject pr = queue.dequeue();
                    /*  if (!proc_params[pr["Path"].toString()].contains(next_worker))
                    {
                        next_worker = pickWorker(pr["Path"].toString());

                    }*/

                    CheckoutHttpClient* sr = nullptr;
                    if (clients[next_worker])
                        sr = clients[next_worker];
                    else
                    {
                        QStringList srl = next_worker.split(":");
                        clients[next_worker] = new CheckoutHttpClient(srl[0], srl[1].toInt());
                        sr = clients[next_worker];
                    }
                    //                    sr->setCollapseMode(false);
                    QString taskid = QString("%1@%2#%3#%4!%5#%6:%7")
                            .arg(pr["Username"].toString(), pr["Computer"].toString(),
                            pr["Path"].toString(), pr["WorkID"].toString(),
                            pr["XP"].toString(), pr["Pos"].toString(), pr["Process_hash"].toString());

                    pr["TaskID"] = taskid;

                    qDebug()  << "Sending Work ID"  << taskid << "to" << next_worker;
                    QJsonArray ar; ar.append(pr);

                    sr->send(QString("/Start/%1").arg(pr["Path"].toString()), QString(""), ar);

                    workers.removeOne(next_worker);
                    workers_status[next_worker]--;
                    pr["SendTime"] = QDateTime::currentDateTime().toSecsSinceEpoch();
                    running[taskid] = pr;
                }

            }
            workers_lock.unlock();
            //            QThread::msleep(2);
        }
        else
        {
            QThread::msleep(300); // Wait for 300ms at least
        }

        qApp->processEvents();
    }
}

QString Server::pickWorker(QString )
{

    static QString lastsrv;

    if (lastsrv.isEmpty())
    {

        auto next_worker = workers.back();
        int p = 1;
        while (rmWorkers.contains(next_worker) && p < workers.size())  next_worker = workers.at(p++);
        if (p==workers.size()) return QString();
        lastsrv=next_worker;
        return next_worker;
    }

    for(auto& nxt: workers)
        if (lastsrv != nxt && !rmWorkers.contains(nxt) )
        {
            lastsrv = nxt;
            return nxt;
        }

    auto next = workers.back();
    lastsrv = next;
    return next;

}


#include <checkout_arrow.h>


QString adjust(QString script)
{
#ifdef WIN32
    return storage_path + script;
#else
    return storage_path + script.replace(":", "");
#endif
}

void Server::process( qhttp::server::QHttpRequest* req,  qhttp::server::QHttpResponse* res)
{

    const QByteArray data = req->collectedData();
    QString urlpath = req->url().path(), query = req->url().query();


    qDebug()  << QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz")
              << qhttp::Stringify::toString(req->method())
              << qPrintable(urlpath)
              << qPrintable(query)
              << data.size();

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




    if (urlpath.startsWith("/Ready")) // Server is ready /Ready/{port}
    {
        //        qDebug() << proc_params;
        QString serverIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());

        uint16_t port;

        QStringList queries = query.split("&");
        QString workid, cpu;

        bool reset = false;
        int avail = 0;
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
            {
                avail = (q.replace("available=", "").toInt());
                avail = avail > 0 ? avail : 1;
            }
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

        QString cw = QString("%1:%2").arg(serverIP).arg(port);

        if (reset) // Occurs if servers reconnect
        {
            workers.removeAll(cw);
            workers_status[cw]=0;
        }
        auto ob = QCborValue::fromCbor(data).toJsonValue().toArray();

        if (avail > 0 && ob.size() == 0 )
        {
            if (workers_status[cw] >= 0)
            {
                if (cpu.isEmpty())
                    workers.enqueue(cw);
                else
                    for (int i= 0; i <  (cpu.isEmpty() ? avail : cpu.toInt()) ; ++i)
                        workers.enqueue(cw);

            }
        }


        if (!cpu.isEmpty())
            workers_status[cw]+=cpu.toInt();
        else if (ob.size() == 0)
            workers_status[cw]++;

        if (!workid.isEmpty())
        {
            qDebug() << "Finished " << workid;
            auto t = QDateTime::currentDateTime().toSecsSinceEpoch() - running[workid]["SendTime"].toInt();
            QStringList wid = workid.split("!");
            QString ww = QString("%1!%2").arg(wid[0], wid[1].split("#")[0]);

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
                QStringList wid = obj["TaskID"].toString().split("!");
                QString wwid = QString("%1!%2").arg(wid[0], wid[1].split("#")[0]);
                QString ww = obj["TaskID"].toString().split("!")[0];
                QString jobid = wid[0];

                run_time[ww] += t;
                run_count[ww] ++;

                work_count[wwid] --;
                perjob_count[jobid]--;

                qDebug() << "Work ID finished" << obj["TaskID"] << wwid << work_count.value(wwid);
                if (work_count.value(wwid) == 0)
                {
                    qDebug() << "Work ID finished" << wwid << "Aggregate & collate data" << obj["TaskID"].toString();
                    QSettings set;
                    QString dbP = set.value("databaseDir", "L:/").toString();

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

                    QStringList files = dir.entryList(QStringList() << QString("%1_[0-9]*[0-9][0-9][0-9][0-9].fth").arg(agg["XP"].toString().replace("/", "")), QDir::Files);
                    QString concatenated = QString("%1/%2.fth").arg(path,agg["XP"].toString().replace("/",""));
                    if (files.isEmpty())
                    {
                        qDebug() << "Error fusing the data to generate" << concatenated;
                        qDebug() << QString("%4_[0-9]*[0-9][0-9][0-9][0-9].fth").arg(agg["XP"].toString().replace("/", ""));
                    }
                    else
                    {

                        if (QFile::exists(concatenated)) // In case the feather exists already add this for fusion at the end of the process since arrow fuse handles duplicates if will skip value if recomputed and keep non computed ones (for instance when redoing a well computation)
                        {
                            dir.rename(concatenated, concatenated + ".torm");
                            files << concatenated.split("/").last()+".torm";
                        }

                        fuseArrow(path, files, concatenated,agg["XP"].toString().replace("\\", "/").replace("/",""));
                    }

                    // qDebug() << agg.keys() << obj.keys();

                    if (agg.contains("PostProcesses") && agg["PostProcesses"].toArray().size() > 0)
                    {
                        qDebug() << "We need to run the post processes:" << agg["PostProcesses"].toArray();
                        // Set our python env first
                        // Setup the call to python
                        // Also change working directory to the "concatenated" folder
                        auto arr = agg["PostProcesses"].toArray();
                        for (int i = 0; i < arr.size(); ++i )
                        { // Check if windows or linux conf, if linux changes remove ":" and prepend /mnt/shares/ at the begining of each scripts
                            QString script = arr[i].toString().replace("\\", "/");
                            auto args = QStringList() << adjust(script)  << concatenated;
                            QProcess* python = new QProcess();


                            postproc << python;

                            python->setProcessEnvironment(python_config);
                            qDebug() << python_config.value("PATH");
                            qDebug() << args;

                            postproc.last()->setProcessChannelMode(QProcess::MergedChannels);
                            postproc.last()->setStandardOutputFile(path+"/"+script.split("/").last().replace(".py", "")+".log");
                            postproc.last()->setWorkingDirectory(path);

                            postproc.last()->setProgram(python_config.value("CHECKOUT_PYTHON", "python"));
                            postproc.last()->setArguments(args);

                            postproc.last()->start();
                        }
                    }
                    else
                        qDebug() << "No Postprocesses";



                    if (  perjob_count[jobid] == 0 && agg.contains("PostProcessesScreen")&& agg["PostProcessesScreen"].toArray().size() > 0)
                    {
                        qDebug() << "We need to run the post processes:" << agg["PostProcessesScreen"].toArray();
                        // Set our python env first
                        // Setup the call to python
                        // Also change working directory to the "concatenated" folder
                        auto data = agg["Experiments"].toArray();
                        QStringList xps;
                        for (auto d: data) xps << d.toString().replace("\\", "/").replace("/", "")+".fth";


                        auto arr = agg["PostProcessesScreen"].toArray();
                        for (int i = 0; i < arr.size(); ++i )
                        { // Check if windows or linux conf, if linux changes remove ":" and prepend /mnt/shares/ at the begining of each scripts
                            QString script = arr[i].toString().replace("\\", "/");
                            QDir dir(path);

                            auto args = QStringList() << adjust(script);
                            //<< concatenated;
                            for (auto & d: xps) if (QFile::exists(path+"/"+d))  args << d;
                            QProcess* python = new QProcess();

                            postproc << python;

                            python->setProcessEnvironment(python_config);
                            qDebug() << python_config.value("PATH");
                            qDebug() << args;

                            postproc.last()->setProcessChannelMode(QProcess::MergedChannels);
                            postproc.last()->setStandardOutputFile(path+"/"+script.split("/").last().replace(".py", "")+".screen_log");
                            postproc.last()->setWorkingDirectory(path);

                            postproc.last()->setProgram(python_config.value("CHECKOUT_PYTHON", "python"));
                            postproc.last()->setArguments(args);

                            postproc.last()->start();
                        }

                    }
                    else
                        qDebug() << "No Screen Post-processes";
                }

                // Check if we have finished the TaskID subsets to pure task id i.e. Screen processing and launch subsequents multiplate python runners
                running.remove(obj["TaskID"].toString());

                workers.enqueue(cw);
                workers_status[cw]++;
            }


        }
        {
            QJsonObject ob;
            setHttpResponse(ob, res, false);
        }
        return;
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
        {
            QJsonObject ob;
            setHttpResponse(ob, res, false);
        }
        return;
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
        {
            QJsonObject ob;
            setHttpResponse(ob, res, false);
        }
        //        qDebug() << proc_params;
        return;
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
            //delete client;
        }

        rmWorkers.remove(QString("%1:%2").arg(srv, port));

        client = new CheckoutHttpClient(srv, port.toUInt());
        client->send("/Proxy", QString("port=%1").arg(dport), QJsonArray());

        QJsonObject ob;
        setHttpResponse(ob, res, false);
        return;
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
        rmWorkers.insert(QString("%1:%2").arg(srv, port));

        HTMLstatus(res, QString("Removed server %1:%2").arg(srv,port));
        return;
    }

    if (urlpath.startsWith("/Status"))
    {
        QJsonObject ob;
        procs.getStatus(ob);
        //            qDebug() << path << ob;
        setHttpResponse(ob, res, !query.contains("json"));
        return;
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
        {
            QJsonObject ob;
            setHttpResponse(ob, res, false);
        }
        return;
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

    if (urlpath.startsWith("/Details/"))
    {

        QStringList queries = query.split("&");
        QString proc, body;
        for (auto& q : queries)
        {
            if (q.startsWith("proc="))
            {
                proc = q.replace("proc=","");

                for (auto& srv: jobs)
                {
                    for (auto& q: srv)
                    {
                        QList<QJsonObject> torm;

                        for (auto& obj: q)
                        {
                            QString workid = QString("%1@%2#%3#%4!%5#%6:%7")
                                    .arg(obj["Username"].toString(), obj["Computer"].toString(),
                                    obj["Path"].toString(), obj["WorkID"].toString(),
                                    obj["XP"].toString(),  obj["Pos"].toString(), obj["Process_hash"].toString());

                            body += QString("<li>%1</li>").arg(workid);
                        }
                    }
                }
            }
        }

        auto message = QString("<html><title>Checkout Processes Listing</title><body><ul>%1</ul></body></html>").arg(body);


        res->setStatusCode(qhttp::ESTATUS_OK);
        res->addHeaderValue("content-length", message.size());
        res->end(message.toUtf8());

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
                QStringList job = proc.split(":");
                if (job.size() < 2)
                    break;
                QStringList name = job[0].split("@");
                int idx = job[1].indexOf('(');
                QString jobname  = job[1].mid(0, idx).replace(" ", "");
                QString workid = job[1].mid(idx+1).replace(")", "");


                qDebug() << "rm job command:" << name[0] << name[1] << jobname << workid;
                for (auto& srv: jobs)
                {
                    for (auto& q: srv)
                    {
                        QList<QJsonObject> torm;

                        for (auto& item: q)
                        {
                 /*           qDebug() << item["Username"].toString() << item["Computer"].toString()
                                << item["Path"].toString() << item["WorkID"].toString();*/
                            if (item["Username"].toString() == name[0] &&
                                    item["Computer"].toString() == name[1] &&
                                    item["Path"].toString().replace(" ", "") == jobname &&
                                    item["WorkID"].toString() == workid)
                                torm << item;
                        }
                        cancelProcs += (int)torm.size();

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
        QString cw = QString("%1:%2").arg(serverIP).arg(port);
        for (int i = 0; i < cpus; ++i) workers.removeOne(cw);
        workers_status[cw] -= cpus;;

        {
            QJsonObject ob;
            setHttpResponse(ob, res, false);
        }
        return;
    }

    if (urlpath.startsWith("/Quit"))
    {
        QJsonObject ob;
        setHttpResponse(ob, res, false);

        workers_lock.lock();
        QJsonArray ar;

        // Look for jobs
        for (auto& srv: jobs.keys())
            for (auto& prio: jobs[srv].values())
                for (auto& obj: prio)
                    ar.append(obj);

        // Look for ongoing (unwatched) tasks
        for (auto& tasks: running.values())
            ar.append(tasks);
        if (ar.size())
        {
            QFile store(QString("%1/Temp/checkout_queue_pendingjobs.json").arg(storage_path));

            if (store.open(QFile::WriteOnly))
                store.write(QCborValue::fromJsonValue(ar).toCbor());
        }
        qApp->exit(0);
        workers_lock.unlock();
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

            QString workid = QString("%1@%2#%3#%4!%5")
                    .arg(obj["Username"].toString(), obj["Computer"].toString(),
                    obj["Path"].toString(), obj["WorkID"].toString(),
                    obj["XP"].toString());
            QString jobid = QString("%1@%2#%3#%4")
                    .arg(obj["Username"].toString(), obj["Computer"].toString(),
                    obj["Path"].toString(), obj["WorkID"].toString());

            work_count[workid]++;
            perjob_count[jobid]++;

            ob.replace(i, obj);
        }

        QJsonObject obj;
        obj["ArrayCoreProcess"] = Core;
        obj["ArrayRunProcess"] = Run;

        setHttpResponse(obj, res, !query.contains("json"));
        //        QtConcurrent::run(&procs, &CheckoutProcess::startProcessServer,
        //                          proc, ob);
        return;

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
        res->addHeader("connection", "close");
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
    for (auto& user : procs.users())
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

    for (auto & old: _users)
    {
        bool rem = true;
        for (auto &ne: procs.users())
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

void Server::recover(QString f)
{
    QFile io(f);
    if (io.open(QFile::ReadOnly))
    {
       auto ar = QCborValue::fromCbor(io.readAll()).toArray();
       int prio = ar.size();
       for (int i = 0; i < prio; i++)
       {
           auto obj = ar[i].toJsonValue().toObject();
           jobs[""][prio].enqueue(obj);
       }
    }
}


