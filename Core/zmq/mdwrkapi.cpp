
#define ZMQ_STATIC


#include <QString>
#include <QDebug>


#include "mdwrkapi.hpp"
#include "performances.hpp"
//#include "mdbroker.hpp"

extern struct GlobParams global_parameters;


std::pair<QString, zmsg *> mdwrk::recv(zmsg *&reply_p)
{

    static int timer = 0;
    //  Format and send the reply if we were provided one
    zmsg *reply = reply_p;
    assert (reply || !m_expect_reply);
    if (reply && m_reply_to.size() != 0) {
//        assert (m_reply_to.size()!=0);
        reply->wrap (m_reply_to, "");
        m_reply_to = "";
        send_to_broker ((char*)MDPW_REPLY, "", reply);
        delete reply_p;
        reply_p = 0;
    }
    else if (reply)
    {
        m_reply_to = "";
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
            //if (timer > 20) // every 20 heartbeats force the saving of commit names
            //{
            //    timer = 0;
            //    return  std::make_pair(QString("Timer"), msg);
            //} else
            if (command.compare (MDPW_REQUEST) == 0) {
                //  We should pop and save as many addresses as there are
                //  up to a null part, but for now, just save one...
                m_reply_to = msg->unwrap ();
                return std::make_pair(QString("Request"), msg);     //  We have a request to process
            }
            else if (command.compare (MDPW_HEARTBEAT) == 0) {
                //  Do nothing for heartbeats
            }
            else if (command.compare (MDPW_DISCONNECT) == 0) {
                connect_to_broker ();
            }
            else if (command.compare(MDPW_PROCESSLIST)==0){
                send_to_broker((char*)MDPW_PROCESSLIST, m_nbThreads, m_preamble);
            }
            else if (command.compare(MDPW_FINISHED) == 0)
            {
//                m_reply_to = msg->unwrap ();
                return std::make_pair(QString("Finished"), msg);
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
                qDebug() << "Disconnected from broker" << s_interrupted;
                s_sleep (m_reconnect);
                connect_to_broker ();
                // Send the preamble messages (list of processes)
                send_to_broker((char*)MDPW_PROCESSLIST, m_nbThreads, m_preamble);
            }
        //  Send HEARTBEAT if it's time
        //                qDebug() << s_clock() << m_heartbeat_at;
        if (s_clock () >= m_heartbeat_at) {
            //                    qDebug() << "Sending heartbeat";

                perf_instance.refresh();


                auto* mg = new zmsg();

                // Get computer Memory total
                // Get computer Memory usage
                // Get process Memory usage

                mg->push_back(QString::number(perf_instance.totalPhysMem));
                mg->push_back(QString::number(perf_instance.physMemUsed));
                mg->push_back(QString::number(perf_instance.procPhysMem));

                // Get computer CPU load
                // Get Process CPU Load

                mg->push_back(QString::number(perf_instance.total_cpu_load));
                mg->push_back(QString::number(perf_instance.proc_cpu_load));



            auto nbThreads = QString("%1").arg(global_parameters.max_threads-global_parameters.running_threads).toStdString();

            send_to_broker ((char*)MDPW_HEARTBEAT, nbThreads, mg);
            m_heartbeat_at = s_clock() + m_heartbeat;
            timer++;
        }
    }
    if (s_interrupted)
        printf ("W: interrupt received, killing worker...\n");
    return std::make_pair(QString("Canceled"), nullptr);
}
