#include "checkoutprocess.h"

#include "networkprocesshandler.h"

#include <wellplatemodel.h>


#include <QSharedMemory>
#include <config.h>

QMutex process_mutex(QMutex::NonRecursive);
QMutex hash_to_save_mtx(QMutex::NonRecursive);

CheckoutProcess::CheckoutProcess():
    _counter(0), mutex_dataupdate(QMutex::NonRecursive)
{
    startTimer(100);
}

CheckoutProcess& CheckoutProcess::handler()
{
    static CheckoutProcess* proc = 0;

    if (!proc)
    {
        proc = new CheckoutProcess();

        proc->connect(&NetworkProcessHandler::handler(), SIGNAL(newProcessList()),
                      proc, SLOT(updatePath()));
        proc->connect(&NetworkProcessHandler::handler(), SIGNAL(parametersReady(QJsonObject)),
                      proc, SLOT(receivedParameters(QJsonObject)))  ;

        proc->connect(&NetworkProcessHandler::handler(), SIGNAL(processStarted(QString, QString)),
                      proc, SLOT(networkProcessStarted(QString, QString)));

        proc->connect(&NetworkProcessHandler::handler(), SIGNAL(updateProcessStatusMessage(QJsonArray)),
                      proc, SLOT(networkupdateProcessStatus(QJsonArray)))  ;
    }

    return *proc;
}

void CheckoutProcess::addProcess(CheckoutProcessPluginInterface *proc)
{
    _plugins[proc->getPath()] = proc;
    //  QJsonObject obj;  proc->write(obj);  qDebug() << obj;
}

void CheckoutProcess::removeProcess(QString name)
{
    CheckoutProcessPluginInterface* itf = _plugins[name];
    _plugins.remove(name);

    delete itf;
}

void CheckoutProcess::removeProcess(CheckoutProcessPluginInterface *proc)
{
    removeProcess(proc->getPath());
}

QStringList CheckoutProcess::paths()
{
    QStringList res  = pluginPaths();
    res += networkPaths();
    return res;
}

QStringList CheckoutProcess::pluginPaths(bool withVersion)
{
    QStringList l;
    foreach(CheckoutProcessPluginInterface* plugin,  _plugins)
        if (plugin)
        {
            l << (withVersion ?
                     QString("%1 - %2 (%3 - last git %4)").arg(plugin->getPath(), plugin->plugin_version(), plugin->buildTime(), plugin->gitTime())
                     :
                     plugin->getPath() );
        }
    return l;
}

QStringList CheckoutProcess::networkPaths()
{
    QStringList l = NetworkProcessHandler::handler().getProcesses();
#ifdef CheckoutPluginInCore
    QStringList r;

    foreach (QString s, l)
        r << QString("Network/%1").arg(s);
    return r;
#else
    return l;
#endif  /* CheckoutPluginInCore */
}


QString CheckoutProcess::setDriveMap(QString map)
{
    qDebug() << "Setting the drive mapping to " << map;
    drive_map = map;
    return drive_map;
}

void CheckoutProcess::setProcessCounter(int *count)
{
    _counter = count;
}

int CheckoutProcess::getProcessCounter(QString hash)
{
    if (_hash_to_save[hash])
        return *_hash_to_save[hash];
    return 0;
}

void CheckoutProcess::getParameters(QString process)
{
    if (process.isEmpty()) return;

    CheckoutProcessPluginInterface* plugin = _plugins[process];
    if (plugin)
    {
        QJsonObject params;

        plugin->write(params);
        emit parametersReady(params);
    }
    else

    {
        //  qDebug() << process;
#ifdef CheckoutPluginInCore
        process = process.remove(0, QString("Network/").size());
#endif /* CheckoutPluginInCore */
        NetworkProcessHandler::handler().getParameters(process);
    }

}

void CheckoutProcess::getParameters(QString process, QJsonObject &params)
{
    CheckoutProcessPluginInterface* plugin = _plugins[process];
    if (plugin)
    {
        plugin->write(params);
    }
    else
    {
#ifdef CheckoutPluginInCore
        process = process.remove(0, QString("Network/").size());
#endif /* CheckoutPluginInCore */
        if (_params.contains(process))
            params = _params[process];
    }
}




#include <QRunnable>

