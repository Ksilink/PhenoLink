
#ifndef MDWRKAPI_HPP
#define MDWRKAPI_HPP


#include "zmsg.hpp"
#include "mdp.hpp"

//  Reliability parameters
#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable

//  Structure of our class
//  We access these properties only via class methods
class mdwrk {
public:

    //  ---------------------------------------------------------------------
    //  Constructor

    mdwrk (QString broker, QString service, int verbose)
    {
        s_version_assert (4, 0);

        m_broker = broker;
        m_service = service;
        m_context = new zmq::context_t (1);
        m_worker = 0;
        m_expect_reply = false;
        m_verbose = verbose;
        m_heartbeat = 2500;     //  msecs
        m_reconnect = 2500;     //  msecs

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

    zmsg *
    recv (zmsg *&reply_p)
    {
        //  Format and send the reply if we were provided one
        zmsg *reply = reply_p;
        assert (reply || !m_expect_reply);
        if (reply) {
            assert (m_reply_to.size()!=0);
            reply->wrap (m_reply_to, "");
            m_reply_to = "";
            send_to_broker ((char*)MDPW_REPLY, "", reply);
            delete reply_p;
            reply_p = 0;
        }
        m_expect_reply = true;

        while (!s_interrupted) {
            zmq::pollitem_t items[] = {
                                       { *m_worker,  0, ZMQ_POLLIN, 0 } };
            zmq::poll (items, 1, std::chrono::milliseconds(m_heartbeat));

            if (items[0].revents & ZMQ_POLLIN) {
                zmsg *msg = new zmsg(*m_worker);
                if (m_verbose)
                {
                    s_console ("I: received message from broker:");
                    msg->dump ();
                }
                m_liveness = HEARTBEAT_LIVENESS;

                //  Don't try to handle errors, just assert noisily
                assert (msg->parts () >= 3);

                auto empty = msg->pop_front ();
                assert (empty.compare((unsigned char *)"") == 0);
                //assert (strcmp (empty, "") == 0);
                //free (empty);

                auto header = msg->pop_front ();
                assert (header.compare((unsigned char *)MDPW_WORKER) == 0);
                //free (header);

                auto command = msg->pop_front ();
                if (command.compare (MDPW_REQUEST) == 0) {
                    //  We should pop and save as many addresses as there are
                    //  up to a null part, but for now, just save one...
                    m_reply_to = msg->unwrap ();
                    return msg;     //  We have a request to process
                }
                else if (command.compare (MDPW_HEARTBEAT) == 0) {
                    //  Do nothing for heartbeats
                }
                else if (command.compare (MDPW_DISCONNECT) == 0) {
                    connect_to_broker ();
                }
                else {
                    s_console ("E: invalid input message (%d)",
                              (int) *(command.data()));
                    msg->dump ();
                }
                delete msg;
            }
            else
                if (--m_liveness == 0) {
                    if (m_verbose) {
                        s_console ("W: disconnected from broker - retrying...");
                    }
                    s_sleep (m_reconnect);
                    connect_to_broker ();
                }
            //  Send HEARTBEAT if it's time
            if (s_clock () >= m_heartbeat_at) {
                auto nbThreads = QString("%1").arg(global_parameters.max_threads-global_parameters.running_threads).toStdString();

                send_to_broker ((char*)MDPW_HEARTBEAT, nbThreads);
                m_heartbeat_at += m_heartbeat;
            }
        }
        if (s_interrupted)
            printf ("W: interrupt received, killing worker...\n");
        return NULL;
    }

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
};



#endif // MDWRKAPI_HPP
