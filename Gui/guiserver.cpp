
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



#include <QMessageBox>

using namespace qhttp::server;

GuiServer::GuiServer(MainWindow* par): win(par)
{
    connect(this, &QHttpServer::newConnection,
            [this](QHttpConnection* ){
        Q_UNUSED(this);
        //        qDebug() << "Connection to GuiServer ! ";
        //            << c->tcpSocket()->errorString();
    });

    connect(this, &GuiServer::reply,
            this, &GuiServer::process
            );

    quint16 port = 8020;

    bool isListening = listen(QString::number(port),
                              [this](qhttp::server::QHttpRequest* req, qhttp::server::QHttpResponse* res){
            //            qDebug() << "Listenning to socket!";
            req->collectData();
            req->onEnd([this, req, res](){
                    this->processSig(req, res);
                    });
            });

    if ( !isListening ) {
        // should tell the user !!!

        qDebug() << "can not listen on" <<  port;
    }
    //    else
    //        qDebug() << "Gui Server Started";
}

void GuiServer::processSig(qhttp::server::QHttpRequest* req, qhttp::server::QHttpResponse* res)
{
    emit reply(req, res);
}

inline QString stringIP(quint32 ip)
{
    return QString("%4.%3.%2.%1").arg(ip & 0xFF)
        .arg((ip >> 8) & 0xFF)
        .arg((ip >> 16) & 0xFF)
        .arg((ip >> 24) & 0xFF);
}


