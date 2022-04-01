#ifndef GUISERVER_H
#define GUISERVER_H

#include <QObject>
#include <QtCore>
#include <QtConcurrent>

#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"

class MainWindow;

using namespace qhttp::server;

class GuiServer : public QHttpServer
{
    Q_OBJECT
public:
    GuiServer(MainWindow* parent);
    void processSig(qhttp::server::QHttpRequest *req, qhttp::server::QHttpResponse *res);

private:
    void setHttpResponse(QJsonObject ob, qhttp::server::QHttpResponse *res, bool binary = true);

signals:
    void reply(qhttp::server::QHttpRequest *req, qhttp::server::QHttpResponse *res);
public slots:
    void process(qhttp::server::QHttpRequest *req, qhttp::server::QHttpResponse *res);


protected:
    MainWindow* win;
};

#endif // GUISERVER_H
