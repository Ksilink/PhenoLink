
#ifndef MDWRKAPI_HPP
#define MDWRKAPI_HPP


#include "zmsg.hpp"
#include "mdp.hpp"

//  Reliability parameters
#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable

#include <Dll.h>

struct GlobParams
{
    int running_threads = 0;
    int max_threads = 0;
};



//  Structure of our class
//  We access these properties only via class methods
class DllCoreExport mdwrk {
    GlobParams& global_parameters;
public:

    //  ---------------------------------------------------------------------
    //  Constructor

    mdwrk (QString broker, QString service, GlobParams& gp, int verbose):
        global_parameters(gp),
        m_broker(broker), m_service(service), m_context(new zmq::context_t (1)), m_worker(0),
        m_verbose(verbose), m_heartbeat(5000), m_reconnect(2500), m_expect_reply(false)

    {
        s_version_assert (4, 0);


        s_catch_signals ();
        connect_to_broker ();
    }

    //  ---------------------------------------------------------------------
    //  Destructor

    virtual
        ~mdwrk ()
    {
        delete m_worker;
        delete m_context;
    }



    void set_worker_preamble(std::string nbTh, zmsg* msg)
    {
        m_nbThreads = nbTh;
        m_preamble = msg;

        send_to_broker((char*)MDPW_PROCESSLIST, nbTh, msg);
    }


    //  ---------------------------------------------------------------------
    //  Send message to broker
    //  If no _msg is provided, creates one internally
    void send_to_broker(char *command, std::string option = "", zmsg *_msg = NULL)
    {
        zmsg *msg = _msg? new zmsg(*_msg): new zmsg ();

        //  Stack protocol envelope to start of message
        if (option.length() != 0) {
            msg->push_front ((char*)option.c_str());
        }
        msg->push_front (command);
        msg->push_front ((char*)MDPW_WORKER);
        msg->push_front ((char*)"");

        if (m_verbose) {
            s_console ("I: sending %s to broker",
                      mdps_commands [(int) *command]);
            msg->dump ();
        }
        msg->send (*m_worker);
        delete msg;
    }

    //  ---------------------------------------------------------------------
    //  Connect or reconnect to broker

    void connect_to_broker ()
    {
        if (m_worker) {
            delete m_worker;
        }
        m_worker = new zmq::socket_t (*m_context, ZMQ_DEALER);
        m_worker->set(zmq::sockopt::linger, 0);
        s_set_id(*m_worker);
        m_worker->connect (m_broker.toLatin1().data());
        if (m_verbose)
            qDebug() << "I: connecting to broker at" << m_broker;

        //  Register service with broker
        send_to_broker ((char*)MDPW_READY, m_service.toStdString());

        //  If liveness hits zero, queue is considered disconnected
        m_liveness = HEARTBEAT_LIVENESS;
        m_heartbeat_at = s_clock () + m_heartbeat;
    }




    //  ---------------------------------------------------------------------
    //  Set heartbeat delay

    void
    set_heartbeat (int heartbeat)
    {
        m_heartbeat = heartbeat;
    }


    //  ---------------------------------------------------------------------
    //  Set reconnect delay

    void
    set_reconnect (int reconnect)
    {
        m_reconnect = reconnect;
    }

    //  ---------------------------------------------------------------------
    //  Send reply, if any, to broker and wait for next request.

    zmsg *recv (zmsg *&reply_p);

private:
    QString m_broker;
    QString m_service;
    zmq::context_t *m_context;
    zmq::socket_t  *m_worker;     //  Socket to broker
    int m_verbose;                //  Print activity to stdout

    //  Heartbeat management
    int64_t m_heartbeat_at;      //  When to send HEARTBEAT
    size_t m_liveness;            //  How many attempts left
    int m_heartbeat;              //  Heartbeat delay, msecs
    int m_reconnect;              //  Reconnect delay, msecs

    //  Internal state
    bool m_expect_reply;           //  Zero only at start

    //  Return address, if any
    QString m_reply_to;

    zmsg* m_preamble;
    std::string m_nbThreads;

};



#endif // MDWRKAPI_HPP