class PluginRunner : public QRunnable
{

public:
    PluginRunner(CheckoutProcessPluginInterface* plu, QString hash): plugin(plu), _hash(hash)
    {
    }

    QString name()
    {
        if (plugin)
            return plugin->getPath();
        return QString();
    }

    virtual void	run()
    {
        int p = 0;
        try {

            QElapsedTimer dtimer;       // Do the process timing
            dtimer.start();
            p=1;
            plugin->prepareData();
            p=2;
            plugin->started(dtimer.elapsed());
            p=3;
            QElapsedTimer timer;       // Do the process timing
            timer.start();
            p=4;
            plugin->exec();
            p=5;
            plugin->finished();
            p=6;
            CheckoutProcess::handler().removeRunner(plugin->user(), (void*)this);
            p=7;
            // - 4) Do the gathering of processes data
            //        qDebug() << "Gathering data";
            QJsonObject result = plugin->gatherData(timer.elapsed());

            p=8;
            qDebug() << timer.elapsed() << "(ms) done";
            CheckoutProcess::handler().finishedProcess(_hash, result);
            p=9;
        }
        catch (...)
        {
            plugin->printMessage(QString("Plugin: %1 failed to run properly, trying to recover (%2)").arg(plugin->getPath()).arg(p));
            plugin->finished();
        }
        // Plugin should be deletable now, should not be saved anywhere
        delete plugin;
        plugin=0;
    }

    ~PluginRunner()
    {
        if (plugin)
            delete plugin;
    }

protected:
    CheckoutProcessPluginInterface* plugin;
    QString _hash;

};



void CheckoutProcess::startProcess(QString process, QJsonArray &array)
{
    //    qDebug() << "Starting process" << process << array;
    QSettings set;

#ifdef WIN32
    QString username = qgetenv("USERNAME");
#else
    QString username = qgetenv("USER");
#endif

    username =   set.value("UserName", username).toString();

    for (int i = 0; i < array.size(); ++i)
    {
        QJsonObject params = array.at(i).toObject();
        _process_to_start[params["CoreProcess_hash"].toString()] = params;
        _display[params["CoreProcess_hash"].toString()] = true;

        QJsonObject pp(params);
        //        pp.remove("CommitName");
        //      pp.remove("CoreProcess_hash");

        QJsonArray arr = pp["Parameters"].toArray(), rra;

        for (int i = 0; i < arr.size(); ++i)
        {
            QJsonObject ob = arr.at(i).toObject();
            if (ob.contains("Value") || ob.contains("Data"))
            {
                QJsonObject r;
                auto l = QStringList() << "Tag" << "Value" << "Value2" << "Channels"
                                       << "Channel" << "ChannelNames" << "Data"
                                       << "isOptional" << "optionalState"
                                       << "BasePath" << "ContentType" << "ImageType"
                                       << "PlateName" << "Enum" << "DataHash" << "Properties" << "PlateName"
                                       << "Pos" << "Channel" << "asVectorImage" << "tiled" << "unbias"
                                       << "isImage" << "FieldId" << "TimePos" << "zPos";
                for (auto key : l)
                    if (ob.contains(key))
                        r[key] = ob[key];

                rra.push_back(r);
            }
        }

        pp["Parameters"] = rra;
        auto l = QStringList() << "Comment" << "ProcessStartId" << "State" << "authors" << "shallDisplay";
        for (auto key: l)
            pp.remove(key);

        QJsonDocument doc(pp);

        pp["Username"] = username;
        pp["Computer"] = QHostInfo::localHostName();
        pp["StartTime"] = QDateTime::currentDateTime().toMSecsSinceEpoch();
        array.replace(i, pp);
    }

#ifdef CheckoutPluginInCore

    CheckoutProcessPluginInterface* plugin = _plugins[process];

    if (plugin)
        for (int i = 0; i < array.size(); ++i)
        {
            QJsonObject params = array.at(i).toObject();      // - 1) clone the plugin: call the clone() function
            plugin = plugin->clone();
            // - 2) Set parameters
            // - 2.a) load json prepared data
            QString hash = params["CoreProcess_hash"].toString();
            params["Process_hash"] = hash;
            params["StartTime"] = QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz");

            process_mutex.lock();
            _status[params["Process_hash"].toString()] = plugin;
            process_mutex.unlock();
            plugin->read(params);



            PluginRunner* runner = new PluginRunner(plugin, hash);
            QThreadPool::globalInstance()->start(runner);
            //        plugin->exec();
            //      qDebug()       << QThreadPool::globalInstance()->activeThreadCount()
            //                     << QThreadPool::globalInstance()->maxThreadCount();

        }

    else
#endif /* CheckoutPluginInCore */
    {
#ifdef CheckoutPluginInCore
        process = process.remove(0, QString("Network/").size());
#endif /* CheckoutPluginInCore */
        NetworkProcessHandler::handler().startProcess(process, array);


    }

    return;
}

