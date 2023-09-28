#ifndef MDBROKER_HPP
#define MDBROKER_HPP
//
//  Majordomo Protocol broker
//  A minimal implementation of http://rfc.zeromq.org/spec:7 and spec:8
//
//     Andreas Hoelzlwimmer <andreas.hoelzlwimmer@fh-hagenberg.at>
//
#include "zmsg.hpp"
#include "mdp.hpp"

#include <map>
#include <set>
#include <deque>
#include <list>


#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QString>
#include <QCborMap>
#include <QCborArray>
#include <QCborValue>
#include <QMutex>



#include <Dll.h>

QMutex access_mutex;


//  We'd normally pull these from config data

#ifdef HEARTBEAT_LIVENESS
#undef HEARTBEAT_LIVENESS
#endif

#define HEARTBEAT_LIVENESS  5       //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  2500    //  msecs
#define HEARTBEAT_EXPIRY    HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS

struct service;


struct  service_call
{
    QString path; // The called service
    QString project; // The project involved for priority mapping
    QString client; // id of the calling client

    int calls; // Number of process involved by this call

    QCborMap callMap; // Mapping names from parameters
    QCborMap parameters; // parameters (with encoded parameters)

    int priority; // if we have computed priority for this service call :)

    int thread_id;

};


//  This defines one worker, idle or active
struct  worker
{
    QString m_identity;   //  Address of worker
    QString m_name; // name of the worker
    service * m_service;      //  Owning service, if known
    int64_t m_expiry;         //  Expires at unless heartbeat
    int available;

    QStringList priority_plugins; // List of plugins that shall be processed with high priority


    worker(QString identity, service * service = 0, int64_t expiry = 0) {
        m_identity = identity;
        m_service = service;
        m_expiry = expiry;
    }

    QMap<QString, QJsonObject> m_plugins;
};


// this define a computing entity
struct  worker_threads
{
    worker_threads(worker* wrk, int id): m_worker(wrk), m_id(id), parameters(0)
    {

    }

    worker * m_worker; // Owner of this thread
    int m_id;
    service_call* parameters; // if this one was called, we have parameters associated
};


//  This defines a single service
struct DllCoreExport service
{
    ~service ();

    QString m_name;             //  Service name
    std::deque<zmsg*> m_requests;   //  List of client requests
    QList<worker*> m_process;  //  List of workers containing the service
    QList<worker*> m_pending; // List of worker with pending process

    //    size_t m_workers;               //  How many workers we have

    service(QString name)
    {
        m_name = name;
    }

    bool add_worker(worker* wrk);


};



//  This defines a single broker
class broker {
public:

    //  ---------------------------------------------------------------------
    //  Constructor for broker object

    broker (int verbose)
    {
        //  Initialize broker state
        m_context = new zmq::context_t(1);
        m_socket = new zmq::socket_t(*m_context, ZMQ_ROUTER);
        m_verbose = verbose;
    }

    //  ---------------------------------------------------------------------
    //  Destructor for broker object

    virtual
        ~broker ()
    {
        while (! m_services.empty())
        {
            delete m_services.begin().value();
            m_services.erase(m_services.begin());
        }
        while (! m_workers.empty())
        {
            delete m_workers.begin().value();
            m_workers.erase(m_workers.begin());
        }
    }

    //  ---------------------------------------------------------------------
    //  Bind broker to endpoint, can call this multiple times
    //  We use a single socket for both clients and workers.

    void
    bind (QString endpoint)
    {
        m_endpoint = endpoint;
        m_socket->bind(m_endpoint.toLatin1().data());
        qDebug() << "I: MDP broker/0.1.1 is active at " << endpoint;
    }

private:

    //  ---------------------------------------------------------------------
    //  Delete any idle workers that haven't pinged us in a while.

