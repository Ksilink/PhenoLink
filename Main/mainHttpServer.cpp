
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

#include "checkouthttpserver.h"
#include <Core/networkprocesshandler.h>
#include <Core/Dll.h>

extern int DllCoreExport read_semaphore;

#include <opencv2/core/utility.hpp>


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


int CheckoutOpenCVErrorCallback( int status, const char* func_name,
                                 const char* err_msg, const char* file_name,
                                 int line, void* )
{

    qDebug() << "OpenCV issued error";
    qDebug() << "Status" << status << "Function" << func_name << "file: " << file_name << "@" << line;
    qDebug() << "Error message" << err_msg;
    throw cv::Exception(status, err_msg, func_name, file_name, line);
    return 1;
}

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

    cv::setBreakOnError(true);
    cv::redirectError(&CheckoutOpenCVErrorCallback);


    //    qDebug() << "Runtime PATH" << QString("%1").arg(getenv("PATH"));

    QSettings sets;
    uint port = sets.value("ServerPort", 13378).toUInt();
    if (!sets.contains("ServerPort") || sets.value("ServerPort", 13378) != port ) sets.setValue("ServerPort", port);

    QStringList data;
    for (int i = 1; i < ac; ++i) { data << av[i];  }



    if (data.contains("-h") || data.contains("-help"))
    {
        qDebug() << "CheckoutQueue Serveur Help:";
        qDebug()      << "\t-p <port>: specify the port to run on";
        qDebug()        << "\t-c <cpu>: Adjust the maximum number of threads";
        qDebug()        << "\t-n <node>: Try to force NUMA node (windows only)";
        qDebug()        << "\t-s <commands> :  List of commands executed at start time";
        qDebug()        << "\t-m <path> :  Mapping of windows drive to linux drives";
        qDebug()        << "\t-rs <value> :  Maximum concurrent disk access by this instance";
        qDebug()        << "\t-proxy <host> :  Set the queue proxy process computer:port";

        qDebug()        << "\t-d : Run in debug mode (windows only launches a console)";
        qDebug()       << "\t-Crashed: Reports that the process has been restarted after crashing" ;
        qDebug()       << "\t-conf <config>: Specify a config file for python env setting json dict";
        qDebug()        << "\t\t shall contain env. variable with list of values,";
        qDebug()       << "$NAME will be substituted by current env value called 'NAME'";
        ;

        exit(0);
    }


    if (data.contains("-p"))
    {
        int idx = data.indexOf("-p")+1;
        if (data.size() > idx) port = data.at(idx).toInt();
        qDebug() << "Changing server port to :" << port;
    }


    int nb_Threads = QThreadPool::globalInstance()->maxThreadCount() - 1;

    if (data.contains("-c"))
    {
        int idx = data.indexOf("-c")+1;
        if (data.size() > idx) nb_Threads = data.at(idx).toInt();
        qDebug() << "Changing max threads :" << nb_Threads;
    }

    if (nb_Threads < 2) nb_Threads = 2;

    qDebug() << "Max number of threads : " << nb_Threads;
    QThreadPool::globalInstance()->setMaxThreadCount(nb_Threads);


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

    server.setPort(port);

    if (data.contains("-conf"))
    {
        QString config;
        int idx = data.indexOf("-conf")+1;
        if (data.size() > idx) config = data.at(idx);
        qDebug() << "Using server config:" << config;

        QFile loadFile(config);

        QProcessEnvironment python_config;
        parse_python_conf(loadFile, python_config);
        NetworkProcessHandler::handler().setPythonEnvironment(python_config);
    }

#ifndef WIN32
    if (data.contains("-m")) // to override the default path mapping !
    {
        int idx = data.indexOf("-m")+1;
        QString map_path = data.at(idx);
        server.setDriveMap(map_path);
    }
    else
    { // We ain't on a windows system, so let's default the mapping to a default value
        server.setDriveMap("/mnt/shares/");
    }
