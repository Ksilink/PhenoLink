
#include <QtCore>
#ifdef WIN32
#include <QApplication>
#endif

#include <QtNetwork>
#include <QDebug>

#include <fstream>
#include <iostream>

#include "Core/pluginmanager.h"
//#include "Core/networkprocesshandler.h"

#include <Core/checkoutprocessplugininterface.h>
#include <Core/checkoutprocess.h>

#include "Core/config.h"
#include <Core/wellplatemodel.h>

#ifdef WIN32
#include <windows.h>
#include <wincon.h>
#endif

#include "mainzCli.h"
/* In Header */
#include <QLoggingCategory>

#include <zmq/mdcliapi.hpp>


#include "checkout_arrow.h"

void help()
{
    qDebug() << "PhenoLink Command Line interface";
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
    qDebug() << "\t-w : wait for process to finish (default is fire & forget mode)";
    qApp->exit();
}




void listParams(QJsonObject ob)
{

    std::cout << ob["Path"].toString().toStdString() << std::endl;
    std::cout << ob["Comment"].toString().toStdString() << std::endl;

    for (auto p: ob["Parameters"].toArray())
    {
        auto c = p.toObject();

        if (c["isImage"].toBool(false)) continue;

        QString val = QString("\t%1 :\t%2\r\n").arg(c["Tag"].toString(), c["Comment"].toString());
//        qDebug() << c;
        if (c.contains("Enum"))
        {
            val += "Options: [";
            for (auto x : c["Enum"].toArray())
                val += x.toString() + ",";
            val.chop(1);
            val += "]";
        }
        if (c.contains("Value"))
            val += c["Value"].toString();
        if (c.contains("Value1"))
            val += " " + c["Value1"].toString();
        if (c.contains("Value2"))
            val += " " + c["Value2"].toString();


        std::cout << val.toStdString() << std::endl;
    }

//    if (proc.isEmpty())
//        qApp->exit();
}

QJsonArray helper::setupProcess(QJsonObject ob, QRegularExpression siteMatcher, QString& project)
{
//    QReguExp siteMatcher;

    QSettings set;

    qDebug() << "Setting json object:" << ob;
    ob.remove("Comment");


    QJsonArray startParams;

    // Need to match the ob & this->params
    QJsonArray arr = ob["Parameters"].toArray(), resArray;
    for (auto par: arr)
    {
        QJsonObject pobj = par.toObject();
        QString op = pobj["Tag"].toString();
//        op.replace(" ", "");

        for (auto s: this->params)
        {
            QStringList opt = s.split("=");

            if (op == opt.first())
            {
                //                qDebug() << s << op;
                if (op.contains("Enum"))
                {
                    auto ar = pobj["Enum"].toArray();
                    for (int i = 0; i < ar.size(); ++i)
                        if (opt.at(1)==ar[i].toString())
                        {
                            opt.replace(1, QString::number(i));
                            break;
                        }
                }
                pobj["Value"]=opt.at(1);
                if (opt.size() > 2)
                {

                    pobj["Value1"] = opt.at(1);
                    pobj["Value2"] = opt.at(2);
                }
            }
        }
        QStringList rm = QStringList() << "Comment" << "Level" << "Enum" << "IsSync" << "Position" << "PerChannelParameter" << "isSlider";
        for (auto x : rm) if (pobj.contains(x))   pobj.remove(x);



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
                    v=QString("%1/PROJECTS/%2/Checkout_Results/").arg(v).arg(project);
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

            ob["Project"] = project.isEmpty() ? sfm->getOwner()->property("project") : project;


            ob["WellTags"] = sfm->getTags().join(";");


#ifdef WIN32
            QString username = qgetenv("USERNAME");
#else
            QString username = qgetenv("USER");
#endif

            username =   set.value("UserName", username).toString();

            ob["Username"] = username;
            ob["Computer"] = QHostInfo::localHostName();;

            procArray.append(ob);
        }

    }

    if (!dump.isEmpty())
    {
//        qDebug() << procArray;
        // Save Json to file
        QFile saveFile(dump);

        if (!saveFile.open(QIODevice::WriteOnly)) {
            qWarning("Couldn't open save file.");
            return QJsonArray();
        }

        auto data = QJsonDocument(procArray);
        saveFile.write(data.toJson());
        saveFile.close();

        exit(0);
    }

    return procArray;
}


