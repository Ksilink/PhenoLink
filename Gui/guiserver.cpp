
#include "guiserver.h"

#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"

#include <QTcpSocket>
#include "Core/networkprocesshandler.h"
#include "mainwindow.h"

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

GuiServer::GuiServer(MainWindow* par): win(par)
{
    connect(this, &QHttpServer::newConnection,
            [this](QHttpConnection* ){
        Q_UNUSED(this);
//        qDebug() << "Connection to GuiServer ! ";
//            << c->tcpSocket()->errorString();
    });
    quint16 port = 8020;

    bool isListening = listen(QString::number(port),
                              [this](qhttp::server::QHttpRequest* req, qhttp::server::QHttpResponse* res){
//            qDebug() << "Listenning to socket!";
            req->collectData();
            req->onEnd([this, req, res](){
        this->process(req, res);
    });
});


    if ( !isListening ) {
        qDebug() << "can not listen on" <<  port;
    }
//    else
//        qDebug() << "Gui Server Started";
}

void GuiServer::process(qhttp::server::QHttpRequest* req, qhttp::server::QHttpResponse* res)
{
    const QByteArray data = req->collectedData();
    QString urlpath = req->url().path(), query = req->url().query();

    qDebug() << qhttp::Stringify::toString(req->method())
             << qPrintable(urlpath)
             << qPrintable(query)
             << data.size();


    // addData/<project>/<CommitName>/<Plate> <= POST data as CBOR
    // CBOR data should look alike:
    // [ { "hash": "process hash value", "Well": "","timepoint":"","fieldId":"","sliceId":"","channel":"","tags":"", "res names": "", ... },
    // all rows that are ready for this push
    //   {} ]
    if (urlpath.startsWith("/addData/"))
    { // Now let's do the fun part :)
        auto ob = QCborValue::fromCbor(data).toJsonValue().toArray();

        QString commit=urlpath.mid((int)strlen("/addData/"));

        //qDebug() << "Adding data" << commit;
        for (auto item: ob)
        {
            auto oj = item.toObject();

            NetworkProcessHandler::handler().removeHash(oj["hash"].toString());

            bool finished = (0 == NetworkProcessHandler::handler().remainingProcess().size());
            auto hash = oj["DataHash"].toString();
            ScreensHandler::getHandler().addDataToDb(hash, commit, oj, false);
            qDebug() << "Process finished" << finished << commit;
            if (finished)
                ScreensHandler::getHandler().commitAll();
        }
    }


    if (urlpath.startsWith("/addImage/"))
    { // Now let's do the fun part :)
        auto ob = QCborValue::fromCbor(data);

        QList<SequenceFileModel*> lsfm;
        lsfm << ScreensHandler::getHandler().addProcessResultImage(ob);

        win->networkRetrievedImage(lsfm);

    }

    QJsonObject ob;
    setHttpResponse(ob, res, !query.contains("json"));
}



void GuiServer::setHttpResponse(QJsonObject& ob, qhttp::server::QHttpResponse* res, bool binary  )
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