#endif
    if (data.contains("-a"))
    {
        int idx = data.indexOf("-a")+1;
        QString affinity;
        if (data.size() > idx) affinity = data.at(idx);
        server.affinity(affinity)        ;
    }

    if (data.contains("-rs"))
    {
        int idx = data.indexOf("-rs")+1;
        if (data.size() > idx) {
            bool ok = true;
            int rs = data.at(idx).toInt(&ok);
            if (ok) read_semaphore = rs;
        }
    }

    if (data.contains("-proxy"))
    {
        int idx = data.indexOf("-proxy")+1;
        QString  proxy;
        if (data.size() > idx) proxy = data.at(idx);
        QStringList ps = proxy.split(":");
        if (ps.size() > 2)
        {
            qDebug() << "Error parsing proxy string, should be server:port or server (if port assumed to be 13378)"
                     << proxy;
            exit(-1);
        }
        else
            qDebug() << "Proxy server :" << proxy;

        //if (data.contains("-Crashed"))

        server.proxyAdvert(ps[0], ps.size() == 2 ? ps[1].toInt() : 13378);
        NetworkProcessHandler::handler().addProxyPort(port);
    }
    else
        NetworkProcessHandler::handler().setNoProxyMode();


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
        //        printf("a new connection has occured!\n");
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

    CheckoutProcess& procs = CheckoutProcess::handler();
    connect(&procs, SIGNAL(finishedJob(QString,QJsonObject)),
            this, SLOT(finished(QString, QJsonObject)));

    return qApp->exec();
}

