
#include <QtCore>
#ifdef WIN32
#include <QApplication>
#endif

#include <QtNetwork>
#include <QDebug>

#include <fstream>
#include <iostream>
#include <limits>

#include "Core/pluginmanager.h"

#include <Core/checkoutprocessplugininterface.h>

#include <Core/checkoutprocess.h>

#include "Core/config.h"

#ifdef WIN32
#include <windows.h>
#include <wincon.h>
#endif

#include <QtConcurrent>

#define ZMQ_STATIC

#include <zmq/mdwrkapi.hpp>
#include <zmq/mdbroker.hpp>

QString storage_path;

void myMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray date = QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz").toLocal8Bit();
    switch (type) {
    case QtInfoMsg:
        std::cerr << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtDebugMsg:
        std::cerr << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        std::cerr  << date.constData() << " Warning  : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        std::cerr   << date.constData()  << " Critical : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        std::cerr  << date.constData() << " Fatal    : "<< localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        abort();

    }
}

#if WIN32

void show_console() {
    AllocConsole();

    FILE *newstdin = nullptr;
    FILE *newstdout = nullptr;
    FILE *newstderr = nullptr;

    freopen_s(&newstdin, "conin$", "r", stdin);
    freopen_s(&newstdout, "conout$", "w", stdout);
    freopen_s(&newstderr, "conout$", "w", stderr);
}


#endif

void startup_execute(QString file)
{
    QFile f(file);

    QString ex=QString(f.readLine());
    while (!ex.isEmpty())
    {
        QStringList line = ex.split(" ");
        QString prog = line.front();
        line.pop_front();
        QProcess::execute(prog, line);


        ex = QString(f.readLine());
    }

}

QProcessEnvironment python_config;

#include <checkout_python.h>

int main(int ac, char** av)
{
#ifdef WIN32
    QApplication app(ac, av);
#else
    QCoreApplication app(ac, av);
#endif

    app.setApplicationName("PhenoLink");
    app.setApplicationVersion(CHECKOUT_VERSION);
    app.setOrganizationDomain("WD");
    app.setOrganizationName("WD");



    QSettings sets;
    uint port = sets.value("ZMQServerPort", 13555).toUInt();
    if (!sets.contains("ZMQServerPort")) sets.setValue("ZMQServerPort", 13555);


    QStringList data;
    for (int i = 1; i < ac; ++i) { data << av[i];  }

    if (data.contains("-h") || data.contains("-help"))
    {
        qDebug() << "PhenoLink ZeroMQ Queue Serveur Help:";
        qDebug() << "\t-p <port>: specify the port to run on";
        qDebug() << "\t-d : Run in debug mode (windows only launches a console)";
        qDebug() << "\t-Crashed: Reports that the process has been restarted after crashing";
        qDebug() << "\t-c <config>: Specify a config file for python env setting json dict";
        qDebug() << "\t-t <path> :  Set the storage path for plugins postprocesses";
        qDebug() << "\t\t shall contain env. variable with list of values,";
        qDebug()<< "$NAME will be substituted by current env value called 'NAME'";
        ;

        exit(0);
    }

    if (data.contains("-p"))
    {
        int idx = data.indexOf("-p")+1;
        if (data.size() > idx) port = data.at(idx).toInt();
        qDebug() << "Changing server port to :" << port;
    }
    else
        qDebug() << "Server port :" << port;


    if (data.contains("-Crashed"))
    { // recovering from crashed session, how to tell the user the queue crashed???

    }


    if (data.contains("-c"))
    {
        QString config;
        int idx = data.indexOf("-c")+1;
        if (data.size() > idx) config = data.at(idx);
        qDebug() << "Using server config:" << config;

        QFile loadFile(config);

        parse_python_conf(loadFile, python_config);

    }

    if (data.contains("-t"))
    {
        int idx = data.indexOf("-t")+1;
        if (data.size() > idx) storage_path = data.at(idx);
        qDebug() << "Setting Storage path :" << storage_path;
    }
    else
    {
#if WIN32
        storage_path = "L:/";
#else
        storage_path = "/mnt/shares/L/";
#endif
    }

    int verbose = 0; //(ac > 1 && strcmp (av [1], "-v") == 0);

    if (data.contains("-d"))
    {
#if WIN32
//        show_console();
#endif
        verbose = 1;
    }





    s_version_assert (4, 0);
    s_catch_signals ();

    broker brk(verbose);
    brk.setpython_env(python_config);

    brk.bind (QString("tcp://*:%1").arg(port));

    brk.start_brokering();

    if (s_interrupted)
        printf ("W: interrupt received, shutting down...\n");

    return 0;


}



#include <checkout_arrow.h>


QString adjust(QString script)
{
#ifdef WIN32
    return storage_path + script;
#else
    return storage_path + script.replace(":", "");
#endif
}