void GuiServer::process(qhttp::server::QHttpRequest* req, qhttp::server::QHttpResponse* res)
{

    static QMutex mutex;
    if (!req || !res)
        return;
    const QByteArray data = req->collectedData();
    QString urlpath = req->url().path(), query = req->url().query();



    //    qDebug() << qhttp::Stringify::toString(req->method())
    //             << qPrintable(urlpath)
    //             << qPrintable(query)
    //             << data.size();


    // addData/<project>/<CommitName>/<Plate> <= POST data as CBOR
    // CBOR data should look alike:
    // [ { "hash": "process hash value", "Well": "","timepoint":"","fieldId":"","sliceId":"","channel":"","tags":"", "res names": "", ... },
    // all rows that are ready for this push
    //   {} ]
    if (urlpath.startsWith("/addData/"))
    { // Now let's do the fun part :)
        auto ob = QCborValue::fromCbor(data).toJsonValue().toArray();

        QString commit=urlpath.mid((int)strlen("/addData/"));

        QStringList hashes;
        for (auto item : (ob))
            hashes << item.toObject()["hash"].toString();
        mutex.lock();
        NetworkProcessHandler::handler().removeHash(hashes);
        mutex.unlock();
        qDebug() << "Removing " << hashes.size();
        QString refIP = stringIP(req->connection()->tcpSocket()->peerAddress().toIPv4Address());

        //qDebug() << "Client Adding data" << refIP << NetworkProcessHandler::handler().remainingProcess().size();
        mutex.lock();

        for (auto item : (ob))
        {
            auto oj = item.toObject();
            //qDebug() << oj;
            bool finished = (0 == NetworkProcessHandler::handler().remainingProcess().size());
            auto hash = oj["DataHash"].toString();
//            auto hash = oj["DataHash"].toString();

            qDebug() << "Process finished" << hash << oj["DataHash"].toString() << finished << NetworkProcessHandler::handler().remainingProcess().size();
            auto mdl = ScreensHandler::getHandler().addDataToDb(hash, commit, oj, false);

            if (mdl && finished)
            {

                ScreensHandler::getHandler().commitAll();
                if (mdl->computedDataModel())
                    mdl->computedDataModel()->resyncmodel();
                win->updateTableView(mdl);
                win->on_wellPlateViewTab_tabBarClicked(-1);
            }
        }

        mutex.unlock();
    }


    else if (urlpath.startsWith("/addImage/"))
    { // Now let's do the fun part :)
        auto ob = QCborValue::fromCbor(data);

//        qDebug() << "Add Image" << ob.toMap().value("ProcessStartId");

        mutex.lock();
        QList<SequenceFileModel*> lsfm;
        lsfm << ScreensHandler::getHandler().addProcessResultImage(ob);

        win->networkRetrievedImage(lsfm);
        mutex.unlock();
    }

    else if (urlpath.startsWith("/Message"))
    {
        QString message(data);
        QMessageBox::information(win, "Remote Message", message);
    }

    else if (urlpath.startsWith("/Load"))
    {
        //        "/Load/?plate=&wells=&field=&tile=&unpacked"
        QStringList queries = query.split("&"), wells, plates;
        //        qDebug() << "Load" << queries;
        bool unpacked=false;
        QStringList pars = QStringList() << "field" << "time" << "zpos" << "tile" << "project" << "drive";
//        QString tile;
        std::map<QString, QString> params;

        for (auto q : (queries) )
        {
            if (q.startsWith("plate="))
            {
                q=q.mid(6);
                plates=q.replace("\\", "/").split(",");
                if (plates.isEmpty())
                {
                    qDebug() << "Returning due to empty plate" << plates << q;
                    return;
                }
            }
            if (q.startsWith("wells="))
            {
                q=q.mid(6);
                wells=q.split(",");
            }
            if(q.startsWith("unpacked"))
            {
                unpacked=true;
            }
            for (auto p : (pars))
                if (q.startsWith(p))
                {
                    q=q.mid(p.size()+1);
                    if (q.size() > 0)
                        params[p]=q;
                }
        }

        //        qDebug() << "Should handle" << plates << unpacked << wells;

        Screens sc;

        for (auto &_plate: (plates) )
        {
            QStringList lpl = _plate.split(":");
            QString plate = lpl[0];

            for (auto &xp: ScreensHandler::getHandler().getScreens())
            {
                    if (xp->fileName().contains(plate))
                        sc << xp;
            }

            if (sc.isEmpty())
            {
                if (plate[1]==':')
                {
                    sc = win->loadSelection(QStringList() << plate, false);
                }
                else
                {
                    QStringList project = params["project"].split(",");// "project can be "," separated :D
                    QString drive = params["drive"];
                    sc= win->findPlate(plate, project, drive);
                }
            }

            for (auto& lmdl: ScreensHandler::getHandler().getScreens())
                lmdl->clearState(ExperimentFileModel::IsSelected);

            for (auto &mdl: (sc))
            {
                win->resetSelection();
                win->clearScreenSelection();
                mdl->clearState(ExperimentFileModel::IsSelected);
                ExperimentDataTableModel* xpmdl = mdl->computedDataModel();

                if (unpacked)
                {
                    QList<SequenceFileModel *>  l  = mdl->getAllSequenceFiles();

                    foreach(SequenceFileModel * mm, l)
                    {
                        mm->setProperties("unpack", "yes");
                        mm->checkValidity();
                    }
                }


               // win->displayWellSelection();
                if (wells.isEmpty() && lpl.size() >= 1)
                {
                    wells = lpl[1].split("/");  // so we can do a query plate:well1/well2/well3,plate:well1/well2 etc...
                }

                for (auto& po: (wells) )
                {
                    auto pos = xpmdl->stringToPos(po);
                    if ((*mdl)(pos).isValid())
                        mdl->select(pos, true);
                }

                for (auto &sfm : mdl->getSelection())
                    if (sfm->isValid())
                    {
                        auto inter = win->getInteractor(sfm);
                        if (inter)
                        {
                            if (params.find("field") != params.end())
                                inter->setField(params["field"].toInt());

                            if (params.find("zpos") != params.end())
                                inter->setZ(params["zpos"].toInt());

                            if (params.find("time") != params.end())
                                inter->setTimePoint(params["time"].toInt());

                            if (params.find("tile") != params.end())
                            {
                                inter->setOverlayId("Tile", params["tile"].toInt());
                                inter->toggleOverlay("Tile", true);
                            }
                        }
                    }
                win->displayWellSelection();
            }

        }

    }


    QJsonObject ob;
    setHttpResponse(ob, res, !query.contains("json"));
}



void GuiServer::setHttpResponse(QJsonObject ob, qhttp::server::QHttpResponse* res, bool binary  )
{
    QByteArray body =  binary ? QCborValue::fromJsonValue(ob).toCbor() :
                                QJsonDocument(ob).toJson();
    res->addHeader("Connection", "close");//keep-alive");

    if (binary)
        res->addHeader("Content-Type", "application/cbor");
    else
        res->addHeader("Content-Type", "application/json");

    res->addHeader("Content-Length", QString::number(body.length()).toLatin1());
    res->setStatusCode(qhttp::ESTATUS_OK);
    res->end(body);
}
