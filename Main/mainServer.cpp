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

#include <Server/processserver.h>


#include "Core/config.h"


#include <windows.h>
#include <wincon.h>

std::ofstream outfile("c:/temp/CheckoutServer_log.txt");

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray date = QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz").toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        outfile << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr << date.constData() << " Debug    : " <<  localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        outfile  << date.constData() << " Warning  : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr  << date.constData() << " Warning  : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        outfile   << date.constData()  << " Critical : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr   << date.constData()  << " Critical : " << localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        outfile  << date.constData() << " Fatal    : "<< localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        std::cerr  << date.constData() << " Fatal    : "<< localMsg.constData() << std::endl;//, context.file, context.line, context.function);
        abort();
    }

    outfile.flush();
}



void show_console() {
     AllocConsole();
     freopen("conin$", "r", stdin);
     freopen("conout$", "w", stdout);
     freopen("conout$", "w", stderr);
}



int forceNumaAll(int node)
{

    HANDLE process = GetCurrentProcess();

    DWORD_PTR processAffinityMask;
    DWORD_PTR systemAffinityMask;
    ULONGLONG  processorMask;

    if (!GetProcessAffinityMask(process, &processAffinityMask, &systemAffinityMask))
        return -1;

    GetNumaNodeProcessorMask(node, &processorMask);

    processAffinityMask = processAffinityMask & processorMask;

    BOOL success = SetProcessAffinityMask(process, processAffinityMask);

    qDebug() << success << GetLastError();
      
    return success;
}

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

int main(int ac,  char* av[])
{
    // 13378
    // outfile = std::ofstream("c:/temp/CheckoutServer_log.txt")

    // Skip log file creation (avoid generation of too much data)

//    qInstallMessageHandler(myMessageOutput);

#ifdef WIN32
    QApplication app(ac, av);



#else
    QCoreApplication app(ac, av);
#endif
    app.setApplicationName("Checkout");
    app.setApplicationVersion(CHECKOUT_VERSION);
    app.setOrganizationDomain("WD");
    app.setOrganizationName("WD");

    QSettings sets;
    uint port = sets.value("ServerPort", 13378).toUInt();
    if (!sets.contains("ServerPort")) sets.setValue("ServerPort", 13378);


    QStringList data;
    for (int i = 1; i < ac; ++i) { data << av[i];  }
    if (data.contains("-p"))
    {
        int idx = data.indexOf("-p")+1;
        if (data.size() > idx) port = data.at(idx).toInt();
        qDebug() << "Changing server port to :" << port;
    }
#if WIN32
    if (data.contains("-n"))
    {
        int idx = data.indexOf("-n")+1;
        int node = 0;
        if (data.size() > idx) node = data.at(idx).toInt();
        qDebug() << "Forcing node :" << node;
        forceNumaAll(node);

    }

#endif
    if (data.contains("-d"))
        show_console();


    if (data.contains("-s"))
    {
        int idx = data.indexOf("-ns")+1;
        QString file;
        if (data.size() > idx) file = data.at(idx);
        qDebug() << "Loading startup script :" << file;
        startup_execute(file);
    }

    PluginManager::loadPlugins(true);

    ProcessServer server;
    //    QThreadPool::globalInstance()->setMaxThreadCount(  QThreadPool::globalInstance()->maxThreadCount()/2);

    int nb_Threads = QThreadPool::globalInstance()->maxThreadCount() - 1;
//    nb_Threads = 1;
    qDebug() << "Max number of threads : " << nb_Threads;

    QThreadPool::globalInstance()->setMaxThreadCount(nb_Threads);

    if (!server.listen(QHostAddress::Any, port))
    {
        qDebug() << "Cannot listen on port" << port;
        return -1;
    }

    app.exec();

    return 0;
}

