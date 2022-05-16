
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
    auto res = QtConcurrent::run(&Server::WorkerMonitor, this);
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

    res->addHeader("Content-Length", QString::number(body.length()).toLatin1());
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
            servers[srv] ++;

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

    body += pendingTasks().join("<br>") + "<br>";

    body += "<h3>Running Jobs</h3>";

    QMap<QString, int> runs;
    for (auto task = running.begin(); task != running.end(); ++task)
        runs[task.key().split("!").first()]++;

    for (auto it = runs.begin(), e = runs.end(); it != e; ++it)
        body += QString("%1 => %2 <a href='/Cancel/?proc=%1'>Cancel</a>&nbsp;<a href='/Restart'>Force Restart</a><br>").arg(it.key()).arg(it.value());

    message = QString("<html><title>Checkout Queue Status %2</title><body>%1</body></html>").arg(body).arg(CHECKOUT_VERSION);


    res->setStatusCode(qhttp::ESTATUS_OK);
    res->addHeader("content-length", QString::number(message.size()).toLatin1());
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
        req->addHeader("content-length", QString::number(body.length()).toLatin1());
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
                    QString srv = next_worker; // Set proxy name
                    CheckoutHttpClient* sr = nullptr;
                    if (clients[srv])
                        sr = clients[srv];
                    else
                    {
                        auto t = next_worker.split(":");
                        clients[srv] = new CheckoutHttpClient(t[0], t[1].toInt());
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

QString Server::pickWorker()
{
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
        if (lastsrv != nxt && !rmWorkers.contains(nxt) )
        {
            lastsrv = nxt;
            return nxt;
        }

    auto next = workers.back();
    lastsrv = next;
    return next;

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
        QFile of(urlpath.mid(8));
        if (of.open(QFile::ReadOnly))
        {
            QByteArray body = of.readAll();

            res->addHeader("Connection", "close");
            res->addHeader("Content-Type", "image/jpeg");

            res->addHeader("Content-Length", QString::number(body.length()).toLatin1());
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
        QString w = QString("%1:%2").arg(serverIP).arg(port);


        if (reset) // Occurs if servers reconnect
        {
            workers.removeAll(w);
            workers_status[w]=0;
        }
        auto ob = QCborValue::fromCbor(data).toJsonValue().toArray();

        if (avail && ob.size() == 0 )
        {
            if (workers_status[w] >= 0)
            {
                for (int i= 0; i< (cpu.isEmpty() ? 1 : cpu.toInt()); ++i)
                    workers.enqueue(w);
            }
        }


        if (!cpu.isEmpty())
            workers_status[w]+=cpu.toInt();
        else if (ob.size() == 0)
            workers_status[w]++;

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
                QString ww = workid.split("!")[0];

                run_time[ww] += t;
                run_count[ww] ++;

                running.remove(obj["TaskID"].toString());
                workers.enqueue(w);
                workers_status[w]++;
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

        rmWorkers.remove(QString("%1:%2").arg(srv, port));

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

            rmWorkers.insert(QString("%1:%2").arg(srv, port));
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
        QString w = QString("%1:%2").arg(serverIP).arg(port);
        for (int i = 0; i < cpus; ++i) workers.removeOne(w);
        workers_status[w] -= cpus;;

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
            res->addHeader("content-length", QString::number(strlen(KMessage)).toLatin1());
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
        res->addHeader("content-length", QString::number(body.length()).toLatin1());
        res->setStatusCode(qhttp::ESTATUS_OK);
        res->end(body);
    }
    else
    {
        QString body = QString("Server Query received, with empty content (%1)").arg(urlpath);
        //        res->addHeader("connection", "close");
        res->addHeader("content-length", QString::number(body.length()).toLatin1());
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


