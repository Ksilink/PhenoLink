
#include <QtCore>
#ifdef WIN32
#include <QApplication>
#endif

#include <QtNetwork>
#include <QDebug>

#include <fstream>
#include <iostream>

#include "Core/pluginmanager.h"
#include "Core/networkprocesshandler.h"

#include <Core/checkoutprocessplugininterface.h>
#include <Core/checkoutprocess.h>

#include "Core/config.h"
#include <Core/wellplatemodel.h>

#ifdef WIN32
#include <windows.h>
#include <wincon.h>
#endif

#include "mainCli.h"
/* In Header */
#include <QLoggingCategory>


#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"


#include "checkout_arrow.h"

using namespace qhttp::server;

void help()
{
    qDebug() << "Checkout Command Line interface";
    qDebug() << "ckcli [options] [commands] [parameters]";
    qDebug() << "ckcli commands:";
    qDebug() << "\tls/list:  List available plugins";
    qDebug() << "\tls/list {plugin/name} Describe the plugin info";
    qDebug() << "\tds/describe platename: Load the plate for an overview";
    qDebug() << "\trun plugin/name [key=value] {list of plate names}: Run the plugin on the plates";
    qDebug() << "\tdump {output.json} plugin/name [key=value] [plate names]: Dumps the description of the run";
    qDebug() << "\tload {output.json} [start=0] [end=-1]: Load and run a description of a run, can be used to skip parts (start/end option)";
    qDebug() << "\tfuse project='projectName' commit=commitname: Refuse the plates in the commit name folder";
    qDebug() << "ckcli options:";
    qDebug() << "\tServer=IP:port : set server ip/port option";
    qApp->exit();
}




void helper::listParams(QJsonObject ob)
{
    std::cout << ob["Comment"].toString().toStdString() << std::endl;

    for (auto p: ob["Parameters"].toArray())
    {
        auto c = p.toObject();
        QString val = QString("\t%1 :\t%2 ").arg(c["Tag"].toString().replace(" ", ""), c["Comment"].toString());
        if (c.contains("Value"))
            val += c["Value"].toString();
        if (c.contains("Value1"))
            val += " " + c["Value1"].toString();
        if (c.contains("Value2"))
            val += " " + c["Value2"].toString();

        std::cout << val.toStdString() << std::endl;
    }

    if (proc.isEmpty())
        qApp->exit();
}

