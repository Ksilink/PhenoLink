#ifndef ZMQTHREAD_H
#define ZMQTHREAD_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QMap>

#include <QThreadPool>
#include <QDebug>

#include <zmq/mdwrkapi.hpp>
#include <Dll.h>


class CheckoutProcessPluginInterface;
class CheckoutHttpClient;
struct RunnerThread;

class  ZMQThread : public QThread
                  // public QObject
{
     Q_OBJECT

    GlobParams& global_parameters;
    // QThreadPool worker_threadpool;
    // QThreadPool save_threadpool;
    bool verbose;
    QString proxy;
    QString drive_map;
    QThread* mainThread;
    mdwrk session;

    QList<RunnerThread*> to_join;


    QMap<QString, CheckoutProcessPluginInterface*> _plugins;
public:

    ZMQThread(GlobParams& gp, QThread* parentThread, QString prx, QString dmap, bool ver);

    void operator()();

    void run() override;
    // void run();

    void startProcessServer(QString process, QJsonArray array);



protected:
    QList<CheckoutHttpClient*> alive_replies;

    void save_and_send_binary(QJsonObject *ob);
};




#endif // ZMQTHREAD_H
