
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


void help()
{
    qDebug() << "Checkout Command Line interface";
    qDebug() << "ckcli:";
    qDebug() << "\tls/list:  List available plugins";
    qDebug() << "\tls/list {plugin/name} Describe the plugin info";
    qDebug() << "\tds/describe platename: Load the plate for an overview";
    qDebug() << "\trun plugin/name [key=value] [plate names]: Run the plugin on the plates";
    qDebug() << "\tdump {output.json} plugin/name [key=value] [plate names]: Dumps the description of the run";
    qDebug() << "\tload {output.json} : Load and run a description of a run";

    qApp->exit();
}



void helper::listParams(QJsonObject ob)
{
    qDebug() << ob["Comment"].toString();

    for (auto p: ob["Parameters"].toArray())
    {
        auto c = p.toObject();
        qDebug() << c["Tag"].toString().replace(" ", "") << "\t" << c["Comment"].toString();
    }
    qApp->exit();
}

void helper::startProcess(QJsonObject ob)
{

   QJsonArray startParams;

    // Need to match the ob & this->params


    // Need to load & unroll the plates

    NetworkProcessHandler::handler().startProcess(process, startParams);
}


void helper::setParams(QString proc, QStringList params, QStringList plates)
{
    this->process = proc;
    this->params = params;
    this->plates = plates;
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




void startProcess(QString proc, QStringList params, QStringList plates)
{
    NetworkProcessHandler::handler().establishNetworkAvailability();
    while(NetworkProcessHandler::handler().getProcesses().isEmpty())
        qApp->processEvents();
    NetworkProcessHandler::handler().getParameters(proc);

    helper h;

    qApp->connect(&NetworkProcessHandler::handler(), SIGNAL(parametersReady(QJsonObject)),
                  &h, SLOT(startProcess(QJsonObject)));

    qApp->exec();

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

            for (int p = i+1; p < ac; ++p)
            {
                QString plate(av[p]);
                if (!plate.contains("="))
                {
                    QString file = handler.findPlate(plate, project);
                    // Found a plate
                    auto efm = handler.loadScreen(file);
                    showPlate(efm);
                }
            }
            //            qDebug() << file;
        }

        if (item == "run")
        {
            QProcess server;
            startServer(a, server, var);
            if (ac <= i+1) { help(); exit(0); }

            QString process(av[i+1]);


            QStringList pluginParams;
            int p = i+1;
            for (; p < ac; ++p){
                QString par(av[p]);
                if (par.contains("="))
                {
                    pluginParams << par;
                }
                else
                    break;
            }

            QString project=find("project", ac, p+1, av);
            QStringList plates;
            ScreensHandler& handler = ScreensHandler::getHandler();

            for (int l = p+1; l <ac; ++l)
            {
                QString v(av[l]);
                if (!v.contains("="))
                {
                    v = handler.findPlate(v, project);
                    if (!v.isEmpty())
                        plates << v;
                }
            }

            if (!process.isEmpty() && !plates.isEmpty())
                startProcess(process, pluginParams, plates);
        }

    }

    delete a;
    return 0;
}





