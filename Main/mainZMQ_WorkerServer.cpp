
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

#include <Core/networkprocesshandler.h>


#include <zmq/mdwrkapi.hpp>
#include <system_error>

extern int DllCoreExport read_semaphore;

//static GlobParams global_parameters;
double PCFreq = 0.0;


QString storage_path;

void myMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray date = QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz").toLocal8Bit();
    switch (type) {
    case QtInfoMsg:
        std::cerr << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtDebugMsg:
        std::cerr << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        std::cerr  << date.constData() << " Warning  : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        std::cerr   << date.constData()  << " Critical : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        std::cerr  << date.constData() << " Fatal    : "<< localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        abort();
    }
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



int forceNumaAll(int nodeindex)
{

    HANDLE process = GetCurrentProcess();

    ULONG highestNode = -1;


    if (!GetNumaHighestNodeNumber(&highestNode))
    {
        qDebug() << "No NUMA node available";
        qDebug() << QString::fromStdString(std::system_category().message(GetLastError()));
        return -1;
    }

    qDebug() << "Available NUMA node" << highestNode ;
    if ((ULONG)nodeindex > highestNode)
    {
        qDebug() << "Requested Node above available nodes";
        return -1;
    }


    GROUP_AFFINITY node;
    GetNumaNodeProcessorMaskEx(nodeindex, &node);


    BOOL success = SetProcessAffinityMask(process, node.Mask);
    qDebug() << success << QString::fromStdString(std::system_category().message(GetLastError()));
    success = SetThreadGroupAffinity(GetCurrentThread(), &node, nullptr);
    qDebug() << success << QString::fromStdString(std::system_category().message(GetLastError()));

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


int PhenoLinkOpenCVErrorCallback( int status, const char* func_name,
                                 const char* err_msg, const char* file_name,
                                 int line, void* )
{

    qDebug() << "OpenCV issued error";
    qDebug() << "Status" << status << "Function" << func_name << "file: " << file_name << "@" << line;
    qDebug() << "Error message" << err_msg;
    throw cv::Exception(status, err_msg, func_name, file_name, line);
    return 1;
}
QProcessEnvironment python_config;

#include <checkout_python.h>
#include "ZMQThread.h"

QJsonValue remap(QJsonValue v, QString map);

QJsonObject remap(QJsonObject ob, QString map)
{
    QJsonObject res;

    for (QJsonObject::iterator it = ob.begin(); it != ob.end(); ++it)
    {
        res[it.key()]=remap(it.value(), map);
    }

    return res;
}


QJsonValue remap(QJsonValue v, QString map)
{

    if (v.isString())
    {
        QString value = v.toString();
        if (value.size() > 2 && value[1]==':')
        {
            qDebug() << map << value;
            QJsonValue res = map + value.remove(1,1);
            //            qDebug() << "Remap" << v << res;
            return res;
        }
    }
    else if (v.isArray())
    {
        QJsonArray res;
        for (const auto & item: v.toArray())
        {
            res.append(remap(item, map));
        }
        return res;
    }
    else if (v.isObject())
    {
        v=remap(v.toObject(), map);
    }
    return v;
}




QJsonObject run_plugin(CheckoutProcessPluginInterface* plugin)
{
#ifdef WIN32

    ULONG64 cycles_start, cycles_end;

    QueryThreadCycleTime(GetCurrentThread(), &cycles_start);

#endif
    QJsonObject result ;
    int p = 0;
    try {
        QElapsedTimer dtimer;       // Do the process timing
        dtimer.start();
        p=1;
        plugin->prepareData();
        p=2;
        plugin->started(dtimer.elapsed());
        p=3;
        QElapsedTimer timer;       // Do the process timing
        timer.start();
        p=4;
        plugin->exec();
        p=5;
        plugin->finished();
        p=6;
        result = plugin->gatherData(timer.elapsed());
        p=8;

        qDebug() << "Plugin" << plugin->getPath() << "Finished in " << timer.elapsed() << "(ms)";
#ifdef WIN32
        {
            QSettings set("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", QSettings::Registry64Format );

            PCFreq = set.value("~MHz").toInt()*10;
        }
        QueryThreadCycleTime(GetCurrentThread(), &cycles_end);
        qDebug() << "Consummed CPU Cycles:" << cycles_end-cycles_start << (cycles_end-cycles_start)/ (timer.elapsed()* PCFreq) << "%";
#endif
        p=9;
    }
    catch (...)
    {
        plugin->printMessage(QString("Plugin: %1 failed to run properly, trying to recover (%2)").arg(plugin->getPath()).arg(p));
        plugin->finished();
    }
    // Plugin should be deletable now, should not be saved anywhere
    delete plugin;
    plugin=0;
    return result;
}


void ZMQThread::run()
{

    CheckoutProcess& procs = CheckoutProcess::handler();

    // If not using cbor outputs the version also
    QStringList prcs = procs.pluginPaths();
    qDebug() << "Plugin List" << prcs;
    _plugins = procs.getPlugins();

    //    procs
    // Handle the non QT threading zmq here
    qDebug() << "Starting Session";
    zmsg* processlist = new zmsg();

    processlist->push_back(NetworkProcessHandler::handler().getServer());


    for (auto& item: prcs)
    {
        QJsonObject params;
        procs.getParameters(item, params);

        auto arr = QJsonDocument(params).toJson();//map.toCborValue().toByteArray().toBase64();
        processlist->push_back(arr.data());//item.toLatin1().data());

    }


    auto nbTh = QString("%1").arg(global_parameters.max_threads).toStdString();

    QSet<QString> commitNames;

    session.set_worker_preamble(nbTh, processlist);

    zmsg *reply = nullptr;
    while (!s_interrupted) {
        QString req_type;
        zmsg *request = nullptr;
        std::tie(req_type, request) =
            session.recv (reply);

        if (s_interrupted) break;

        if (req_type == "Request")
        {

            if (request == 0) {
                qDebug() << "Broken answer";
                break;              //  Worker was interrupted
            }
            //

            //        reply = request;        //  Echo is complex... :-)
            //        qDebug() << "srv ok" <<  request->parts();
            auto obj = QCborValue::fromCbor(request->pop_front()).toJsonValue().toObject();
            //        qDebug() << obj["ThreadID"] << obj["Client"];
            QJsonArray ob; ob.push_back(obj);

            commitNames.insert(obj["CommitName"].toString());

            startProcessServer(obj["Path"].toString(), ob);
            // See tj


        }
        else if (req_type == "Finished")
        {
            qDebug() << "Command Finished from broker";
            QString commit =  request->pop_front();

            NetworkProcessHandler::handler().storeObject(commit, true);

        } 
        else if (req_type == "Canceled")
        {
            delete request;
            qDebug() << "Canceled process";
            break;
        }
        else if (req_type == "Timer")
        {           
            if (!commitNames.empty())
            {
                qDebug() << "Save Timer" << commitNames;
                for (auto name : commitNames)
                {
                    NetworkProcessHandler::handler().storeObject(name);
                }
                commitNames.clear();
            }
        }

        if (!reply)
            reply = new zmsg(request->address().toLatin1());

        delete request;

        reply->push_back(QByteArray("OK"));
        reply->push_back(QString("%1").arg(procs.numberOfRunningProcess()).toLatin1());

        //        qDebug() << "Sending OK reply";
    }




    for (auto name : commitNames)
        NetworkProcessHandler::handler().storeObject(name);


    delete processlist;
    qApp->exit(0);
}



inline ZMQThread::ZMQThread(GlobParams &gp, QThread *parentThread, QString prx, QString dmap, bool ver):
    global_parameters(gp),
    verbose(ver),
    proxy(prx), drive_map(dmap),
    mainThread(parentThread), session(QString("tcp://%1").arg(proxy), "processes", gp, verbose)
{

    worker_threadpool.setMaxThreadCount(QThreadPool::globalInstance()->maxThreadCount());
    worker_threadpool.setExpiryTimeout(-1);
}

void ZMQThread::startProcessServer(QString process, QJsonArray array)
{

    qDebug() << "Remaining unstarted processes" //<< _process_to_start.size()
             << worker_threadpool.activeThreadCount()
             << worker_threadpool.maxThreadCount();

    CheckoutProcessPluginInterface* plugin = _plugins[process];

    if (plugin)
    {
        for (int i = 0; i < array.size(); ++i)
        {
            QJsonObject params = array.at(i).toObject();


            QJsonDocument doc(params);
            QByteArray arr = doc.toJson();
            arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
            QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

            QString sHash = hash.toHex();
            params["Process_hash"] = sHash;



            if (!drive_map.isEmpty())
            { // Iterate in each params data to remap drives !
                params = remap(params, drive_map);
            }


            //  qDebug() << "Starting process" << process << params;
            // - 1) clone the plugin: call the clone() function

            plugin = plugin->clone();
            if (!plugin)
            {
                qDebug() << "Plugin cannot be cloned";
                break;
            }

            //params["StartTime"] = QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz");

            //            QString hash = params["Process_hash"].toString();
            // qDebug() << "Process hash" << hash;
            // - 2) Set parameters
            // - 2.a) load json prepared data
            //          qDebug() << "Reading params";
            plugin->read(params);



            QFutureWatcher<QJsonObject>* wa = new QFutureWatcher<QJsonObject>();
            // Connect the finished
            connect(wa, &QFutureWatcher<QJsonObject>::finished, this,  &ZMQThread::thread_finished);
            //            QString key = params["Username"].toString() + "@" + params["Computer"].toString();


            QString key = QString("%1@%2#%3#%4!%5")
                              .arg(params["Username"].toString(), params["Computer"].toString(),
                                   params["Path"].toString(), params["WorkID"].toString(),
                                   params["XP"].toString());

            qDebug() << "Adding Process" << key;
            CheckoutProcess::handler().setNumberOfProcess(CheckoutProcess::handler().numberOfRunningProcess()
                                                          +1);


            QFuture<QJsonObject> fut = QtConcurrent::run(&worker_threadpool, run_plugin, plugin);
            wa->setFuture(fut);
            wa->moveToThread(mainThread);
        }
    }
    else
        qDebug() << "Process" << process << "not found";

    qDebug() << "Process add in thread list";
}


void ZMQThread::save_and_send_binary(QJsonObject *_ob)
{

    auto &ob = *_ob;

    QString hash = ob["Process_hash"].toString();


    //        QString key = QString("%1@%2#%3#%4!%5")
    //                          .arg(ob["Username"].toString(), ob["Computer"].toString(),
    //                               ob["Path"].toString(), ob["WorkID"].toString(),
    //                               ob["XP"].toString());


    // TODO: Uncomment the bellow mentionned entries
    // For debug removed the call to avoid writing useless data

    QJsonArray data = NetworkProcessHandler::handler().filterObject(hash, ob, false);
    QCborArray bin = NetworkProcessHandler::handler().filterBinary(hash, ob);


    // Handle the image transfers
    if (bin.size()!=0)
    {
        QString address = ob["ReplyTo"].toString();
        ///    qDebug() << hash << res << address;
        if (!address.isEmpty())
        {
            CheckoutHttpClient *client = NULL;

            for (CheckoutHttpClient *cl : alive_replies)
                if (address == cl->iurl.host())
                {
                    client = cl;
                }

            if (!client)
            {
                client = new CheckoutHttpClient(address, 8020);
                alive_replies << client;
            }

            for (auto b : bin)
            {
                // FIXME
                // Need to put back image on client
                client->send(QString("/addImage/"), QString(), b.toCbor());
            }
        }
    }

    delete _ob;
}


void ZMQThread::thread_finished()
{
    qDebug() << "Process finished";

    QFutureWatcher<QJsonObject>* wa = dynamic_cast<QFutureWatcher<QJsonObject>* >(sender());
    if (wa)
    {
        CheckoutProcess::handler().setNumberOfProcess(CheckoutProcess::handler().numberOfRunningProcess()
                                                      - 1);


        QJsonObject ob = wa->result();

        QJsonObject* sob = new QJsonObject(ob);


        auto res = QtConcurrent::run(&ZMQThread::save_and_send_binary, this, sob);

        // consider the storage over here
        auto msg = new zmsg(ob["Client"].toString().toLatin1().data());
        msg->push_back(QString("%1").arg(ob["ThreadID"].toInt()).toLatin1());
        session.send_to_broker((char*)MDPW_READY, "", msg);
    }


}



int main(int ac, char** av)
{
    GlobParams global_parameters;

#ifdef WIN32
    QApplication app(ac, av);
#else
    QCoreApplication app(ac, av);
#endif

    app.setApplicationName("PhenoLink");
    app.setApplicationVersion(CHECKOUT_VERSION);
    app.setOrganizationDomain("WD");
    app.setOrganizationName("WD");

    cv::setBreakOnError(true);
    cv::redirectError(&PhenoLinkOpenCVErrorCallback);



    s_version_assert (4, 0);
    s_catch_signals ();

    //    qDebug() << "Runtime PATH" << QString("%1").arg(getenv("PATH"));

    QSettings sets;
    uint port = sets.value("ServerPort", 13555).toUInt();
    if (!sets.contains("ServerPort") || sets.value("ServerPort", 13555) != port ) sets.setValue("ServerPort", port);

    QStringList data;
    for (int i = 1; i < ac; ++i) { data << av[i];  }



    if (data.contains("-h") || data.contains("-help"))
    {
        qDebug() << "PhenoLinkQueue Serveur Help:";
        qDebug()      << "\t-p <port>: specify the port to run on";
        qDebug()      << "\t-c <cpu>: Adjust the maximum number of threads";
        qDebug()      << "\t-n <node>: Try to force NUMA node (windows only)";
        qDebug()      << "\t-s <commands> :  List of commands executed at start time";
        qDebug()      << "\t-m <path> :  Mapping of windows drive to linux drives";
        qDebug()      << "\t-rs <value> :  Maximum concurrent disk access by this instance";
        qDebug()      << "\t-proxy <host> :  Set the queue proxy process computer:port";

        qDebug()      << "\t-t <path> :  Set the storage path for plugins infos";
        qDebug()      << "\t-d : Run in debug mode (windows only launches a console)";
        qDebug()      << "\t-Crashed: Reports that the process has been restarted after crashing" ;
        qDebug()      << "\t-conf <config>: Specify a config file for python env setting json dict";
        qDebug()      << "\t\t shall contain env. variable with list of values,";
        qDebug()      << "\t\t $NAME will be substituted by current env value called 'NAME'";
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
    global_parameters.max_threads = nb_Threads;
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

#endif

    if (data.contains("-pg"))
    {
        int idx = data.indexOf("-pg")+1;
        QString loadMode;
        if (data.size() > idx) loadMode = data.at(idx);

        qApp->setProperty("LoadMode", loadMode);
    }


    int verbose = 0; //(ac > 1 && strcmp (av [1], "-v") == 0);

    if (data.contains("-d"))
    {
#if WIN32
        show_console();
#endif
        verbose = 1;
    }


    if (data.contains("-s"))
    {
        int idx = data.indexOf("-s")+1;
        QString file;
        if (data.size() > idx) file = data.at(idx);
        qDebug() << "Loading startup script :" << file;
        startup_execute(file);
    }


    if (data.contains("-t"))
    {
        int idx = data.indexOf("-t")+1;
        QString file;
        if (data.size() > idx) file = data.at(idx);
        qDebug() << "Setting Storage path :" << file;
        CheckoutProcess::handler().setStoragePath(file);
    }
    else
    {
        QString file;
#if WIN32
        file = "L:/";
#else
        file = "/mnt/shares/L/";
#endif
        qDebug() << "Setting Storage path :" << file;
        CheckoutProcess::handler().setStoragePath(file);
    }

    NetworkProcessHandler::handler().addProxyPort(port);

    PluginManager::loadPlugins(true);

    //    Server server;
    //    server.setPort(port);

    if (data.contains("-conf"))
    {
        QString config;
        int idx = data.indexOf("-conf")+1;
        if (data.size() > idx) config = data.at(idx);
        qDebug() << "Using server config:" << config;

        QFile loadFile(config);

        QProcessEnvironment python_config;
        parse_python_conf(loadFile, python_config);
        CheckoutProcess::handler().setEnvironment(python_config);
    }

    QString drive_map;

#ifndef WIN32
    if (data.contains("-m")) // to override the default path mapping !
    {
        int idx = data.indexOf("-m")+1;
        drive_map = data.at(idx);
    }
    else
    { // We ain't on a windows system, so let's default the mapping to a default value
        drive_map = "/mnt/shares/";
    }
#endif

    //    if (data.contains("-a"))
    //    {
    //        int idx = data.indexOf("-a")+1;
    //        QString affinity;
    //        if (data.size() > idx) affinity = data.at(idx);
    //        server.affinity(affinity)        ;
    //    }

    if (data.contains("-rs"))
    {
        int idx = data.indexOf("-rs")+1;
        if (data.size() > idx) {
            bool ok = true;
            int rs = data.at(idx).toInt(&ok);
            if (ok) read_semaphore = rs;
        }
    }

    QString  proxy;

    if (data.contains("-proxy"))
    {
        int idx = data.indexOf("-proxy")+1;

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
    }
    else
    {
        qDebug() << "Proxy server is mandatory";
        return -1;
    }


    ZMQThread thread(global_parameters, QThread::currentThread(), proxy, drive_map, verbose);
    thread.start();

    return QCoreApplication::exec();
}


#include <checkout_arrow.h>