void CheckoutProcess::restartProcessOnErrors()
{
    NetworkProcessHandler& handler = NetworkProcessHandler::handler();
    QList<QPair<QString, QJsonArray> > errors = handler.erroneousProcesses();

    foreach (auto d, errors)
    {
        handler.startProcess(d.first, d.second);
    }

}

QMutex status_protect;

QJsonObject remap(QJsonObject , QString );

QJsonValue remap(QJsonValue v, QString map)
{

    if (v.isString())
    {
        QString value = v.toString();
        if (value[1]==":")
        {
            QJsonValue res = map + value.remove(1,1);
            //            qDebug() << "Remap" << v << res;
            return res;
        }
    }
    else if (v.isArray())
    {
        QJsonArray res;
        for (auto item: v.toArray())
        {
            res.append(remap(item, map));
        }
        return res;
    }
    else if (v.isObject())
        return remap(v.toObject(), map);

    return v;
}


QJsonObject remap(QJsonObject ob, QString map)
{
    QJsonObject res;

    for (QJsonObject::iterator it = ob.begin(); it != ob.end(); ++it)
    {
        res[it.key()]=remap(it.value(), map);
    }

    return res;
}


void CheckoutProcess::startProcessServer(QString process, QJsonArray &array)
{

    qDebug() << "Remaining unstarted processes" << _process_to_start.size()
             << QThreadPool::globalInstance()->activeThreadCount()
             << QThreadPool::globalInstance()->maxThreadCount();

    CheckoutProcessPluginInterface* plugin = _plugins[process];
    if (plugin)
    {
        for (int i = 0; i < array.size(); ++i)
        {
            QJsonObject params = array.at(i).toObject();
            if (!this->drive_map.isEmpty())
            { // Iterate in each params data to remap drives !
                params = remap(params, drive_map);
            }


            //  qDebug() << "Starting process" << process << params;
            // - 1) clone the plugin: call the clone() function

            plugin = plugin->clone();
            if (!plugin)
            {
                qDebug() << "Plugin cannot be cloned";
                break;
            }

            //params["StartTime"] = QDateTime::currentDateTime().toString("yyyyMMdd:hhmmss.zzz");

            QString hash = params["Process_hash"].toString();
            // qDebug() << "Process hash" << hash;
            status_protect.lock();
            _status[hash] = plugin;
            status_protect.unlock();

            // - 2) Set parameters
            // - 2.a) load json prepared data
            //          qDebug() << "Reading params";
            plugin->read(params);

            //           qDebug() << "Starting process";
            PluginRunner* runner = new PluginRunner(plugin, hash);
            if (runner)
            {
                QString key = params["Username"].toString() + "@" + params["Computer"].toString();
                status_protect.lock();
                _peruser_runners[key].push_back((void*)runner);
                status_protect.unlock();
                auto th_instance = QThreadPool::globalInstance();
                if (th_instance)
                    th_instance->start(runner);
                else
                    qDebug() << "Error with launching threads";
            }
            else
            {
                qDebug() << "Unable to start runner";
            }
        }
    }
    else
        qDebug() << "Process" << process << "not found";

    qDebug() << "Process add in thread list";
}


void CheckoutProcess::updatePath()
{
    emit newPaths();
}

void CheckoutProcess::receivedParameters(QJsonObject obj)
{
    //  qDebug() << obj;
    QString process = obj["Path"].toString();
    _params[process] = obj;
    emit parametersReady(obj);
}

class ProcessDataHolder : public CheckoutProcessPluginInterface
{
public:
    ProcessDataHolder(): CheckoutProcessPluginInterface()
    {

    }

