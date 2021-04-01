#ifndef CHECKOUTHTTPSERVER_H
#define CHECKOUTHTTPSERVER_H



#include <QtCore>
#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"


using namespace qhttp::server;

struct Server : public QHttpServer
{
    Q_OBJECT
public:
    using QHttpServer::QHttpServer;

    int start(quint16 port) ;
    void setHttpResponse(QJsonObject& ob, QHttpResponse* res, bool binary = true);
    void process(QHttpRequest* req, QHttpResponse* res);

public slots:
    void finished(QString hash, QJsonObject ob);
};



#endif // CHECKOUTHTTPSERVER_H