    void
    purge_workers ()
    {
        QSet<worker*> toCull;
        int64_t now = s_clock();
        for (QSet<worker_threads*>::iterator wrk = m_waiting_threads.begin(), ewrk = m_waiting_threads.end();
             wrk != ewrk; ++wrk)
        {
            if ((*wrk)->m_worker->m_expiry <= now)
            {
                toCull.insert(  (*wrk)->m_worker);
                if ((*wrk)->parameters) // recovering lost jobs
                {
                    for (auto& job: m_ongoing_jobs)
                        if (job == (*wrk)->parameters)
                        {
                            m_requests.push_front(job); // push back in front lost jobs
                        }
                }

            }
        }


        for (QSet<worker*>::iterator wrk = toCull.begin(); wrk != toCull.end(); ++wrk)
        {
            if (m_verbose) {
                qDebug() << "I: deleting expired worker:" << (*wrk)->m_identity;
            }
            worker_delete((*wrk), 0);
        }
    }

    //  ---------------------------------------------------------------------
    //  Locate or create new service entry

    service *
    service_require (QString name)
    {
        assert (!name.isEmpty());
        if (m_services.count(name)) {
            return m_services[name];
        } else {
            service * srv = new service(name);
            m_services[name] = srv;
            if (m_verbose) {
                s_console ("I: received message:");
            }
            return srv;
        }
    }



    //  ---------------------------------------------------------------------
    //  Dispatch requests to waiting workers as possible

    void
    service_dispatch (service *srv, zmsg *msg)
    {
        //        assert (srv);

        if (!srv) return;


        if (msg) {                    //  Queue message if any

            auto client  = msg->pop_front(),
                empty = msg->pop_front();

            qDebug() << "Client" << client << "-" << empty;

            //            QCborMap
            auto map = QCborValue::fromCbor(msg->pop_front()).toMap();
            // New service call
            //
            //            << msg->pop_front();
            //                       qDebug() << msg->pop_front()
            int nbCalls = msg->parts();
            qDebug() << "Service Call" << nbCalls;
            while (msg->parts())
            {
                service_call* call = new service_call;

                call->path = srv->m_name;
                //                call->project;
                call->client = client;

                call->callMap = map;
                call->calls = nbCalls;
                call->parameters = QCborValue::fromCbor(msg->pop_front()).toMap();
                call->priority = 0;

                m_requests.push_back(call);
            }


            srv->m_requests.push_back(msg);
        }

        purge_workers ();
        // FIXME

        while (!m_waiting_threads.empty() && ! m_requests.empty())
        { // we have threads & we have requests !!!

            worker* wrk = nullptr; // Search first worker most recently seen
            for (auto w = m_workers.begin(), e = m_workers.end(); w != e; ++w)
            {
                if (wrk == nullptr)
                {
                    if (w.value()->available > 0)
                        wrk = *w;
                }
                else
                    if (w.value()->m_expiry > wrk->m_expiry && w.value()->available > 0)
                        wrk = *w;
            }

            // Assign thread
            worker_threads* th = nullptr;
            for (auto thi: m_waiting_threads)
                if (thi->m_worker == wrk)
                {
                    th = thi;
                        break;
                }

            if (th != nullptr)
            {
                service_call* job = nullptr;
                int priority = 0;

                if (!wrk->priority_plugins.isEmpty())
                { // if the worker has no priority list

                    // first search for plugin priority list if any job was subscribed
                    for (auto &plugin: qAsConst(wrk->priority_plugins))
                        for (auto jbs: m_requests)
                            if (plugin == jbs->path && jbs->calls < low_job_number && jbs->priority > priority)
                            {
                                job = jbs;
                                priority = jbs->priority;
                            }

                    // If no low count priority jobs check if we have long
                    if (job == nullptr)
                        for (auto & plugin: qAsConst(wrk->priority_plugins))
                            for (auto jbs: m_requests)
                                if (plugin == jbs->path && jbs->priority > priority)
                                {
                                    job = jbs;
                                    priority = jbs->priority;
                                }
                }

                if (job == nullptr)
                    for (auto jbs: m_requests) // Handle low number of jobs first
                        if (jbs->calls < low_job_number && jbs->priority > priority)
                        {
                            job = jbs;
                            priority = jbs->priority;
                        }

                if (job == nullptr)
                    job = m_requests.front();

                // We have a job !
                // We have a worker !
                // Let's send some work
                start_job(wrk, th, job);

            }
//            else
//                qDebug() << "No worker threads found for worker";

        }




    }