    virtual ~ProcessDataHolder()
    {

    }




protected:
    virtual CheckoutProcessPluginInterface* clone()
    {
        qDebug() << "ProcessDataHolder::clone() was called, shall not happen";
        return this;
    }

    virtual void exec()
    {
        qDebug() << "ProcessDataHolder::exec() was called, shall not happen";
    }

};


void CheckoutProcess::networkProcessStarted(QString core, QString hash)
{
    if (!_process_to_start.size()) return;

    _hash_to_core[hash]=core;
    QJsonObject ob = _process_to_start[core];
    _process_to_start.remove(core);

    //    qDebug() << "Remaining unstarted processes" << _process_to_start.size()
    //             << QThreadPool::globalInstance()->activeThreadCount()
    //                << QThreadPool::globalInstance()->maxThreadCount();

    ProcessDataHolder* holder = new ProcessDataHolder;


    //  qDebug() << ob["Hash"].toString();

    holder->description(ob["Path"].toString(), QStringList(), "");
    holder->setPosition(ob["Pos"].toString());
    status_protect.lock();
    _status[hash] = holder;
    status_protect.unlock();
    hash_to_save_mtx.lock();
    _hash_to_save[hash]=_counter;
    if (_counter)    (*_counter)++;
    hash_to_save_mtx.unlock();
//    emit processStarted(hash);

}

int getKeyFromJSON(QString key, QJsonObject ob);

void CheckoutProcess::addToComputedDataModel(QJsonObject ob)
{

    // Sanity Checking
    QStringList vals = QStringList() << "Parameters" << "CommitName" << "Path" << "Data";
    foreach (QString v, vals)
        if (!ob.contains(v))
            return;

    //  qDebug() << "DB Commit Started";
    QString hashDetails, hashData, id;
    int timepoint = -1, fieldId = -1, sliceId = -1, channel = -1;

    QJsonArray params = ob["Parameters"].toArray();
    //   qDebug() << "Add to Computed data model" << ob;

    for (int i = 0; i < params.size(); ++i )
    {
        QJsonObject par = params[i].toObject();
        //      if (par.contains("Meta"))
        //        {
        //          qDebug() << par["Meta"];
        // //        QJsonObject meta = p
        //        }
        if (par["isImage"].toBool() && par.contains("DataHash"))
        {
            hashDetails = par["DataHash"].toString();
            hashData = par["DataHash"].toString();

            fieldId = getKeyFromJSON("FieldId", par);
            timepoint = getKeyFromJSON("TimePos", par);
            sliceId = getKeyFromJSON("zPos", par);
            channel = getKeyFromJSON("Channels", par);

            id = par["Pos"].toString();
            break;
        }
    }

    Screens& screens = ScreensHandler::getHandler().getScreens();
    ExperimentDataModel* datamdl = 0;

    foreach (ExperimentFileModel* mdl, screens)
        if (hashData.startsWith(mdl->hash()))
        {
            mutex_dataupdate.lock();
            datamdl = mdl->computedDataModel();
            mutex_dataupdate.unlock();
            break;
        }

    if (!datamdl) {
        qDebug() << "Experiment data not found, could not store data...." << hashData;
        return;
    }

    if (!ob.contains("CommitName"))
        qDebug() << "No commmit name....";
    else
    {
        mutex_dataupdate.lock();
        datamdl->setCommitName(ob["CommitName"].toString());
        mutex_dataupdate.unlock();
    }
    QString alg = ob["Path"].toString().replace("/", "_").replace("-", "_").replace(" ", "_");
    QJsonArray data = ob["Data"].toArray();

    for (int i = 0; i < data.size(); ++i)
    {

        QJsonObject d = data[i].toObject();
        if (d["isImage"].toBool()) continue;

        QString tag = d["Tag"].toString().replace(" ", "_").replace("-", "_").replace(" ", "_");
        mutex_dataupdate.lock();
        datamdl->setAggregationMethod(tag, d["Aggregation"].toString() );
        if (d["Data"].toString().contains(";"))
        {
            QStringList t = d["Data"].toString().split(";",Qt::SkipEmptyParts);
            for (int i = 0; i < t.size(); ++i)
                datamdl->addData(QString("%1#%2").arg(tag).arg(i, 4, 10, QLatin1Char('0')), fieldId, sliceId, timepoint, channel, id, t[i].toDouble());
        }
        else
            datamdl->addData(tag, fieldId, sliceId, timepoint, channel, id,d["Data"].toString().toDouble());
        mutex_dataupdate.unlock();
    }

}


