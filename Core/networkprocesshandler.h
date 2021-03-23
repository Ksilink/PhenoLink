#ifndef NETWORKPROCESSHANDLER_H
#define NETWORKPROCESSHANDLER_H


#include <QtCore>
#include <QtNetwork>

#include "Dll.h"



extern  QTextStream *hash_logfile;

//struct CheckoutHost
//{
//    QHostAddress address;
//    quint16      port;
//};
#include "qhttp/qhttpclient.hpp"
#include "qhttp/qhttpclientrequest.hpp"
#include "qhttp/qhttpclientresponse.hpp"

using namespace qhttp::client;



struct DllCoreExport CheckoutHttpClient: public QObject
{
    Q_OBJECT

public:
    CheckoutHttpClient(QString host, quint16 port);
    void send() ;
    void onIncomingData(const QByteArray& data) ;
    void finalize();

public:
    QUrl         iurl;
    QHttpClient  iclient;
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
    void startProcess(QString process, QJsonArray ob);
    void startProcess(CheckoutHost* h, QString process, QJsonArray ob);

    void getProcessMessageStatus(QString process, QList<QString> hash);

    void queryPayload(QString hash);

    void deletePayload(QString hash);
    void processFinished(QStringList);
    void removeHash(QString hash);

    void exitServer();

    unsigned errors() { return _error_list.size(); }

    QList<QPair<QString, QJsonArray> > erroneousProcesses();

    void readResponse(QTcpSocket *tcpSocket);

    QStringList remainingProcess();
protected:
    void writeInitialQuery(QTcpSocket *soc);

    QTcpSocket* getNewSocket(CheckoutHost *h, bool with_sig=true);

    void handleMessageProcesses(QString& keyword, QDataStream& in, QTcpSocket *tcpSocket);
    void handleMessageProcessDetails(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);
    void handleMessageProcessStart(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);
    void handleMessageProcessStatus(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);
    void handleMessageProcessPayload(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);

    void handleMessageDeletePayload(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);


private slots:
    void readResponse();
    void displayError(QAbstractSocket::SocketError socketError);
    void watcherProcessStatusFinished();

signals:
    void newProcessList();
    void parametersReady(QJsonObject);
    void processStarted(QString, QString);
    void updateProcessStatusMessage(QJsonArray);
    void payloadAvailable(QString hash);


protected:
    // Allows to link a process with a network connection
    QList<CheckoutHttpClient*> activeHosts;

    QMap<QString, QList<CheckoutHttpClient*> > procMapper;

    QMap<QString, CheckoutHttpClient*> runningProcs;


//    QList<QTcpSocket*> activeProcess;
//    QMap<QTcpSocket*, uint > _blockSize;

    QList<QPair<CheckoutHttpClient* , QPair<QString, QJsonArray> > > _error_list; // Keep track of the errors from start process

    bool _waiting_Update;


    int last_serv_pos;
    QFile* data;
};

#endif // NETWORKPROCESSHANDLER_H