    //  ---------------------------------------------------------------------
    //  Handle internal service according to 8/MMI specification

    void
    service_internal (QString service_name, zmsg *msg)
    {
        qDebug() << "Broker service call" << service_name << msg->parts();

        //  Remove & save client return envelope and insert the
        //  protocol header and service name, then rewrap envelope.
        QString client = msg->unwrap();


        if (service_name == "mmi.service") {
            service * srv = m_services[msg->body()];
            if (srv && !srv->m_process.isEmpty()) {
                msg->body_set("200");
            } else {
                msg->body_set("404");
            }
        }
        if (service_name == "mmi.list")
            {
                if (msg->parts())
                {
                    auto item = msg->pop_front();
                    if (item.isEmpty())
                    {
                        //                        qDebug() << "Sending list";

                        for (auto procs = m_services.keyBegin(), end = m_services.keyEnd(); procs != end; ++procs)
                        {
                            if (!procs->startsWith("mmi."))
                                {
                                    qDebug() << (*procs);
                                    msg->push_back((*procs).toLatin1());
                                }
                        }
                    }
                    else
                    {
                        qDebug() << "Searching Parameters info for" << client << item;
                        if (m_services.contains(item))
                        {
                            if (m_services[item]->m_process.size() > 0)
                            {
                                if (m_services[item]->m_process.first()->m_plugins.contains(item))
                                {
                                    //                                    qDebug() << "Found item" << item;
                                    qDebug() << m_services[item]->m_process.first()->m_plugins[item];
                                    auto data = QCborValue::fromJsonValue(m_services[item]->m_process.first()->m_plugins[item]).toCbor();
                                    msg->push_back(data);

                                }
                                else
                                    qDebug() << "Badly registered plugin" << item << "No workers...";
                            }
                            else
                            {
                                qDebug() << "No workers for" << item;
                            }

                        }
                        else
                            qDebug() << "Service not found" << item;

                    }
                }
                else
                {
                    //                    qDebug() << "Sending list";

                    for (auto procs = m_services.keyBegin(), end = m_services.keyEnd(); procs != end; ++procs)
                    {
                        //                        qDebug() << (*procs);
                        if (!procs->startsWith("mmi."))
                            msg->push_back((*procs).toLatin1());
                    }
                }
            }
        if (service_name == "mmi.status")
            {
                // Count the number of finished process for this client & clean the list
                int nb_finished_jobs =  clear_list(m_finished_jobs, client);

                msg->push_back(QString::number(nb_finished_jobs).toLatin1());

//                qDebug() << "Job Status: " << nb_finished_jobs << m_finished_jobs.size() << m_ongoing_jobs.size();
            }

        if (service_name == "mmi.cancel")
            {
                // Cancel all the jobs for this client
                int nb_canceled = clear_list(m_requests, client);

                clear_list(m_ongoing_jobs, client);
                clear_list(m_finished_jobs, client);

                msg->push_back(QString::number(nb_canceled).toLatin1());
            }



        /*else
            {
                msg->body_set("501");
            }*/


        msg->wrap(MDPC_CLIENT, service_name);
        msg->wrap(client, "");
        msg->send (*m_socket);
        delete msg;
    }

    //  ---------------------------------------------------------------------
    //  Creates worker if necessary

    worker *
    worker_require (QString identity)
    {
        assert (identity.length()!=0);

        //  self->workers is keyed off worker identity
        if (m_workers.count(identity)) {
            return m_workers[identity];
        } else {
            worker *wrk = new worker(identity);
            m_workers[identity] = wrk;
            if (m_verbose) {
                qDebug() << "I: registering new worker:" << identity;
            }
            return wrk;
        }
    }

