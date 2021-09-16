
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



int forceNumaAll(int node)
{

    HANDLE process = GetCurrentProcess();

    DWORD_PTR processAffinityMask;
    DWORD_PTR systemAffinityMask;
    ULONGLONG  processorMask;

    if (!GetProcessAffinityMask(process, &processAffinityMask, &systemAffinityMask))
        return -1;

    GetNumaNodeProcessorMask(node, &processorMask);

    processAffinityMask = processAffinityMask & processorMask;

    BOOL success = SetProcessAffinityMask(process, processAffinityMask);

    qDebug() << success << GetLastError();

    return success;
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

#if WIN32
    if (data.contains("-n"))
    {
        int idx = data.indexOf("-n")+1;
        int node = 0;
        if (data.size() > idx) node = data.at(idx).toInt();
        qDebug() << "Forcing node :" << node;
        forceNumaAll(node);

    }

    if (data.contains("-d"))
        show_console();

#endif
    if (data.contains("-s"))
    {
        int idx = data.indexOf("-s")+1;
        QString file;
        if (data.size() > idx) file = data.at(idx);
        qDebug() << "Loading startup script :" << file;
        startup_execute(file);
    }

    PluginManager::loadPlugins(true);


    Server server;

#ifndef WIN32
    if (data.contains("-m")) // to override the default path mapping !
    {
        int idx = data.indexOf("-m")+1;
        QString map_path = data.at(idx);
        server.setDriveMap(map_path);
    }
    else
    { // We ain't on a windows system, so let's default the mapping to a default value
        server.setDriveMap("/mnt/shares");
    }
#endif



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
        printf("a new connection has occured!\n");
    });



    bool isListening = listen(
                QString::number(port),
                [this]( qhttp::server::QHttpRequest* req,  qhttp::server::QHttpResponse* res){
            req->collectData();
            req->onEnd([this, req, res](){
        process(req, res);
    });
});

    if ( !isListening ) {
        qDebug("can not listen on %d!\n", port);
        return -1;
    }

    QtConcurrent::run(this, &Server::WorkerMonitor);

    CheckoutProcess& procs = CheckoutProcess::handler();
    connect(&procs, SIGNAL(finishedJob(QString,QJsonObject)),
            this, SLOT(finished(QString, QJsonObject)));

    return qApp->exec();
}

void Server::setHttpResponse(QJsonObject& ob,  qhttp::server::QHttpResponse* res, bool binary)
{
    QByteArray body =  binary ? QCborValue::fromJsonValue(ob).toCbor() :
                                QJsonDocument(ob).toJson();
    res->addHeader("Connection", "keep-alive");

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

    CheckoutProcess& procs = CheckoutProcess::handler();

    message = procs.dumpHtmlStatus();

    res->setStatusCode(qhttp::ESTATUS_OK);
    res->addHeaderValue("content-length", message.size());
    res->end(message.toUtf8());
}

QMutex priority_lock, workers_lock;

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



void Server::WorkerMonitor()
{

    while (true) // never ending run
    {
        if (jobs.count() && ! workers.isEmpty()) // If we have some jobs left and some workers available

        {
            while (!workers.isEmpty())
            {
                workers_lock.lock();
                auto next_worker = workers.dequeue();
                workers_lock.unlock();
                QQueue<QJsonObject>& queue = getHighestPriorityJob(next_worker.first);

                // Hey hey look what we have here: the job to be run by next_worker let's call the start func then :
                // start it :)



            }

        }
        else
        {
            QThread::msleep(300); // Wait for 300ms at least
        }
    }
}

void Server::process( qhttp::server::QHttpRequest* req,  qhttp::server::QHttpResponse* res)
{

    const QByteArray data = req->collectedData();
    QString urlpath = req->url().path(), query = req->url().query();

    qDebug() << qhttp::Stringify::toString(req->method())
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
        ob["Processes"] = QJsonArray::fromStringList(prcs);

        setHttpResponse(ob, res, !query.contains("json"));
        return;
    }

    if (urlpath.startsWith("/Process/"))
    {
        QString path = urlpath.mid(9);
        //          qDebug() << path;
        QJsonObject ob;
        procs.getParameters(path, ob);
        //            qDebug() << path << ob;
        setHttpResponse(ob, res, !query.contains("json"));
    }

    if (urlpath== "/status.html" || urlpath=="/index.html")
    {
        HTMLstatus(res);
        return;
    }

    if (urlpath.startsWith("/Ready")) // Server is ready /Ready/{port}
    {
        QString serverIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());
        QString port = urlpath.replace("/Ready/", "");

        QStringList queries = query.split("&");
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
        }

        QMutexLocker lock(&workers_lock);
        workers.enqueue(qMakePair(serverIP, port.toUInt()));
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
        res->addHeader("connection", "keep-alive");
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
    //       trayIconMenu->addAction(minimizeAction);
    //       trayIconMenu->addAction(maximizeAction);
    //       trayIconMenu->addAction(restoreAction);
    //       trayIconMenu->addSeparator();
    _cancelMenu = trayIconMenu->addMenu("Cancel Processes:");
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/ServerIcon.png"));
    trayIcon->setToolTip(QString("Checkout Server %2 (%1)").arg(sets.value("ServerPort", 13378).toUInt()).arg(CHECKOUT_VERSION));
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
        tooltip = QString("CheckoutServer %2 (%1 requests").arg(npro).arg(CHECKOUT_VERSION);
    else
        tooltip = QString("CheckoutServer %2 (%1)").arg(_serv->serverPort()).arg(CHECKOUT_VERSION);

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


