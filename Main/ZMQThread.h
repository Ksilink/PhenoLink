#ifndef ZMQTHREAD_H
#define ZMQTHREAD_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QMap>

#include <QDebug>

#include <mdwrkapi.hpp>



class CheckoutProcessPluginInterface;

class ZMQThread : public QThread
{
    Q_OBJECT

    GlobParams& global_parameters;
    bool verbose;
    QString proxy;
    QString drive_map;
    QThread* mainThread;
    mdwrk session;

    QMap<QString, CheckoutProcessPluginInterface*> _plugins;
public:

    ZMQThread(GlobParams& gp, QThread* parentThread, QString prx, QString dmap, bool ver): verbose(ver),
        global_parameters(gp),
        proxy(prx), drive_map(dmap),
        mainThread(parentThread), session(QString("tcp://%1").arg(proxy), "processes", gp, verbose)
    {

    }

    void run() override;

    void startProcessServer(QString process, QJsonArray array);

    void thread_finished();

};



#endif // ZMQTHREAD_H