    //  ---------------------------------------------------------------------
    //  Deletes worker from all data structures, and destroys worker

    void
    worker_delete (worker *wrk, int disconnect)
    {
        assert (wrk);
        if (disconnect) {
            qDebug() << "Explicit destruction";
            worker_send (wrk, (char*)MDPW_DISCONNECT, "", NULL);
        }


        QList<service*> toCull;
        for (auto srv : m_services)
        {
            if (srv->m_process.contains(wrk))
                srv->m_process.removeAll(wrk);
            if (srv->m_process.isEmpty())
                toCull.append(srv);
        }


        for (auto srv: toCull)
        {
            qDebug() << "Warning service" << srv->m_name << " has no more workers handling it";
        }
        if (toCull.size()!=0)
              qDebug() << "We'll keep track of pending job to the missing services";

        for (auto wt = m_waiting_threads.begin(), end = m_waiting_threads.end(); end != wt; )
        {
            if ((*wt)->m_worker == wrk)
                wt = m_waiting_threads.erase(wt);
            else
                ++wt;
        }

        for (auto wt = m_workers_threads.begin(), end = m_workers_threads.end(); end != wt; )
        {
            if ((*wt)->m_worker == wrk)
            {
                delete (*wt);
                wt = m_workers_threads.erase(wt);
            }
            else
                ++wt;
        }




        qDebug() << "Cleaning Worker"
                 << wrk;

        //        m_workers_threads.remove(wrk);

        m_workers.remove(wrk->m_identity);
        delete wrk;
    }



    //  ---------------------------------------------------------------------
    //  Process message sent to us by a worker

    void
    worker_process (QString sender, zmsg *msg)
    {
        assert (msg && msg->parts() >= 1);     //  At least, command

        auto command = msg->pop_front();
        bool exists = m_workers.count(sender);
       worker *wrk = worker_require (sender);

        if (command.compare (MDPW_READY) == 0) {

            {
                if (sender.size() >= 4  //  Reserved service name
                    &&  sender.startsWith("mmi.")) {
                    worker_delete (wrk, 1);
                } else {
                    //  Attach worker to service and mark as idle
                    auto client  = msg->pop_front ();

                    auto thread_id = msg->pop_front().toInt();

                    worker_threads* th = nullptr;
                    for (auto wt: m_workers_threads)
                      if (wt->m_id == thread_id && wt->m_worker == wrk)
                            {
                                th = wt;
                                break;
                      }

                    QList<service_call*> toCull;
                    for (auto &job : m_ongoing_jobs)
                      if (job->client == client && job->thread_id == thread_id)
                      {
                                toCull << job;
                                th->parameters = nullptr;
                                m_finished_jobs << job;
                      }
                    for (auto& j : toCull) m_ongoing_jobs.removeOne(j);


                    wrk->available++;

                    wrk->m_expiry = s_clock () + HEARTBEAT_EXPIRY;

                    if (th)
                    {
                        m_waiting_threads.insert(th);
                        if (!m_requests.empty())
                          service_dispatch (service_require(m_requests.front()->path),  nullptr);
                    }
                }
            }
            wrk->m_expiry = s_clock () + HEARTBEAT_EXPIRY;
        } else {
            if (command.compare (MDPW_REPLY) == 0) {
//                if (worker_ready) {
                    //  Remove & save client return envelope and insert the
                    //  protocol header and service name, then rewrap envelope.
                    QString client = msg->unwrap ();
                    msg->wrap (QString(MDPC_CLIENT));//, wrk->m_service->m_name);
                    msg->wrap (client);
                    msg->send (*m_socket);
                    worker_waiting (wrk);
                 wrk->m_expiry = s_clock () + HEARTBEAT_EXPIRY;
            } else {
                if (command.compare (MDPW_HEARTBEAT) == 0) {
                        wrk->m_expiry = s_clock () + HEARTBEAT_EXPIRY;
                    if (!exists)
                    {
                        worker_send(wrk, (char*)MDPW_PROCESSLIST, 0, 0);
                    }


                } else {
                    if (command.compare (MDPW_DISCONNECT) == 0) {
                        worker_delete (wrk, 0);
                    } else
                        if (command.compare(MDPW_PROCESSLIST) == 0)
                        {
                            // Keep track of processes
                            auto nbThreads = msg->pop_front().toInt();


                            qDebug() << "Available Workers" << nbThreads;

                            // Nb workers avail for this
                            auto data = msg->pop_front();
                            while (data.size())
                            {
                                //                            auto dat = QByteArray((const char*)data.c_str(), data.size());
                                auto json = QJsonDocument::fromJson(data).object();
                                auto path = json["Path"].toString();

                                qDebug() << path << json["PluginVersion"];
                                wrk->m_plugins[path] = json;

                                data = msg->pop_front();

                                auto srv = service_require(path);

                                if (!srv->add_worker(wrk))
                                    qDebug() << "Not adding worker to service";

                                //                          QDateTime::fromString(json["PluginVersion"].toString().mid(8,19), "yyyy-MM-dd hh:mm:ss");
                            }
                            wrk->available = nbThreads;
                            for (int i = 0; i < nbThreads; ++i)
                            {
                                auto w = new worker_threads(wrk, i);
                                m_workers_threads.insert(w);
                                m_waiting_threads.insert(w);
                            }
                            wrk->m_expiry = s_clock () + HEARTBEAT_EXPIRY;

                            if (!m_requests.empty()) // we have requests and a server is back from the dead
                                service_dispatch (service_require(m_requests.front()->path),  nullptr);
                        }
                        else
                        {
                            s_console ("E: invalid input message (%d)", (int) *command.data());
                            msg->dump ();
                        }
                }
            }
        }
    }

