
#include <QtCore>
#ifdef WIN32
#include <QApplication>
#endif

#include <QtNetwork>
#include <QDebug>

#include <fstream>
#include <iostream>


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

#if WIN32

    if (data.contains("-d"))
        show_console();
#endif

    Server server;

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

Server::Server(): QHttpServer()
{
#ifdef WIN32

    _control = new Control(this);

#endif
}

int Server::start(quint16 port)
{
    connect(this, &QHttpServer::newConnection, [this](QHttpConnection*){
        Q_UNUSED(this);
        qDebug() << "a new connection has occured!";
    });



    bool isListening = listen(
                QString::number(port),
                [this]( qhttp::server::QHttpRequest* req,  qhttp::server::QHttpResponse* res){
            req->collectData();
            req->onEnd([this, req, res](){
        qDebug() << "Processing query";
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


void Server::HTMLstatus(qhttp::server::QHttpResponse* res)
{

    QString message;

    //    CheckoutProcess& procs = CheckoutProcess::handler();
    //    message = procs.dumpHtmlStatus();




    res->setStatusCode(qhttp::ESTATUS_OK);
    res->addHeaderValue("content-length", message.size());
    res->end(message.toUtf8());
}

QMutex priority_lock, workers_lock;

unsigned int Server::njobs()
{
    unsigned int count = 0;

    for (auto& srv: jobs)
    {
        for (auto& q: srv)
            count += q.size();
    }

    return count;
}


QQueue<QJsonObject>& Server::getHighestPriorityJob(QString server)
{
    QMutexLocker locker(&priority_lock);

    if (jobs.contains(server)) // does this server have some affinity with the current process ?
    { // if yes we dequeue from this one
        int key = jobs[server].lastKey();
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
            while (!workers.isEmpty())
            {
                workers_lock.lock();
                auto next_worker = workers.back();
                workers_lock.unlock();
                QQueue<QJsonObject>& queue = getHighestPriorityJob(next_worker.first);
                // Hey hey look what we have here: the job to be run by next_worker let's call the start func then :
                // start it :)
                if (queue.size())
                {

                    QJsonObject pr = queue.dequeue();
                    QString srv = QString("%1:%2").arg(next_worker.first).arg(next_worker.second); // Set proxy name
                    CheckoutHttpClient* sr = nullptr;
                    if (clients[srv])
                       sr = clients[srv];
                    else
                    {
                         clients[srv] = new CheckoutHttpClient(next_worker.first, next_worker.second);
                         sr = clients[srv] ;
                    }

                    QString taskid =  QString("%1@%2#%3#%4#%5#%6")
                            .arg(pr["Username"].toString(), pr["Computer"].toString(),
                                 pr["Path"].toString(),  pr["WorkID"].toString(),
                                 pr["XP"].toString(), pr["Pos"].toString());

                    pr["TaskID"]= taskid;

                    qDebug()  << "Taskid" << taskid << pr;
                    QJsonArray ar; ar.append(pr);

                    sr->send(QString("/Start/%1").arg(pr["Path"].toString()), QString(""), ar);
                    workers_lock.lock(); workers.pop_back(); workers_lock.unlock();

                    running[taskid] = pr;


                    qApp->processEvents();
                    break;
                }
            }
        }
        else
        {
            QThread::msleep(300); // Wait for 300ms at least
            qApp->processEvents();
        }
        //qDebug() << "Worker Monitor Loop";
    }
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
        QString workid;
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
            if (q.startsWith("port"))
            {
                port = q.replace("port=","").toUInt();
            }
            if (q.startsWith("workid"))
            {
               workid = q.replace("workid=","");
            }

        }

        QMutexLocker lock(&workers_lock);
        workers.enqueue(qMakePair(serverIP, port));

        if (!workid.isEmpty())
        {
            qDebug() << "Finished " << workid;
            running.remove(workid);
        }

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

    if (urlpath.startsWith("/Status"))
    {
        QJsonObject ob;
        procs.getStatus(ob);
        //            qDebug() << path << ob;
        setHttpResponse(ob, res, !query.contains("json"));
    }


    if (urlpath.startsWith("/Cancel/"))
    {
        QString user = urlpath.mid(8);
        procs.cancelUser(user);
        HTMLstatus(res);
        return;
    }

    if (urlpath.startsWith("") )
    {}

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
            QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

            Core.append(obj["CoreProcess_hash"]);
            QString sHash = hash.toHex();
            Run.append(QString(sHash));
            obj["Process_hash"] = sHash;
            if (req->connection()->tcpSocket()->peerAddress() ==
                    req->connection()->tcpSocket()->localAddress())
                obj["LocalRun"] = true;
            obj["ReplyTo"] = refIP; // Address to push results to !
            auto project = obj["Project"].toString();
            int priority = obj.contains("Priority") ? obj["Priority"].toInt() : 1;

            priority_lock.lock();
            if (project_affinity.contains(project))
            {
                jobs[project_affinity[project]][priority].enqueue(obj);
            }
            else
                jobs[""][priority].enqueue(obj); // Unmapped project goes to "global" path

            priority_lock.unlock();


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


