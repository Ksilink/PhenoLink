#ifndef PROCESSSERVER_H
#define PROCESSSERVER_H

#include <QTcpServer>
#include <QTcpSocket>

#include "Core/Dll.h"

#ifdef CheckoutServerWithPython
#include "Python/checkoutpythonpluginshandler.h"
#endif

#ifdef WIN32

class Control;
#endif

//#define PROCESS_SERVER_THREADED
#ifdef PROCESS_SERVER_THREADED
class ProcessThread;
#endif


class DllServerExport ProcessServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ProcessServer(QObject *parent = 0);


protected:
    void incomingConnection(qintptr socketDescriptor);

public slots:
  void exit_func();

  void disconnect();
  void ready();
signals:
  void error(QTcpSocket::SocketError socketError);
  void exit_signal();

protected:
  void replyList(QTcpSocket *tcpSocket); // reply with the processes list
  void replyProcess(QTcpSocket* tcpSocket, QString str); // send the process description to the client
  void startProcess(QTcpSocket* tcpSocket, QString proc, QJsonArray *ob); // reply with the processes list
  void updateProcessMessage(QTcpSocket* tcpSocket, QStringList hash);

  void exit_server();

#ifdef WIN32
protected:
   Control* _control;
public:
   Control* getControl() { return _control; }
#else
public:

   void setDriveMap(QString map);   

#endif

   QMap<QTcpSocket*, uint> clients;


#ifdef CheckoutServerWithPython
   CheckoutPythonPluginsHandler _python;
#endif

#ifdef PROCESS_SERVER_THREADED
   QList<ProcessThread*> threads;
#endif

};

#endif // PROCESSSERVER_H