    //  ---------------------------------------------------------------------
    //  Send message to worker
    //  If pointer to message is provided, sends that message

    void
    worker_send (worker *worker,
                QByteArray command, QString option = QString(), zmsg *msg = NULL)
    {
        msg = (msg ? new zmsg(*msg) : new zmsg ());

        //  Stack protocol envelope to start of message
        if (!option.isEmpty()) {                 //  Optional frame after command
            msg->push_front (option.toLatin1());
        }
        msg->push_front (command);
        msg->push_front (MDPW_WORKER);

        msg->wrap(worker->m_identity);

        msg->send (*m_socket);


        delete msg;
    }

    //  ---------------------------------------------------------------------
    //  This worker is now waiting for work

    void
    worker_waiting (worker *worker)
    {
        assert (worker);

        service_dispatch (worker->m_service, 0);
    }



    //  ---------------------------------------------------------------------
    //  Process a request coming from a client

    void
    client_process (QString sender, zmsg *msg)
    {
        assert (msg);     //  Service name + body

        auto service_name = msg->pop_front();
        service *srv = service_require (service_name);
        //  Set reply return address to client sender
        msg->wrap (sender);
        if (service_name.length() >= 4
            &&  service_name.startsWith("mmi.") ) {
            service_internal (service_name, msg);
        } else {
            service_dispatch (srv, msg);
        }
    }

public:

