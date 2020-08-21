#include <ostream>
#include "mainwindow.h"
#include <QApplication>

#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <QStandardPaths>
#include <QLocale>

#include <QtSql>
#include <QDir>

#include "Core/config.h"

#include "Core/pluginmanager.h"
#include "Core/networkprocesshandler.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    QByteArray localMsg = msg.toLocal8Bit();
    QTextStream logFile(&_logFile);
    QString color = "", txt="";
    switch (type) {
    case QtInfoMsg:
        txt="Info";
        color="#00FF00";
        break;
    case QtDebugMsg:
        txt="Debug";
        color="#00FF00";
        break;
    case QtWarningMsg:
        txt="Warning";
        color="#FFFF00";
        break;
    case QtCriticalMsg:
        txt="Critical";
        color="#FF0000";
        break;
    case QtFatalMsg:
        txt="Fatal";
        color="#FF0000";
    }

    logFile << QString("<div>%4<div style='color:%1'>%2</div>  : %3</div>\r\n").arg(color).arg(txt).arg(msg).arg(QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz")) ;
    if (type == QtFatalMsg)  abort();

    logFile.flush();
}

int main(int argc, char *argv[])
{
    //    qInstallMessageHandler(myMessageOutput);

    QApplication a(argc, argv);
    a.setApplicationName("Checkout");
    a.setApplicationVersion(CHECKOUT_VERSION);
    a.setApplicationDisplayName(QString("Checkout %1").arg(CHECKOUT_VERSION));
    a.setOrganizationDomain("WD");
    a.setOrganizationName("WD");

    // Force number locale to use the "C" style for floating point value
    QLocale loc = QLocale::system(); // current locale
    loc.setNumberOptions(QLocale::c().numberOptions()); // borrow number options from the "C" locale
    QLocale::setDefault(loc); // set as default


    QDir dir( QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
    //QDir dir("P:/DATABASES");
    dir.mkdir(dir.absolutePath() + "/databases/");
    dir.mkpath(dir.absolutePath());

    _logFile.setFileName(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/WD_CheckoutLog.txt");
    _logFile.open(QIODevice::WriteOnly);


    //qDebug()  << QStandardPaths::standardLocations(QStandardPaths::DataLocation);


  /*  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dir.absolutePath() + "/WD_Checkout" + QString("%1").arg(CHECKOUT_VERSION) + ".db");

    bool ok = db.open();*/

    //Q_UNUSED(ok);

    //if (!db.tables().contains("OpennedScreens"))
    //{
    //    QSqlQuery qu = db.exec("create table OpennedScreens (hash text, "
    //                           "ScreenDirectory text, lastload datetime);");
    //}
    //if (!db.tables().contains("Processes"))
    //{
    //    QSqlQuery qu = db.exec("create table Processes (process_tag text, "
    //                           "process_json text, lastload datetime);");
    //}


    QSettings set;
    if (!set.contains("databaseDir"))
        set.setValue("databaseDir", QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/databases/");

    // Start the network worker for processes
    QProcess server;
    server.setProcessChannelMode(QProcess::MergedChannels);
    server.setStandardOutputFile(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() +"/CheckoutServer_log.txt");
    server.setWorkingDirectory(a.applicationDirPath());
    QString r = "CheckoutProcessServer.exe";

    server.setProgram(r);
    server.start();

    if (!server.waitForStarted())
        qDebug() << "Server not properly started" << server.errorString() << r;

    QThread::sleep(5);

    NetworkProcessHandler::handler().establishNetworkAvailability();

    PluginManager::loadPlugins();
    // Start the network based process plugins
    // qDebug() << server.readAll();


    MainWindow w(&server);

//    QFile stylesheet("://Styles/darkorange.qss");
//        if (stylesheet.open(QFile::ReadOnly))
//        {
//            QTextStream ss(&stylesheet);
//            w.setStyleSheet(ss.readAll());
//        }
//    stylesheet.close();

    w.show();
    int res = a.exec();

    // The server shall probably not be killed by the app,
    // depending on the expected behavior of the service
    //qDebug() << server.readAll();
    server.kill();

    foreach (QString file, ScreensHandler::getHandler().getTemporaryFiles())
        QFile::remove(file);

    return res;
}

