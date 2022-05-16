#ifndef CHECKOUTHTTPSERVER_H
#define CHECKOUTHTTPSERVER_H



#include <QtCore>
#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"

#ifdef WIN32
#include <QWidget>

#include <QSystemTrayIcon>
#include <QMenu>

#include <Core/config.h>
#include <core/checkoutprocess.h>



struct Server;

class Control: public QWidget
{
    Q_OBJECT
public:
    Control(Server* serv);

    void showMessage(QString title, QString content)
    {
        trayIcon->showMessage(title, content);
    }

    virtual void timerEvent(QTimerEvent * event);


public slots:

    void quit()
    {
        trayIcon->showMessage("Stopping Checkout Server", "Application requested checkout server exit");
        qApp->quit();
    }

    void cancelUser()
    {
        // Play with QSender grab name & call QProcess Cancel func.
        QAction* origin = qobject_cast<QAction*>(sender());
        if (!origin) return;
        QString user = origin->text();
        CheckoutProcess::handler().cancelUser(user);
    }

protected:
    QAction* quitAction;
    QMenu*   _cancelMenu;
    QList<QAction*> _users;

    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    Server* _serv;
    int lastNpro;

};
#endif

using namespace qhttp::server;
struct CheckoutHttpClient;

struct Server : public QHttpServer
{
    Q_OBJECT
public:
    using QHttpServer::QHttpServer;

    Server();

    int start(quint16 port) ;
    void setHttpResponse(QJsonObject& ob, qhttp::server::QHttpResponse* res, bool binary = true);

    void process(qhttp::server::QHttpRequest* req, qhttp::server::QHttpResponse* res);
    uint serverPort();
    void setPort(uint port) {dport = port;};

public slots:
    void finished(QString hash, QJsonObject ob);
    void exit_func();

#ifdef WIN32
protected:
   Control* _control;
public:
   Control* getControl() { return _control; }
#else
public:

   void setDriveMap(QString map);
#endif

protected:
   void HTMLstatus(qhttp::server::QHttpResponse *res, QString mesg=QString());
   void WorkerMonitor();


   QString pickWorker();
   QQueue<QJsonObject> &getHighestPriorityJob(QString server);


   QStringList pendingTasks();

   unsigned int njobs();
   unsigned int nbUsers();
    //
    unsigned dport;

    // We need to maintain a worker list
    QQueue<QString > workers;
    QSet<QString> rmWorkers;


    // Who's connected
    QMap<QString, int> workers_status;

    // We need a priority queue of processes
    QMap<QString, QMap< int, QQueue<QJsonObject> > > jobs;

    // Pending
    QMap<QString, QJsonObject > running;

    QMap<QString, unsigned int> run_time, run_count;
    // Project Affinity map
    QMap<QString, QString> project_affinity; // projection of project name to server name


    QSet<QString> proc_list;
    QMap<QString, QMap<QString, QJsonObject> >  proc_params; // Process name, server name => Proc descr


    CheckoutHttpClient* client;
};



#endif // CHECKOUTHTTPSERVER_H