    //  Get and process messages forever or until interrupted
    inline void
    start_brokering() {
        int64_t now = s_clock();
        int64_t heartbeat_at = now + HEARTBEAT_INTERVAL;
        while (!s_interrupted) {
            zmq::pollitem_t items [] = {
                                       { *m_socket, 0, ZMQ_POLLIN, 0} };
            int64_t timeout = heartbeat_at - now;
            if (timeout < 0)
                timeout = 0;
            zmq::poll (items, 1, std::chrono::milliseconds(timeout));

            //  Process next input message, if any
            if (items [0].revents & ZMQ_POLLIN) {
                zmsg *msg = new zmsg(*m_socket);
                if (m_verbose) {
                    s_console ("I: received message:");
                    msg->dump ();
                }
                auto sender = msg->pop_front ();
                msg->pop_front (); //empty message
                auto header = msg->pop_front ();

                //              std::cout << "sbrok, sender: "<< sender << std::endl;
                //              std::cout << "sbrok, header: "<< header << std::endl;
                //              std::cout << "msg size: " << msg->parts() << std::endl;
                //              msg->dump();
                if (header.compare(MDPC_CLIENT) == 0) {
                    client_process (sender, msg);
                }
                else if (header.compare(MDPW_WORKER) == 0) {
                    worker_process (sender, msg);
                }
                else {
                    qDebug() << sender << header;
                    s_console ("E: invalid message:");
                    msg->dump ();
                    delete msg;
                }
            }
            //  Disconnect and delete any expired workers
            //  Send heartbeats to idle workers if needed
            now = s_clock();
            if (now >= heartbeat_at) {
                purge_workers ();
                for (auto wrk: m_workers)
                    worker_send (wrk, MDPW_HEARTBEAT);

                heartbeat_at += HEARTBEAT_INTERVAL;
                now = s_clock();
            }
        }
    }


    QCborMap recurseExpandParameters(QCborMap ob, QCborMap map)
    {
        QCborMap res;

        for (auto kv = ob.begin(), ekv = ob.end(); kv != ekv; ++kv)
        {
            assert(map.contains(kv.key()));
            QString id = map[kv.key()].toString();


            if (kv.value().isMap())
                res[id] = recurseExpandParameters(kv.value().toMap(), map);
            else if (kv.value().isArray())
            {
                QCborArray r;
                auto aa = kv.value().toArray();
                for (auto ii: aa)
                {
                    if (ii.isMap())
                        r.push_back(recurseExpandParameters(ii.toMap(),  map));
                    else
                        r.push_back(ii);
                }
                res[id] = r;
            } else
                res[id] = kv.value();
        }
        return res;
    }

    void start_job(worker* wrk, worker_threads* thread, service_call* job)
    {

        // Perform the parameter adjust

        job->parameters = recurseExpandParameters(job->parameters, job->callMap);

        job->parameters.insert(QString("ThreadID"), thread->m_id);
        job->parameters.insert(QString("Client"), job->client);
        job->thread_id = thread->m_id;

//        qDebug() << job->parameters;
        thread->parameters = job;

        zmsg* msg = new zmsg();
        msg->push_back(job->parameters.toCborValue().toCbor());
        worker_send(wrk, (char*)MDPW_REQUEST, job->path, msg);


        m_requests.removeAll(job); // Remove the job from the requests
        m_ongoing_jobs.append(job); // Append to the ongoing list

        m_waiting_threads.remove(thread); // remove the thread from waiting list
        wrk->available--; // adjust the work thread avail for this worker instance
        delete msg;
    }

    int clear_list(QList<service_call*>& list, QString client)
    {

        QList<service_call*> toCull;
        int nb_culled = 0;
        for (auto& job: list)
            if (job->client == client)
            {
                nb_culled++;
                toCull << job;
            }
        for (auto& rm: toCull) list.removeOne(rm);

        return nb_culled;
    }



private:
    zmq::context_t * m_context;                  //  0MQ context
    zmq::socket_t * m_socket;                    //  Socket for clients & workers

    int m_verbose;                               //  Print activity to stdout

    QString m_endpoint;                      //  Broker binds to this endpoint


    QMap<QString, service*> m_services;  //  Hash of known services
    QMap<QString, worker*> m_workers;    //  Hash of known workers

    QList<service_call* > m_requests; // List of pending requests
    QList<service_call*> m_ongoing_jobs; // jobs that are running
    QList<service_call*> m_finished_jobs;

    QSet<worker_threads*> m_workers_threads;
    QSet<worker_threads*> m_waiting_threads;

    // Behavior variables:
    int low_job_number; // Defines what a low number of jobs is for the queue (to priorize)

    //    std::set<worker*> m_waiting;              //  List of waiting workers

    // We need to track the process to rewake it
};



#endif // MDBROKER_HPP