void CheckoutProcess::networkupdateProcessStatus(QJsonArray obj)
{
    QJsonArray finished, unfinished;
    //    qDebug() << "Finished Processes" << obj.size();

    for (int i = 0; i < obj.size(); ++i)
    {
        qDebug() << obj[i].toObject();
        if (!obj[i].isObject()) continue;
        QJsonObject ob = obj[i].toObject();

        if (ob["State"].toString() == "Finished")
        {

            addToComputedDataModel(ob);

            QString hash=ob["Hash"].toString();
            hash_to_save_mtx.lock();
            _status.remove(hash);
            //            qDebug() << "GUI Finished Hash" << hash << _hash_to_save.contains(hash);



            if (_hash_to_save[hash])
            {
                int* c = _hash_to_save[hash];
                (*c)--;
                int tc = *c;

                if (tc == 0)
                {
                    qDebug() << "GUI Hash finished" << hash;
                    //                    qDebug() << ob;
                    // Last object of the running process, check for the field CommitName in ob & commit to the database if not empty
                    _hash_to_save.remove(hash);

                }
            }

            hash_to_save_mtx.unlock();
            status_protect.lock();
            if (_status.isEmpty())
            {
                qDebug() << NetworkProcessHandler::handler().remainingProcess();
                emit emptyProcessList();
            }
            status_protect.unlock();
            //            qDebug() << "Adding object" << ob;
            finished.append(ob);

        }
        else
            unfinished.append(ob);

    }
    qDebug() <<"Clearing finished process: " << finished.size();

    emit processFinished(finished);

    emit updateProcessStatus(unfinished);
}

void CheckoutProcess::refreshProcessStatus()
{
    QMutexLocker locker(&process_mutex);
    QMap<QString, QList<QString> > reorg;
    for (QMap<QString, CheckoutProcessPluginInterface*>::iterator it = _status.begin(), e  = _status.end(); it != e; ++it)
    {
        CheckoutProcessPluginInterface* intf = it.value();
        if (dynamic_cast<ProcessDataHolder*>(intf))
        {
            reorg[intf->getPath()] << it.key();
        }
    }
    //    if (reorg.size())
    //        qDebug() << "Refresh process status" << reorg.size();
    for (QMap<QString, QList<QString> >::iterator it = reorg.begin(), e  = reorg.end(); it != e; ++it)
    {
        NetworkProcessHandler::handler().getProcessMessageStatus(it.key(), it.value());
    }

}

QJsonObject CheckoutProcess::refreshProcessStatus(QString hash)
{
    QMutexLocker locker(&process_mutex);
    if (_finished.contains(hash))
    {
        qDebug() << "Process " << hash << "finished";
        return _finished[hash];
    }

    // Do nothing still not started...
    if (_process_to_start.contains(hash))
        return QJsonObject();


    CheckoutProcessPluginInterface* intf = _status[hash];

    if (!intf)
    { /*qDebug() <<"Hash not found" << hash ;*/return QJsonObject(); }

    if (dynamic_cast<ProcessDataHolder*>(intf))
    { /*qDebug() <<"Hash not found (Remote)" << hash ;*/ return QJsonObject(); }


    QJsonObject obj = intf->createStatusMessage();
    obj["Path"] = intf->getPath(); // the process called

    return obj;
}

// Server side process finished!
void CheckoutProcess::finishedProcess(QString hash, QJsonObject result)
{

    QMutexLocker locker(&process_mutex);

    CheckoutProcessPluginInterface* intf = _status[hash];
    if (dynamic_cast<ProcessDataHolder*>(intf))  return ;

    //    qDebug() << "Process finished " << hash << "emiting signal";
    emit finishedJob(hash, result);
    _status.remove(hash);
    _finished[hash] = result;

    qDebug() << "process finished, remaining" << _status.size();
    //    qDebug() << "Removing" << hash;
}

unsigned CheckoutProcess::numberOfRunningProcess()
{
    return  _status.size();
}

void CheckoutProcess::exitServer()
{
    NetworkProcessHandler::handler().exitServer();
}

bool CheckoutProcess::shallDisplay(QString hash)
{
    return _display.contains(_hash_to_core[hash]);
}

