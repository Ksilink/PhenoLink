#ifndef PROCESSTHREAD_H
#define PROCESSTHREAD_H

#include "Core/Dll.h"
#include <QThread>
#include <QTcpSocket>
#include <QHostAddress>

class DllServerExport ProcessThread : public QThread
{
    Q_OBJECT

public:
    ProcessThread(qintptr socketDescriptor, QObject *parent);
    void run();


protected slots:
    void disconnect();
    void ready();
signals:
    void error(QTcpSocket::SocketError socketError);
    void exit_signal();

protected:

    void replyList(); // reply with the processes list
    void replyProcess(QString str); // send the process description to the client
    void startProcess(QString proc, QJsonArray ob); // reply with the processes list
    void updateProcessMessage(QStringList hash);

    void exit_server();
private:
    qintptr socketDescriptor;
    uint blockSize;

    QTcpSocket* tcpSocket;

};


#endif // PROCESSTHREAD_H
