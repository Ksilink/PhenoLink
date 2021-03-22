
#include <QtCore>
#ifdef WIN32
#include <QApplication>
#endif

#include <QtNetwork>
#include <QDebug>

#include <fstream>
#include <iostream>


#include "Core/pluginmanager.h"

#include <Core/checkoutprocessplugininterface.h>

#include <Core/checkoutprocess.h>

#include "Core/config.h"

#ifdef WIN32
#include <windows.h>
#include <wincon.h>
#endif

#include <QtConcurrent>

#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"


// addData/<project>/<CommitName>/<Plate> <= POST data as CBOR
// CBOR data should look alike:
// [ { "hash": "process hash value", "Well": "","timepoint":"","fieldId":"","sliceId":"","channel":"","tags":"", "res names": "", ... },
// all rows that are ready for this push
//   {} ]
// When all "hash" is finished
// ListProcesses
// Process/<ProcPath>
//

using namespace qhttp::server;

struct Server : public QHttpServer
{
    using QHttpServer::QHttpServer;

    int start(quint16 port) {
        connect(this, &QHttpServer::newConnection, [this](QHttpConnection*){
            Q_UNUSED(this);
            printf("a new connection has occured!\n");
        });

        bool isListening = listen(
                    QString::number(port),
                    [this](QHttpRequest* req, QHttpResponse* res){
                req->collectData();
                req->onEnd([this, req, res](){
            process(req, res);
        });
    });

        if ( !isListening ) {
            qDebug("can not listen on %d!\n", port);
            return -1;
        }

        return qApp->exec();
    }

    void setHttpResponse(QJsonObject& ob, QHttpResponse* res, bool binary = true)
    {
        QByteArray body =  binary ? QCborValue::fromJsonValue(ob).toCbor() :
                                    QJsonDocument(ob).toJson();
        res->addHeader("Connection", "keep-alive");

        if (binary)
            res->addHeader("Content-Type", "application/cbor");
        else
            res->addHeader("Content-Type", "application/json");

        res->addHeaderValue("Content-Length", body.length());
        res->setStatusCode(qhttp::ESTATUS_OK);
        res->end(body);
    }

    void process(QHttpRequest* req, QHttpResponse* res)
    {

        const QByteArray data = req->collectedData();

        qDebug() << qhttp::Stringify::toString(req->method())
                 << qPrintable(req->url().path())
                 << qPrintable(req->url().query())
                 << data.size();

        QString urlpath = req->url().path(), query = req->url().query();
        CheckoutProcess& procs = CheckoutProcess::handler();

        if (urlpath == "/ListProcesses")
        {

            QStringList prcs = procs.pluginPaths();
            int processor_count = (int)std::thread::hardware_concurrency();

            QJsonObject ob;

            ob["CPU"] = processor_count;
            ob["Processes"] = QJsonArray::fromStringList(prcs);

            setHttpResponse(ob, res, !query.contains("json"));
            return;
        }

        if (urlpath.startsWith("/Process/"))
        {
            QString path = urlpath.mid(9);
            //            qDebug() << path;
            QJsonObject ob;
            procs.getParameters(path, ob);
            setHttpResponse(ob, res, !query.contains("json"));
        }


        if (urlpath.startsWith("/Start/"))
        {
            QString proc = urlpath.mid(7);

            auto ob = QCborValue::fromCbor(data).toJsonValue().toArray();
            QJsonArray Core,Run;

            for (int i = 0; i < ob.size(); ++i)
            {
                QJsonObject obj = ob.at(i).toObject();

                QByteArray arr = QCborValue::fromJsonValue(obj).toCbor();
                arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
                QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

                Core.append(obj["CoreProcess_hash"]);
                QString sHash = hash.toHex();
                Run.append(QString(sHash));
                obj["Process_hash"] = sHash;
                if (req->remoteAddress() ==
                        req->connection()->tcpSocket()->localAddress().toString())
                    obj["LocalRun"] = true;

                ob.replace(i, obj);
            }

            QJsonObject obj;
            obj["ArrayCoreProcess"] = Core;
            obj["ArrayRunProcess"] = Run;

            // Response is obj

            setHttpResponse(obj, res, !query.contains("json"));
            QtConcurrent::run(&procs, &CheckoutProcess::startProcessServer,
                              proc, ob);

        }

        if (data.size())
        {
            auto root = QJsonDocument::fromJson(data).object();

            if ( root.isEmpty()  ||  root.value("name").toString() != QLatin1String("add") ) {
                const static char KMessage[] = "Invalid json format!";
                res->setStatusCode(qhttp::ESTATUS_BAD_REQUEST);
                res->addHeader("connection", "close");
                res->addHeaderValue("content-length", strlen(KMessage));
                res->end(KMessage);
                return;
            }

            int total = 0;
            auto args = root.value("args").toArray();
            for ( const auto jv : args ) {
                total += jv.toInt();
            }
            root["args"] = total;

            QByteArray body = QJsonDocument(root).toJson();
            res->addHeader("connection", "keep-alive");
            res->addHeaderValue("content-length", body.length());
            res->setStatusCode(qhttp::ESTATUS_OK);
            res->end(body);
        }
        else
        {
            QString body("Server Query received, with empty content");
            res->addHeader("connection", "close");
            res->addHeaderValue("content-length", body.length());
            res->setStatusCode(qhttp::ESTATUS_OK);
            res->end(body.toLatin1());
        }
    }
};



int main(int ac, char** av)
{
#ifdef WIN32
    QApplication app(ac, av);
#else
    QCoreApplication app(ac, av);
#endif

    app.setApplicationName("Checkout");
    app.setApplicationVersion(CHECKOUT_VERSION);
    app.setOrganizationDomain("WD");
    app.setOrganizationName("WD");


    PluginManager::loadPlugins(true);


    QString portOrUnixSocket("10022"); // default: TCP port 10022
    if ( ac > 1 )
        portOrUnixSocket = av[1];

    Server server;


    return server.start(portOrUnixSocket.toUInt());
}