void helper::startProcess(QJsonObject ob)
{
    QRegExp siteMatcher;

    QJsonArray startParams;

    // Need to match the ob & this->params
    QJsonArray arr = ob["Parameters"].toArray(), resArray;
    for (auto par: arr)
    {
        QJsonObject pobj = par.toObject();
        QString op = pobj["Tag"].toString();
        op.replace(" ", "");

        for (auto s: this->params)
        {
            QStringList opt = s.split("=");

            if (op == opt.first())
            {
                //                qDebug() << s << op;
                pobj["Value"]=opt.at(1);
                if (opt.size() > 2)
                {

                    pobj["Value1"] = opt.at(1);
                    pobj["Value2"] = opt.at(2);
                }
            }
        }

        resArray.append(pobj);
    }
    ob["Parameters"]=resArray;
    ///    show
    listParams(ob); // need to check the value ?

    // Need to load & unroll the plates$
    QList<SequenceFileModel*> lsfm ;
    for (auto plate : plates)
    {
        auto efm = ScreensHandler::getHandler().loadScreen(plate);
        QList<SequenceFileModel*> t = efm->getAllSequenceFiles();
        foreach (SequenceFileModel* l, t)
            lsfm.push_back(l);
    }



    QJsonArray procArray;
    int StartId = 0;
    for (auto sfm: lsfm)
    {

        QList<bool> selectedChanns;
        for (auto p: sfm->getChannelNames())
            selectedChanns.push_back(true);

        QString imgType;
        QStringList metaData;

        bool asVectorImage = isVectorImageAndImageType(ob, imgType, metaData);


        QList<QJsonObject>  images =  sfm->toJSON(imgType, asVectorImage, selectedChanns, metaData, siteMatcher);
        foreach (QJsonObject im, images)
        {
            ob["shallDisplay"] = false;
            ob["ProcessStartId"] = StartId++;
            QJsonArray params = ob["Parameters"].toArray(), bias;

            for (int i = 0; i < params.size(); ++i )
            { // First set the images up, so that it will match the input data
                QJsonObject par = params[i].toObject();

                if (par.contains("ImageType"))
                {
                    foreach (QString j, im.keys())
                        par.insert(j, im[j]);

                    if (par["Channels"].isArray() && bias.isEmpty())
                    {
                        QJsonArray chs = par["Channels"].toArray();
                        for (int j = 0; j < chs.size(); ++j)
                        {
                            int channel = chs[j].toInt();
                            QString bias_file = sfm->property(QString("ShadingCorrectionSource_ch%1").arg(channel));
                            bias.append(bias_file);
                        }
                    }


                    par["bias"] = bias;
                    params.replace(i, par);
                }

                QSettings set;
                if (par["isDbPath"].toBool())
                {
                    QString v = set.value("databaseDir", par["Default"].toString()).toString();
                    v=QString("%1/PROJECTS/%2/Checkout_Results/").arg(v).arg(sfm->getOwner()->property("project"));
                    par["Value"]=v;
                    params.replace(i, par);
                }


            }
            QJsonArray cchans;
            for (int i = 0; i < params.size(); ++i )
            {
                QJsonObject par = params[i].toObject();
                if (par["Channels"].isArray() && cchans.isEmpty())
                {
                    cchans = par["Channels"].toArray();
                }
            }
            ob["Parameters"] = params;

            params = ob["ReturnData"].toArray();
            for (int i = 0; i < params.size(); ++i )
            {
                QJsonObject par = params[i].toObject();

                if (par.contains("isOptional"))
                { // Do not reclaim images if we are running heavy duty informations!!!
                    par["optionalState"] = false;
                    ob["BatchRun"]=true;
                    params.replace(i, par);
                }
            }

            ob["ReturnData"] = params;
            ob["Pos"] = sfm->Pos();

            QByteArray arr;    arr += QCborValue::fromJsonValue(ob).toByteArray();//QJsonDocument(obj).toBinaryData();
            arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
            QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

            ob["CoreProcess_hash"] = QString(hash.toHex());
            ob["CommitName"] = commitName;
            if (sfm->getOwner())
                ob["XP"] = sfm->getOwner()->groupName() +"/"+sfm->getOwner()->name();
            ob["WellTags"] = sfm->getTags().join(";");


            procArray.append(ob);
        }

    }

    if (!dump.isEmpty())
    {
        qDebug() << procArray;
        // Save Json to file

        qApp->exit();
        exit(0);
    }

    //qApp->exit();
    if (true)
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

        NetworkProcessHandler::handler().startProcess(proc, procArray);
    }
}


void helper::setParams(QString proc, QString commit, QStringList params, QStringList plates)
{
    this->proc = proc;
    this->commitName = commit;
    this->params = params;
    this->plates = plates;
}

void helper::setDump(QString dumpfile)
{
    this->dump = dumpfile;
}

void helper::process(qhttp::server::QHttpRequest* req, qhttp::server::QHttpResponse* res)
{
    const QByteArray data = req->collectedData();
    QString urlpath = req->url().path(), query = req->url().query();

    if (urlpath.startsWith("/addData/"))
    { // Now let's do the fun part :)
        auto ob = QCborValue::fromCbor(data).toJsonValue().toArray();

        QString commit=urlpath.mid((int)strlen("/addData/"));

        //qDebug() << "Adding data" << ob;
        for (auto item: (ob))
        {
            auto oj = item.toObject();

            NetworkProcessHandler::handler().removeHash(oj["hash"].toString());

            bool finished = (0 == NetworkProcessHandler::handler().remainingProcess().size());
            auto hash = oj["DataHash"].toString();
            ScreensHandler::getHandler().addDataToDb(hash, commit, oj, false);
            if (finished)
            {
                ScreensHandler::getHandler().commitAll();
                qApp->exit();
            }
        }
    }

    QJsonObject ob;
    setHttpResponse(ob, res, !query.contains("json"));
}


void helper::setHttpResponse(QJsonObject ob, qhttp::server::QHttpResponse* res, bool binary  )
{
    QByteArray body =  binary ? QCborValue::fromJsonValue(ob).toCbor() :
                                QJsonDocument(ob).toJson();
    //res->addHeader("Connection", "keep-alive");

    if (binary)
        res->addHeader("Content-Type", "application/cbor");
    else
        res->addHeader("Content-Type", "application/json");

    res->addHeaderValue("Content-Length", body.length());
    res->setStatusCode(qhttp::ESTATUS_OK);
    res->end(body);
}

