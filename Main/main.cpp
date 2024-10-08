#include <ostream>
#include "mainwindow.h"
#include <QApplication>

#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <QStandardPaths>
#include <QLocale>

#include <QDir>

#include "Core/config.h"

#include "Core/pluginmanager.h"
#include "Core/networkprocesshandler.h"

#include <QLoggingCategory>

#if WIN32

    #include <windows.h>
    #include <wincon.h>
    // #include <QtWebView/QtWebView>

#endif

#include <QStyleFactory>


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

void show_console() {
#if WIN32

    AllocConsole();

    FILE *newstdin = nullptr;
    FILE *newstdout = nullptr;
    FILE *newstderr = nullptr;

    freopen_s(&newstdin, "conin$", "r", stdin);
    freopen_s(&newstdout, "conout$", "w", stdout);
    freopen_s(&newstderr, "conout$", "w", stderr);
#endif
}

int main(int argc, char *argv[])
{
    //    qInstallMessageHandler(myMessageOutput);
    //  show_console();
// #if WIN32
//     QtWebView::initialize();
// #endif

    QApplication a(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Plastique"));
    a.setApplicationName("PhenoLink");
    a.setApplicationVersion(CHECKOUT_VERSION);
    a.setApplicationDisplayName(QString("PhenoLink %1").arg(CHECKOUT_VERSION));
    a.setOrganizationDomain("WD");
    a.setOrganizationName("WD");
//    a.setStyle(QStyleFactory::create("windowsvista"));
    a.setStyle(QStyleFactory::create("Fusion"));
    QSettings set;

    if (set.value("UserMode/Debug", false).toBool())
        show_console();


    // const auto & styles = QStyleFactory::keys();
    // for(const auto & s : styles)
    // {
    //     qDebug() << s;
    // }


    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);

    // Force number locale to use the "C" style for floating point value
    QLocale loc = QLocale::system(); // current locale
    loc.setNumberOptions(QLocale::c().numberOptions()); // borrow number options from the "C" locale
    QLocale::setDefault(loc); // set as default


    QDir dir( QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());
    //QDir dir("P:/DATABASES");
    dir.mkpath(dir.absolutePath() + "/databases/");
    dir.mkpath(dir.absolutePath());

    _logFile.setFileName(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first() + "/WD_CheckoutLog.txt");
    _logFile.open(QIODevice::WriteOnly);

    if (!set.contains("databaseDir"))
        set.setValue("databaseDir", QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first() + "/databases/");


    QStringList var = set.value("ZMQServer", QStringList() << "127.0.0.1").toStringList();
    QProcess server;

    if (var.contains("127.0.0.1") || var.contains("localhost"))
//    if (false)
    {
        // Start the network worker for processes
         server.setProcessChannelMode(QProcess::MergedChannels);
        server.setStandardOutputFile(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first() +"/CheckoutServer_log.txt");
        server.setWorkingDirectory(a.applicationDirPath());
        QString r = "PhenoLinkZMQServer.exe";

        server.setProgram(r);
        if (set.value("UserMode/Debug", false).toBool())
            server.setArguments(QStringList() << "-stda" << "-d");
        else
            server.setArguments(QStringList() << "-stda" );


        server.start();


        QThread::sleep(1);
    }


//    NetworkProcessHandler::handler().establishNetworkAvailability();

    PluginManager::loadPlugins();
    // Start the network based process plugins
    // qDebug() << server.readAll();


    MainWindow w(&server);

//    QFile stylesheet("://Styles/darkorange.qss");
//    if (stylesheet.open(QFile::ReadOnly))
//    {
//        QTextStream ss(&stylesheet);
//        w.setStyleSheet(ss.readAll());
//    }
//    stylesheet.close();

    w.show();
    int res = a.exec();

    // The server shall probably not be killed by the app,
    // depending on the expected behavior of the service
    //qDebug() << server.readAll();
    if (server.isOpen())
    {
        server.terminate();
        QThread::sleep(1);
        server.kill();
        QThread::sleep(1);
    }


    foreach (QString file, ScreensHandler::getHandler().getTemporaryFiles())
        QFile::remove(file);

    exit(res);
    return res;
}

