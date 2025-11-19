#ifndef MDCLIAPI_HPP
#define MDCLIAPI_HPP

#include <Dll.h>
#include "zmsg.hpp"
#include "mdp.hpp"
#include <QCborValue>
#include <QCborMap>

//  Structure of our class
//  We access these properties only via class methods

class  mdcli {
public:

    //  ---------------------------------------------------------------------
    //  Constructor

    mdcli (QString broker, QString id = QString(), int verbose = 0)
    {
        s_version_assert (4, 0);

        m_broker = broker;
        m_context = new zmq::context_t (1);
        m_verbose = verbose;
        m_timeout = 2500;           //  msecs
        m_client = 0;

        s_catch_signals ();
        connect_to_broker (id);
    }


    //  ---------------------------------------------------------------------
    //  Destructor

    virtual
        ~mdcli ()
    {
        delete m_client;
        delete m_context;
    }


    //  ---------------------------------------------------------------------
    //  Connect or reconnect to broker



    void connect_to_broker (QString id = QString())
    {
        if (m_client) {
            delete m_client;
        }
        m_client = new zmq::socket_t (*m_context, ZMQ_DEALER);
        int linger = 0;
        m_client->set(zmq::sockopt::linger, linger);
        if (!id.isEmpty())
            m_client->set(zmq::sockopt::routing_id, id.toStdString());
        else
           s_set_id(*m_client);
        m_client->connect (m_broker.toStdString());
        _status = true;
//        if (m_verbose)
//            /*s_console*/ ("I: connecting to broker at %s...", m_broker.toStdString());
    }


    //  ---------------------------------------------------------------------
    //  Set request timeout

    void
    set_timeout (int timeout)
    {
        m_timeout = timeout;
    }


    //  ---------------------------------------------------------------------
    //  Send request to broker
    //  Takes ownership of request message and destroys it when sent.

    int
    send (std::string service, zmsg *&request_p)
    {
        assert (request_p);
        zmsg *request = request_p;

        //  Prefix request with protocol frames
        //  Frame 0: empty (REQ emulation)
        //  Frame 1: "MDPCxy" (six bytes, MDP/Client x.y)
        //  Frame 2: Service name (printable string)
        request->push_front ((char*)service.c_str());
        request->push_front ((char*)MDPC_CLIENT);
        request->push_front ((char*)"");
        if (m_verbose) {
            s_console ("I: send request to '%s' service:", service.c_str());
            request->dump ();
        }


        request->send (*m_client);
        return 0;
    }


    //  ---------------------------------------------------------------------
    //  Returns the reply message or NULL if there was no reply. Does not
    //  attempt to recover from a broker failure, this is not possible
    //  without storing all unanswered requests and resending them all...

    zmsg *
    recv ()
    {
        //  Poll socket for a reply, with timeout
        zmq::pollitem_t items[] = {
                                   { *m_client, 0, ZMQ_POLLIN, 0 } };
        zmq::poll (items, 1, std::chrono::milliseconds(m_timeout));

        //  If we got a reply, process it
        if (items[0].revents & ZMQ_POLLIN) {
            zmsg *msg = new zmsg (*m_client);
            if (m_verbose) {
                s_console ("I: received reply:");
                msg->dump ();
            }
            //  Don't try to handle errors, just assert noisily
            assert (msg->parts () >= 3);

            assert (msg->pop_front ().length() == 0);  // empty message

            auto header = msg->pop_front();
            assert(header == MDPC_CLIENT);
//            assert (header.compare((unsigned char *)MDPC_CLIENT) == 0);
            auto service = msg->pop_front();

            // Check if this is an error notification
            // We need to peek at the first frame without removing it
            if (msg->parts() > 0)
            {
                // Peek at first frame by popping and checking, then pushing back if not error
                auto firstFrame = msg->pop_front();
                auto data = QCborValue::fromCbor(firstFrame);
                
                if (data.isMap())
                {
                    auto map = data.toMap();
                    if (map.contains(QString("error")))
                    {
                        // This is an error notification!
                        qDebug() << "[CLIENT] Received error notification from broker";
                        handle_error_notification(map);
                        // Don't return this as normal reply
                        delete msg;
                        return nullptr;
                    }
                }
                
                // Not an error - push the frame back so caller gets complete message
                msg->push_front(firstFrame);
            }

            return msg;     //  Success
        }
        if (s_interrupted)
            std::cout << "W: interrupt received, killing client..." << std::endl;
        else
            if (m_verbose)
                s_console ("W: permanent error, abandoning request");
        _status = false;
        return 0;
    }

    bool getStatus()
    {
        return _status;
    }

protected:
    //  ---------------------------------------------------------------------
    //  Handle error notification from broker
    //  Virtual method that can be overridden for custom error handling (e.g., GUI popups)

    virtual void
    handle_error_notification(const QCborMap& error)
    {
        QString errorType = error[QString("error")].toString();
        QString message = error[QString("message")].toString();
        QString service = error[QString("service")].toString();
        
        qWarning() << "[CLIENT ERROR HANDLER] ========================================";
        qWarning() << "[CLIENT ERROR HANDLER] ERROR from broker:" << errorType;
        qWarning() << "[CLIENT ERROR HANDLER] Service:" << service;
        qWarning() << "[CLIENT ERROR HANDLER] Message:" << message;
        qWarning() << "[CLIENT ERROR HANDLER] ========================================";
        
        // Subclasses can override this for GUI popups or other handling
    }

private:
    QString m_broker;
    zmq::context_t * m_context;
    zmq::socket_t * m_client;     //  Socket to broker
    int m_verbose;                //  Print activity to stdout
    int m_timeout;                //  Request timeout
    bool _status;
};

#endif // MDCLIAPI_HPP
