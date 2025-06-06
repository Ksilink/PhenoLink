#include "pluginmanager.h"


#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QString>
#include <QPluginLoader>

#include "checkoutdataloaderplugininterface.h"
#include "checkoutprocessplugininterface.h"
#include "checkoutprocess.h"

#include <QtConcurrent>
#include <QSettings>

namespace PluginManager
{

void loadPlugins(bool isServer)
{
    QDir pluginsDir(qApp->applicationDirPath());
//    QSettings set;



    bool isDebug = false;
    bool isRelease = false;
    bool isRelDeb = false;
#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug")
    {
        pluginsDir.cdUp();
        isDebug = true;
    }
    if (  pluginsDir.dirName().toLower() == "release")
    {
        pluginsDir.cdUp();
        isRelease = true;
    }
    if (  pluginsDir.dirName().toLower() == "relwithdebinfo")
    {
        pluginsDir.cdUp();
        isRelDeb = true;
    }


#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif

    pluginsDir.cd("plugins");

    if (qApp->property("LoadMode").isValid())
    {
        pluginsDir.cd(qApp->property("LoadMode").toString());
    }
    else
    {
        if (isDebug) pluginsDir.cd("Debug");
        if (isRelease) pluginsDir.cd("Release");
        if (isRelDeb) pluginsDir.cd("RelWithDebInfo");
    }

    if (!pluginsDir.setCurrent(pluginsDir.path()))
        qDebug() << "Unable to set current directory to " << pluginsDir;

    QCoreApplication::instance()->addLibraryPath(pluginsDir.path());

    CheckoutDataLoader& loader = CheckoutDataLoader::handler();
    CheckoutProcess & process = CheckoutProcess::handler();
    QMutex mutex;

    struct paraLoader{
        paraLoader(CheckoutDataLoader& l, CheckoutProcess & p, QDir pl, bool serv, QMutex& mut):
            loader(l), process(p), pluginsDir(pl), isServer(serv), mutx(mut)
        {

        }
        CheckoutDataLoader& loader;
        CheckoutProcess& process;
        QDir pluginsDir;
        bool isServer;
        QMutex& mutx;

        void operator()(QString fileName)
        {

            QSettings set;

//            if (set.value("UserMode/Debug", false).toBool())
//                qDebug() << "Checking file" << fileName << pluginsDir.absoluteFilePath(fileName);

            QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));


            QObject *plugin = pluginLoader.instance();

                        if (!pluginLoader.isLoaded())
                            qDebug() << pluginLoader.errorString();


            if (plugin)
            {
                bool added = false;
            //    qDebug() << "Plugin" << pluginsDir.absoluteFilePath(fileName);
                CheckoutDataLoaderPluginInterface* pl = qobject_cast<CheckoutDataLoaderPluginInterface*>(plugin);
                if (pl)
                {
//                    if (set.value("UserMode/Debug", false).toBool())
//                        qDebug() << "Plugin" << pl->pluginName() << "(" << fileName << ") loaded handling: " << pl->handledFiles();
                    mutx.lock();
                    loader.addPlugin(pl);
                    mutx.unlock();
                    added = true;
                }

                CheckoutProcessPluginInterface* pr = qobject_cast<CheckoutProcessPluginInterface*>(plugin);
                if (isServer && pr)
                {
                    if (pr->plugin_version().isEmpty())
                    {
                        qDebug() << "WARNING empty versionned plugin are not loaded anymore !!";
                        qDebug() << "        Fix:"<< pr->getPath();
                    }
                    else
                    {
//                        if (set.value("UserMode/Debug", false).toBool())
//                            qDebug() << "Plugin" << pr->getPath() << pr->getComments() << "(" << pr->getAuthors() << ")" << pr->plugin_version();
                        mutx.lock();
                        process.addProcess(pr);
                        mutx.unlock();
                        added = true;
                    }
                }

                if (!added)
                { // Removed plugin not added...
                    //				qDebug() << "Unloading plugin" << pluginLoader.errorString();
                    pluginLoader.unload();
                }
            }
        }
    };

    QStringList entries = pluginsDir.entryList(QStringList() << "*.dll" << "*.so" << "*.dylib", QDir::Files);
    entries.removeIf([](const QString& str){
        if (str.contains("opencv") || str.contains("brotl") ||
            str.contains("jxl") || str.contains("webp") ||
            str.contains("tiff") || str.contains("protob") ||
            str.contains("jxl") || str.contains("webp") ||
            str.contains("tbb") || str.contains("zlib") || str.contains("png") ||
            str.contains("yuv") || str.contains("fftw") || str.contains("hwy") ||
            str.contains("lcms") || str.contains("lzma") || str.contains("jpeg") ||

            str.contains("conc") || str.contains("abseil") || str.contains("gfortran") ||
            str.contains("quadmath") || str.contains("lapack") || str.contains("gcc") ||

            str.contains("pthread") || str.contains("vc") || str.contains("Qt6") || str.contains("blas")

                )
                return true; else return false; });

    QtConcurrent::blockingMap(entries, paraLoader(loader, process, pluginsDir, isServer, mutex));

}

}
