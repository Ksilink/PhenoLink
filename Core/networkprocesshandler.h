#ifndef NETWORKPROCESSHANDLER_H
#define NETWORKPROCESSHANDLER_H


#include <QtCore>
#include <QtNetwork>

#include "Dll.h"
#ifndef QHTTP_HAS_CLIENT
#define QHTTP_HAS_CLIENT
#endif

extern  QTextStream *hash_logfile;

//struct CheckoutHost
//{
//    QHostAddress address;
//    quint16      port;
//};

#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"
#include "qhttp/qhttpclient.hpp"
#include "qhttp/qhttpclientrequest.hpp"
#include "qhttp/qhttpclientresponse.hpp"

using namespace qhttp::client;

struct Req
{
    QUrl url;
    QByteArray data;
    bool keepalive;

    Req(QUrl u, QByteArray d, bool ka = false):
        url(u), data(d), keepalive(ka)
    {

    }
};



struct DllCoreExport CheckoutHttpClient: public QObject
{
    Q_OBJECT

public:

    CheckoutHttpClient(QString host, quint16 port);
    ~CheckoutHttpClient();
    void send(QString path, QString query=QString());
    void send(QString path, QString query, QJsonArray ob = QJsonArray(), bool keepalive = false) ;
    void send(QString path, QString query, QByteArray ob = QByteArray(), bool keepalive = false) ;
    void onIncomingData(const QByteArray& data) ;
    void finalize();

    void sendQueue();
protected:
    void timerEvent(QTimerEvent *event) override;
public:
    QQueue<Req> reqs;
    QUrl         iurl;
    QHttpClient  iclient;
    bool         awaiting;
    int          icpus;
    int          procs_counter;
};


// faking :)
typedef CheckoutHttpClient CheckoutHost;


class DllCoreExport NetworkProcessHandler: public QObject
{
    Q_OBJECT
public:
    NetworkProcessHandler();
    ~NetworkProcessHandler();

    static NetworkProcessHandler& handler();
    void establishNetworkAvailability();

    QStringList getProcesses();
    void getParameters(QString process);
    void setParameters(QJsonObject ob);

    void startProcess(QString process, QJsonArray ob);
    void startProcess(CheckoutHttpClient *h, QString process, QJsonArray ob);

    void getProcessMessageStatus(QString process, QList<QString> hash);

    void finishedProcess(QString hash, QJsonObject res);

    void removeHash(QString hash);

    void exitServer();

    unsigned errors() { return _error_list.size(); }

    QList<QPair<QString, QJsonArray> > erroneousProcesses();


    QStringList remainingProcess();
    void setProcesses(QJsonArray ar, CheckoutHttpClient* cl);
    void handleHashMapping(QJsonArray Core, QJsonArray Run);

private slots:
    void displayError(QAbstractSocket::SocketError socketError);
    void watcherProcessStatusFinished();

signals:
    void newProcessList();
    void parametersReady(QJsonObject);
    void processStarted(QString, QString);
    void updateProcessStatusMessage(QJsonArray);
    void payloadAvailable(QString hash);
    void finishedJob();

protected:
    // Allows to link a process with a network connection
    QList<CheckoutHttpClient*> activeHosts;

    QMap<QString, QList<CheckoutHttpClient*> > procMapper;

    QMap<QString, CheckoutHttpClient*> runningProcs;

    QList<CheckoutHttpClient*> alive_replies;

    QList<QPair<CheckoutHttpClient* , QPair<QString, QJsonArray> > > _error_list; // Keep track of the errors from start process

    bool _waiting_Update;

    int last_serv_pos;
    QFile* data;
};

#endif // NETWORKPROCESSHANDLER_H
