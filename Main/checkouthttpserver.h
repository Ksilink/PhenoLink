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

struct Server : public QHttpServer
{
    Q_OBJECT
public:
    using QHttpServer::QHttpServer;

    Server();

    int start(quint16 port) ;
    void setHttpResponse(QJsonObject& ob, QHttpResponse* res, bool binary = true);
    void process(QHttpRequest* req, QHttpResponse* res);
    uint serverPort();

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

};



#endif // CHECKOUTHTTPSERVER_H