void helper::setParams(QString proc, QString commit, QStringList params, QStringList plates, QString project)
{
    this->proc = proc;
    this->commitName = commit;
    this->params = params;
    this->plates = plates;
    this->project = project;
}

void helper::setDump(QString dumpfile)
{
    this->dump = dumpfile;
}



void dumpProcess(QString server, QString proc = QString())
{

    qDebug() << "Connecting to" << server;
    mdcli session(server);

    zmsg* req = proc.isEmpty() ? new zmsg() : new zmsg(proc.toLatin1());
    session.send("mmi.list", req);
    zmsg* reply = session.recv();
    if (reply)
    {
        reply->pop_front();
        if (proc.isEmpty())
            while (reply->parts())
                qDebug() << reply->pop_front();
        else
        {
            while (reply->parts())
                listParams(
                    QCborValue::fromCbor(reply->pop_front()).toMap().toJsonObject());
        }
    }
    else
            qDebug() << "No response from server";

    delete req;
    delete reply;


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


void showPlate(ExperimentFileModel* efm)
{
    qDebug() << "Channels" << efm->getChannelNames();
    auto sfm = efm->getFirstValidSequenceFiles();
    qDebug() << sfm.getZCount() << sfm.getTimePointCount() << sfm.getFieldCount() << sfm.getChannels();
    std::cout << "   ";
    for (unsigned c = 0; c < efm->getColCount(); ++c)
        std::cout << QString("%1 ").arg(c+1, 2).toStdString();

    std::cout << std::endl;
    int wells = 0;
    for (unsigned r = 0; r < efm->getRowCount(); ++r)
    {
        std::cout << QString("%1 ").arg(QLatin1Char('A'+r), 2).toStdString();
        for (unsigned c = 0; c < efm->getColCount(); ++c)
        {
            if ((*efm)(r,c).getAllFiles().size())
            {
                std::cout << QString("%1 ").arg((*efm)(r,c).getAllFiles().size(), 2).toStdString();
                wells++;
            }
            else
                std::cout << " - ";
        }
        std::cout << std::endl;
    }
    std::cout << "Plate contains " << wells << "wells" << std::endl;
}



void startProcess(QString server, QString proc, QString project, QString commitName,  QStringList params, QStringList plates, QString dumpfile, bool wait)
{

    qDebug() << "Connecting to" << server << "sending" << proc;
    qDebug() << "Should commit file to " << commitName;
    mdcli session(server);

    // Query the process list
    zmsg* req = new zmsg(proc.toLatin1());
    session.send("mmi.list", req);

    zmsg* reply = session.recv();

    qDebug() << reply->parts();

    assert(reply->pop_front() == "mmi.list");

    helper h;

    if (proc.endsWith("BirdView"))
        commitName = "BirdView";

    h.setParams(proc, commitName, params, plates, project);
    h.setDump(dumpfile);

    reply->pop_front();
    auto response =    reply->pop_front();

    qDebug() << "Response" << response;




    auto array = h.setupProcess(QCborValue::fromCbor(response).toMap().toJsonObject(),
                                QRegularExpression(), project);

    // Unroll the array to start the process
    delete req;
    delete reply;

    qDebug() << "Prepared " << array.size() << "Wells";
    req = new zmsg();//proc.toLatin1());

    int processes = array.size();

    auto cborArray = simplifyArray(array);
    for (int i = 0; i < cborArray.size(); ++i)
    {
//        qDebug() << array.at(i);
        QByteArray data = cborArray.at(i).toCbor();
        req->push_back(data);
    }

    qDebug() << "Sending process to queue" << req->parts();
    // Fire & Forget mode no need to track in CLI

    auto start = QDateTime::currentDateTime();

    // Send the process
    session.send(proc.toLatin1().toStdString(), req);

    delete req;
    cborArray.clear();
    // wait for reply
    reply = session.recv();
    if (reply)
        qDebug() << "Process started";
//    else
//        qDebug() << "Error while starting process";


    if (wait)
    {
        qDebug() << "Waiting for the process to finish";
        // query the broker for finish
        auto req = new zmsg();
        int finished = 0;
        while (finished < processes )
        {
            QThread::msleep(800);

            session.send("mmi.status", req);
            reply = session.recv();
            // analyze the reply
            reply->pop_front();
            QString msg = reply->pop_front();
            int count = msg.toInt();
            finished += count;

            if (count != 0)// do not hammer the terminal if nothing to update
            {
                std::cout << "\r" << std::fixed << std::setw(3)
                          << std::setprecision(1) << 100.*(finished / (float)processes ) << "% " <<  msg.toInt() << " " << finished << "/" << processes << "                                                  ";
                std::flush(std::cout);
            }
        }
        auto end = QDateTime::currentDateTime();
        qDebug() << "\r\n" << "Runtime" << end.toMSecsSinceEpoch() - start.toMSecsSinceEpoch() << "ms";

    }


}

int main(int ac, char** av)
{
    auto a = new QCoreApplication(ac, av);
    //    QApplication::setStyle(QStyleFactory::create("Plastique"));
    a->setApplicationName("PhenoLink");
    a->setApplicationVersion(CHECKOUT_VERSION);
    //    a.setApplicationDisplayName(QString("PhenoLink %1").arg(CHECKOUT_VERSION));
    a->setOrganizationDomain("WD");
    a->setOrganizationName("WD");

    /* After creating QApplication instance */
    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);


    QSettings set;

    QDir dir( QStandardPaths::standardLocations(QStandardPaths::AppDataLocation ).first());
    //QDir dir("P:/DATABASES");
    dir.mkpath(dir.absolutePath() + "/databases/");
    dir.mkpath(dir.absolutePath());

    if (!set.contains("databaseDir"))
        set.setValue("databaseDir", QStandardPaths::standardLocations(QStandardPaths::AppDataLocation ).first() + "/databases/");

    QStringList var = set.value("Server", QStringList() << "127.0.0.1:13555").toStringList();

    bool wait = false;
    if (ac < 2) help();

    for (int i = 1; i < ac; ++i)
    {
        QString item(av[i]);
        if (item.startsWith("Server=")) // Overload the configuraiton servers
        {
            var.clear();
            var << item.split("=").at(1);
        }
        if (item=="-w")
        {
            wait = true;
            qDebug() << "Run time wait mode: On";
        }
    }


    for (int i = 1; i < ac; ++i)
    {
        QString item(av[i]);

        // Dump server processes
        if (item == "ls" || item == "list")
        {

            if (ac == 2)
                dumpProcess(QString("tcp://%1").arg(var.first()));
            else
                dumpProcess(QString("tcp://%1").arg(var.first()), QString(av[i+1]));
        }

        // Describe a plate
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
                    QString file = handler.findPlate(plate, project.split(",", Qt::SkipEmptyParts), drive);
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
            commit = find("commitName", ac, p+1, av);

            for (; p < ac; ++p){
                QString par(av[p]);
                if (par.contains("="))
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
                    v = handler.findPlate(v, project.split(",", Qt::SkipEmptyParts), drive);
                    if (!v.isEmpty())
                        plates << v;
                }
            }
            if (!process.isEmpty() && !plates.isEmpty())
                startProcess(QString("tcp://%1").arg(var.first()), process, project, commit, pluginParams, plates, dumpfile, wait);
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
                }
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