void Server::setHttpResponse(QJsonObject& ob,  qhttp::server::QHttpResponse* res, bool binary)
{
    QByteArray body =  binary ? QCborValue::fromJsonValue(ob).toCbor() :
                                QJsonDocument(ob).toJson();
    res->addHeader("Connection", "close"); //"keep-alive");

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
    if (urlpath.startsWith("/Status"))
    {
        QJsonObject ob;
        procs.getStatus(ob);
        //            qDebug() << path << ob;
        setHttpResponse(ob, res, !query.contains("json"));
    }

    qDebug() << qhttp::Stringify::toString(req->method())
             << qPrintable(urlpath)
             << qPrintable(query)
             << data.size();

    if (urlpath.startsWith("/Cancel/"))
    {
        QString user = urlpath.mid(8);
        procs.cancelUser(user);
        HTMLstatus(res);
        return;
    }

    if (urlpath.startsWith("/Proxy"))
    {

        uint16_t port;
        QString host = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());


        QStringList queries = query.split("&");
        for (auto q : queries)
        {
            if (q.startsWith("port"))
            {
                port = q.replace("port=","").toUInt();
            }
        }


        if (client)
        {
            qDebug() << "Reseting proxy";
            delete client;
            client = nullptr;
        }

        proxyAdvert(host, port);
    }

    if (urlpath.startsWith("/Start/"))
    {
        QString srvIp = stringIP(req->connection()->tcpSocket()->localAddress().toIPv4Address());
        QString refIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());
        QString proc = urlpath.mid(7);

        auto ob = QCborValue::fromCbor(data).toArray().toJsonArray();
        QJsonArray Core,Run;

        qDebug() << "Starting Processes" << ob.size();

        for (int i = 0; i < ob.size(); ++i)
        {
            QJsonObject obj = ob.at(i).toObject();

            QByteArray arr = QCborValue::fromJsonValue(obj).toCbor();
            arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
            //QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

            if (!obj.contains("Process_hash"))
            {
                //                QString sHash = hash.toHex();
                Core.append(obj["CoreProcess_hash"]);
                Run.append(obj["CoreProcess_hash"].toString());
                obj["Process_hash"] = obj["CoreProcess_hash"];
                if (req->connection()->tcpSocket()->peerAddress() ==
                        req->connection()->tcpSocket()->localAddress())
                    obj["LocalRun"] = true;
            }

            if (!obj.contains("ReplyTo"))
                obj["ReplyTo"] = refIP; // Address to push results to !

            ob.replace(i, obj);
        }

        QJsonObject obj;
        obj["ArrayCoreProcess"] = Core;
        obj["ArrayRunProcess"] = Run;

        // Response is obj
        //        qDebug() << ob << refIP; // Address to push results to !
        procs.setServerName(srvIp);

        setHttpResponse(obj, res, !query.contains("json"));
        procs.startProcessServer(proc, ob);

        if (!proxy.isEmpty() && !proxy.startsWith(refIP)) {
            // Shall tell the proxy we have process ongoing that where not sent from his side
            qDebug() << "Warn the Proxy about used CPU " << proxy << refIP;
            if (client)
                client->send(QString("/UsedCPU/"),
                             QString("port=%1&cpus=%2").arg(dport).arg(ob.size()), QJsonArray());

        }

    }

    if (data.size())
    {
        auto root = QJsonDocument::fromJson(data).object();

//        if ( root.isEmpty()  ||  root.value("name").toString() != QLatin1String("add") ) {
//            const static char KMessage[] = "Invalid json format!";
//            res->setStatusCode(qhttp::ESTATUS_BAD_REQUEST);
//            res->addHeader("connection", "close");
//            res->addHeaderValue("content-length", strlen(KMessage));
//            res->end(KMessage);
//            return;
//        }

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

void Server::affinity(QString projects)
{
    affinity_list = projects.split(',');
}


#include "qhttp/qhttpclient.hpp"
#include "qhttp/qhttpclientrequest.hpp"
#include "qhttp/qhttpclientresponse.hpp"

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

void Server::proxyAdvert(QString host, int port, bool crashed)
{
    // send /Ready/ command to proxy
    client = new CheckoutHttpClient(host, port);
    proxy = QString("%1:%2").arg(host).arg(port);

    CheckoutProcess& procs = CheckoutProcess::handler();

    // If not using cbor outputs the version also
    QStringList prcs = procs.pluginPaths();

    QJsonArray pro;
    for (auto &pr: prcs)
    {
        QJsonObject obj;
        procs.getParameters(pr, obj);
        pro.append(obj);
    }

    client->send(QString("/setProcessList"),
                 QString(),
                 pro);
    qApp->processEvents();


    int processor_count = QThreadPool::globalInstance()->maxThreadCount();
    client->send(QString("/Ready/%1").arg(processor_count),
                 QString("affinity=%1&port=%2&cpu=%3&reset&available=1&crashed=%4")
                 .arg(affinity_list.join(",")).arg(dport).arg(processor_count).arg(crashed),
                 QJsonArray());


}

void Server::finished(QString hash, QJsonObject ob)
{
    //    qDebug() << "Finishing on server side";

    if (client && ob.contains("TaskID") && !ob["TaskID"].isNull())
    {
        QJsonObject pr;
        pr["TaskID"] = ob["TaskID"].toString();
        QJsonArray ar; ar << pr;

        client->send(QString("/Ready/0"),
                     QString("affinity=%1&port=%2&available=%3")
                     .arg(affinity_list.join(","))
                     .arg(dport)
                     .arg(QThreadPool::globalInstance()->activeThreadCount() <= QThreadPool::globalInstance()->maxThreadCount() ?
                              QThreadPool::globalInstance()->maxThreadCount()-QThreadPool::globalInstance()->activeThreadCount() : 1),
                     ar);
    }

//    NetworkProcessHandler::handler().finishedProcess(hash, ob);

    if (client && CheckoutProcess::handler().numberOfRunningProcess() <= 1)
        client->send(QString("/ServerDone"),
                     QString("port=%1&cpu=%2").arg(dport).arg(QThreadPool::globalInstance()->maxThreadCount()), QJsonArray());

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
    for (auto& user : procs.users())
    {
        tooltip = QString("%1\r\n%2").arg(tooltip).arg(user);
        bool missing = true;
        for (auto& me: _users)
            if (me->text() == user)
            {
                missing = false;
                break;
            }
        if (missing) missing_users << user;
    }

    for (auto& old: _users)
    {
        bool rem = true;
        for (auto& ne: procs.users())
        {
            if (ne == old->text())
                rem = false;
        }
        if (rem)
            _users.removeOne(old);
    }
    trayIcon->setToolTip(tooltip);

    //    if (lastNpro != npro && npro == 0)
    //        trayIcon->showMessage("Checkout Server", "Checkout server has finished all his process");

    lastNpro = npro;

    for (auto& user: missing_users)
        _users.append(_cancelMenu->addAction(user));
}



#endif