void dumpProcess(QString proc = QString())
{

    if (proc.isEmpty())
    {
        NetworkProcessHandler::handler().establishNetworkAvailability();

        while(NetworkProcessHandler::handler().getProcesses().isEmpty())
            qApp->processEvents();

        QStringList procs = NetworkProcessHandler::handler().getProcesses();
        foreach(QString path, procs)
        {
            qDebug() << path;
        }
        exit(0);
    }
    else
    {
        NetworkProcessHandler::handler().establishNetworkAvailability();
        while(NetworkProcessHandler::handler().getProcesses().isEmpty())
            qApp->processEvents();
        NetworkProcessHandler::handler().getParameters(proc);

        helper h;

        qApp->connect(&NetworkProcessHandler::handler(), SIGNAL(parametersReady(QJsonObject)),
                      &h, SLOT(listParams(QJsonObject)));

        qApp->exec();

    }


}

QString unquote(QString in)
{
    if (in[0]=='"' || in[0]=='\'')
        in.remove(0, 1);
    if (in[in.size()-1]=='"' || in[in.size()-1]=='\'')
        in.chop(0);
    return in;
}

QString find(QString mat, int ac, int p, char** av)
{
    for (int i = p; i < ac; ++i)
    {
        QString m(av[i]);
        if (m.startsWith(mat))
        {
            if (m.contains("=")) return unquote(m.split("=").last());
            if (i+1 < ac)
                return unquote(av[i+1]);
            else
                help();
        }

    }
    return QString();
}


void startServer(QCoreApplication* a,QProcess& server, QStringList var )
{
    QSettings set;
    if (var.contains("127.0.0.1") || var.contains("localhost"))
    {
        // Start the network worker for processes
        //        server.setProcessChannelMode(QProcess::MergedChannels);
        server.setStandardOutputFile(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() +"/CheckoutServer_log.txt");
        server.setWorkingDirectory(a->applicationDirPath());
        QString r = "CheckoutHttpServer.exe";

        server.setProgram(r);
        if (set.value("UserMode/Debug", false).toBool())
            server.setArguments(QStringList() << "-d");

        server.start();

        if (!server.waitForStarted())
            qDebug() << "Server not properly started" << server.errorString() << r;

        QThread::sleep(5);
    }

}

void showPlate(ExperimentFileModel* efm)
{
    qDebug() << "Channels" << efm->getChannelNames();
    auto sfm = efm->getFirstValidSequenceFiles();
    qDebug() << sfm.getZCount() << sfm.getTimePointCount() << sfm.getFieldCount() << sfm.getChannels();
    std::cout << "   ";
    for (unsigned c = 0; c < efm->getColCount(); ++c)
        std::cout << QString("%1 ").arg(c+1, 2).toStdString();

    std::cout << std::endl;
    for (unsigned r = 0; r < efm->getRowCount(); ++r)
    {
        std::cout << QString("%1 ").arg(QLatin1Char('A'+r), 2).toStdString();
        for (unsigned c = 0; c < efm->getColCount(); ++c)
        {
            if ((*efm)(r,c).getAllFiles().size())
                std::cout << QString("%1 ").arg((*efm)(r,c).getAllFiles().size(), 2).toStdString();
            else
                std::cout << " - ";
        }
        std::cout << std::endl;
    }
}




void startProcess(QString proc, QString commitName,  QStringList params, QStringList plates, QString dumpfile)
{

    NetworkProcessHandler::handler().establishNetworkAvailability();
    while(NetworkProcessHandler::handler().getProcesses().isEmpty())
        qApp->processEvents();
    NetworkProcessHandler::handler().getParameters(proc);

    helper *h = new helper();

    if (proc.endsWith("BirdView"))
        commitName = "BirdView";

    h->setParams(proc, commitName, params, plates);
    h->setDump(dumpfile);

    qApp->connect(&NetworkProcessHandler::handler(), &NetworkProcessHandler::parametersReady,
                  [h](QJsonObject o) { h->startProcess(o);  });

    qApp->exec();
    delete h;
}