void CheckoutProcess::attachPayload(QString hash, std::vector<unsigned char> data,
                                    bool , size_t pos)
{
    //    qDebug() << "Attaching payload" << hash << data.size() <<  pos;
    if (pos != 0)
        hash += QString("%1").arg(pos);

    _payloads_vectors[hash] = data;

    emit payloadAvailable(hash);
}

void CheckoutProcess::attachPlugin(QString hash, CheckoutProcessPluginInterface *p)
{
    _stored[hash] = p;
}

bool CheckoutProcess::hasPayload(QString hash)
{
    return /* _payloads.contains(hash) || */
            _payloads_vectors.contains(hash);
}

unsigned CheckoutProcess::errors()
{
    return NetworkProcessHandler::handler().errors();
}

void CheckoutProcess::getStatus(QJsonObject& ob)
{
    for (auto it = _peruser_runners.begin(), e = _peruser_runners.end(); it != e; ++it)
    {
        QMap<QString, int> counter;
        status_protect.lock();

        for (auto q: it.value())
        {
            auto pl = static_cast<PluginRunner*>(q);
            counter[pl->name()]++;
        }
        for (auto it = counter.begin(), e = counter.end(); it != e; ++it)
            ob[it.key()]=it.value();
        status_protect.unlock();
    }
}

QString CheckoutProcess::dumpProcesses()
{
    return QString();
}

QString CheckoutProcess::dumpHtmlStatus()
{
    QString body;

    body += QString("<h2>Connected Users : %1</h2>").arg(_peruser_runners.size());

    for (auto it = _peruser_runners.begin(), e = _peruser_runners.end(); it != e; ++it)
    {
        body += QString("%1 : %2 <a href='/Cancel/%1'>Cancel User Processes</a><br>").arg(it.key()).arg(it.value().size());
        QMap<QString, int> counter;
        status_protect.lock();

        for (auto q: it.value())
        {
            auto pl = static_cast<PluginRunner*>(q);
            counter[pl->name()]++;
        }

        for (auto it = counter.begin(), e = counter.end(); it != e; ++it)
            body += QString("<p>%1 : %2 </p>").arg(it.key()).arg(it.value());

        status_protect.unlock();
    }
    QHostInfo info;
    QStringList addresses;
    for (auto v : info.addresses())
        addresses.append(v.toString());

    return QString("<html><title>Checkout Server Status %2</title><body><p>%3</p>%1</body></html>").arg(body).arg(addresses.join(" ")).arg(CHECKOUT_VERSION);
}

QStringList CheckoutProcess::users()
{
    QStringList users, del;
    status_protect.lock();

    for (auto it = _peruser_runners.begin(), e = _peruser_runners.end(); it != e; ++it)
        if (it.value().size() > 0)
            users << it.key();
        else
            del << it.key();

    for (auto v: del)
        _peruser_runners.remove(v);
    status_protect.unlock();
    return users;
}

void CheckoutProcess::removeRunner(QString user, void *run) {
    status_protect.lock();
    _peruser_runners[user].removeOne(run);
    if (_peruser_runners[user].isEmpty())
        _peruser_runners.remove(user);
    status_protect.unlock();
}

void CheckoutProcess::cancelUser(QString user)
{
    status_protect.lock();
    for (auto q : _peruser_runners[user])
    {
        auto res = QThreadPool::globalInstance()->tryTake(static_cast<PluginRunner*>(q));
        Q_UNUSED(res);
    }
    _peruser_runners[user].clear();
    _peruser_runners.remove(user);
    status_protect.unlock();
}

void CheckoutProcess::finishedProcess(QStringList /*dhash*/)
{
//    NetworkProcessHandler::handler().processFinished(dhash);
}

std::vector<unsigned char> CheckoutProcess::detachPayload(QString hash)
{
    std::vector<unsigned char> r;
    if (_payloads_vectors.contains(hash))
    {
        //        qDebug() << "Searched hash" << hash;
        r = std::move(_payloads_vectors[hash]);
        _payloads_vectors.remove(hash);
        if (_stored.contains(hash))
        {
            _stored.remove(hash);
        }

    }
    else { qDebug() << "Expecting hash (" << hash << ") to be present in the server, but was not found, skipping..."; }

    return r;
}

