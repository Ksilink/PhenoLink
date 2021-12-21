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

private:
    void process(qhttp::server::QHttpRequest *req, qhttp::server::QHttpResponse *res);
    void setHttpResponse(QJsonObject ob, qhttp::server::QHttpResponse *res, bool binary = true);

protected:
    MainWindow* win;
};

#endif // GUISERVER_H