int main(int ac, char** av)
{
    auto a = new QCoreApplication(ac, av);
    //    QApplication::setStyle(QStyleFactory::create("Plastique"));
    a->setApplicationName("Checkout");
    a->setApplicationVersion(CHECKOUT_VERSION);
    //    a.setApplicationDisplayName(QString("Checkout %1").arg(CHECKOUT_VERSION));
    a->setOrganizationDomain("WD");
    a->setOrganizationName("WD");

    /* After creating QApplication instance */
    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);


    QSettings set;

    QDir dir( QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
    //QDir dir("P:/DATABASES");
    dir.mkpath(dir.absolutePath() + "/databases/");
    dir.mkpath(dir.absolutePath());

    if (!set.contains("databaseDir"))
        set.setValue("databaseDir", QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/databases/");

    QStringList var = set.value("Server", QStringList() << "127.0.0.1").toStringList();


    if (ac < 2) help();

    for (int i = 1; i < ac; ++i)
    {
        QString item(av[i]);
        if (item.startsWith("Server=")) // Overload the configuraiton servers
        {
            var.clear();
            var << item.split("=").at(1);
        }
    }


    for (int i = 1; i < ac; ++i)
    {
        QString item(av[i]);

        if (item == "ls" || item == "list")
        {
            QProcess server;
            startServer(a, server, var);

            if (ac == 2)
                dumpProcess();
            else
                dumpProcess(QString(av[i+1]));
        }

        if (item == "ds" || item == "describe")
        {
            PluginManager::loadPlugins();
            // Find a platename ?

            ScreensHandler& handler = ScreensHandler::getHandler();
            QString project = find("project", ac, i+1, av);
            QString drive = find("drive", ac, i+1, av);

            for (int p = i+1; p < ac; ++p)
            {
                QString plate(av[p]);
                if (!plate.contains("="))
                {
                    QString file = handler.findPlate(plate, project.split(","), drive);
                    if (!file.isEmpty())
                    {
                        // Found a plate
                        auto efm = handler.loadScreen(file);
                        showPlate(efm);
                    }
                }
            }
            exit(0);
        }

        if (item == "run" || item == "dump")
        {
            QString dumpfile;

            PluginManager::loadPlugins();

            QProcess server;
            startServer(a, server, var);
            if (ac <= i+1) { help(); exit(0); }

            if (item == "dump")
            {
                dumpfile = av[i+1];
                i++;
            }

            QString process(av[i+1]), commit;


            QStringList pluginParams;
            int p = i+2;
            QString project = find("project", ac, p + 1, av);
            QString drive = find("drive", ac, p + 1, av);

            for (; p < ac; ++p){
                QString par(av[p]);
                if (par.startsWith("commitName="))
                {
                    commit = par.split("=").last();
                }
                else if (par.contains("="))
                {
                    pluginParams << par;
                }
                else
                    break;
            }

            QStringList plates;
            ScreensHandler& handler = ScreensHandler::getHandler();

            for (int l = p; l <ac; ++l)
            {
                QString v(av[l]);
                if (!v.contains("="))
                {
                    v = handler.findPlate(v, project.split(","), drive);
                    if (!v.isEmpty())
                        plates << v;
                }
            }
            if (!process.isEmpty() && !plates.isEmpty())
                startProcess(process, commit, pluginParams, plates, dumpfile);
        }


        if (item == "fuse")
        {
            QString project = find("project", ac, i + 1, av);
            QString commit = find("commit", ac, i + 1, av);

            QSettings set;

            QString dbP = set.value("databaseDir", "L:").toString();
#ifndef  WIN32
            if (dbP.contains(":"))
                dbP = QString("/mnt/shares/") + dbP.replace(":","");
#endif


            // FIXME:  Cloud solution adjustements needed here !

            dbP.replace("\\", "/").replace("//", "/");
            QString folder = QString("%1/PROJECTS/%2/Checkout_Results/%3").arg(dbP,
                                                                               project,
                                                                               commit);

            QDir dir(folder);

            //            qDebug() << "folder" << folder << dir << QString("*_[0-9]*[0-9][0-9][0-9][0-9].fth");
            QStringList files = dir.entryList(QStringList() << QString("*_[0-9]*[0-9][0-9][0-9][0-9].fth"), QDir::Files);
            //            qDebug() << files;

            QStringList plates;

            for (auto pl: files)
            {
                QString t(pl.left(pl.lastIndexOf("_")));
                if (!plates.contains(t))
                    plates << t;
            }
            qDebug() << plates;

            for (auto plate: plates)
            {

                QStringList files = dir.entryList(QStringList() << QString("%1_[0-9]*[0-9][0-9][0-9][0-9].fth").arg(plate), QDir::Files);


                QString concatenated = QString("%1/%2.fth").arg(folder,plate.replace("/",""));
                if (files.isEmpty())
                {
                    qDebug() << "Error fusing the data to generate" << concatenated;
                    qDebug() << QString("%4_[0-9]*[0-9][0-9][0-9][0-9].fth").arg(plate.replace("/", ""));
                } //C:/Users/NicolasWiestDaessle/Documents/CheckoutDB
                else
                {

                    if (QFile::exists(concatenated)) // In case the feather exists already add this for fusion at the end of the process since arrow fuse handles duplicates if will skip value if recomputed and keep non computed ones (for instance when redoing a well computation)
                    {
                        dir.rename(concatenated, concatenated + ".torm");
                        files << concatenated.split("/").last()+".torm";
                    }

                    fuseArrow(folder, files, concatenated,plate.replace("\\", "/").replace("/",""));
                }

            }
        }
    }

    delete a;
    return 0;
}





