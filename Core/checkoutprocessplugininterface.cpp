#include "checkoutprocessplugininterface.h"
#include <QElapsedTimer>

#include <networkprocesshandler.h>

#include <checkoutprocess.h>



QMutex CheckoutProcessPluginInterface::mutex;
QMap<QString, QVector<cv::Mat*> > CheckoutProcessPluginInterface::_hashtoBias;
QMap<QString, int > CheckoutProcessPluginInterface::_hashtoBiasCount;

QString CheckoutProcessPluginInterface::datastore;

CheckoutProcessPluginInterface::CheckoutProcessPluginInterface(): _state(NotStarted)
{
    _infos << QString("[%1]").arg((quint64)QThread::currentThreadId());
}

QString CheckoutProcessPluginInterface::getEnv(QString key, QString def)
{
    return CheckoutProcess::handler().getEnv(key, def);
}



CheckoutProcessPluginInterface &CheckoutProcessPluginInterface::description(QString path, QStringList authors, QString comment)
{
    this->path = path;
    this->authors = authors;
    this->comments = comment;

    return *this;
}

CheckoutProcessPluginInterface &CheckoutProcessPluginInterface::addDependency(QString dep)
{
 _dependencies << dep;
 return *this;
}

CheckoutProcessPluginInterface &CheckoutProcessPluginInterface::addDependencies(QStringList dep)
{
    _dependencies.append(dep);
    return *this;
}

CheckoutProcessPluginInterface &CheckoutProcessPluginInterface::addPostProcessScreen(QStringList dep)
{
    for (auto& d : dep)  _multi_postprocess << d;
    return *this;
}

CheckoutProcessPluginInterface &CheckoutProcessPluginInterface::addPostProcessScreen(QString d)
{
    _multi_postprocess << d;
    return *this;
}




CheckoutProcessPluginInterface &CheckoutProcessPluginInterface::addPostProcess(QStringList dep)
{
    for (auto& d : dep)  _postprocess << d;
    return *this;
}

CheckoutProcessPluginInterface &CheckoutProcessPluginInterface::addPostProcess(QString d)
{
    _postprocess << d;
    return *this;
}



void CheckoutProcessPluginInterface::write(QJsonObject &json) const
{
    QJsonArray params;
    foreach (const RegistrableParent* regs, _parameters) {
        QJsonObject param;
        regs->write(param);
        params.append(param);
    }

    QJsonArray ret;
    foreach (const RegistrableParent* regs, _results) {
        QJsonObject param;
        regs->write(param);
        ret.append(param);
    }

    json["Path"] = path;
    json["PluginVersion"] = plugin_version();
    json["authors"] = QJsonArray::fromStringList(authors);

    json["Comment"] = comments;
    auto deps = QSet<QString>(_dependencies.begin(), _dependencies.end());
    json["Dependencies"] =  QJsonArray::fromStringList(QStringList(deps.begin(), deps.end()));

    json["PostProcesses"] = QJsonArray::fromStringList(_postprocess);
    json["PostProcessesScreen"] = QJsonArray::fromStringList(_multi_postprocess);

    json["Parameters"] = params;
    json["ReturnData"] = ret;
    json["State"] = QString(_state == Running ? "Running" :
                           (_state == Finished ? "Finished" :
                           (_state == Crashed ? "Crashed": "NotStarted"))); // Not allowed to change state here
    json["Pos"] = getPosition();

    json["shallDisplay"] =  _callParams["shallDisplay"];
    json["ProcessStartId"] =  _callParams["ProcessStartId"];
    json["StartTime"] = _callParams["StartTime"];

    json["ReplyTo"] = _callParams["ReplyTo"];

    //  qDebug() << "###### StartTime" << json["StartTime"];
    if (_state == Finished)
        json["Result"] = _result;


}

CheckoutProcessPluginInterface::~CheckoutProcessPluginInterface()
{

}

int getKeyFromJSON(QString key, QJsonObject ob)
{
    int min = std::numeric_limits<int>::max();
    if (ob.contains("Data"))
    {
        if (ob["Data"].isArray())
        {
            QJsonArray arr = ob["Data"].toArray();
            min = getKeyFromJSON(key, arr.at(0).toObject());
            for (int i = 1; i < arr.size(); ++i)
            {
                int v = getKeyFromJSON(key, arr.at(i).toObject());
                if  (v != min) min = -1;
            }
        }
        else if (ob["Data"].isObject())
        {
            QJsonObject obj = ob["Data"].toObject();
            for (auto i = obj.begin(), e = obj.end(); i != e; ++i)
            {
                int v = getKeyFromJSON(key, i.value().toObject());
                if (v != min) min = -1;
            }
        }
    }
    if (ob.contains(key))
    {
        if (ob[key].isArray()) min = std::min(min, ob[key].toArray().size() == 1 ? ob[key].toArray().at(0).toInt() : -1);
        else min = std::min(min, ob[key].toInt());
    }
    return min;
}


void CheckoutProcessPluginInterface::prepareData()
{

    //qDebug() << "Plugin Prepare data" << _callParams.keys();

    QString hash = _callParams["Process_hash"].toString();
    //  qDebug() << "Process hash func: " << hash;
    int p = 0;
    foreach ( RegistrableParent* regs, _parameters)
        if (regs)
    {
        QString dhash = QString("%1%2").arg(hash).arg(p++);
        regs->setHash(dhash);
        RegistrableImageParent* im = dynamic_cast< RegistrableImageParent*>(regs);
        if (im)
        {
            QJsonArray params = _callParams["Parameters"].toArray();
            for (int i = 0; i < params.size(); ++i)
            {
                QJsonObject o = params[i].toObject();
                if (o.contains("bias"))
                {
                    auto bar = o["bias"].toArray();
                    QFileInfo dir(o["BasePath"].toString());
                    QString base_path = dir.absolutePath();
                    QStringList bfile;
                    for (int i = 0; i < bar.size(); ++i)
                        bfile << base_path + "/" + bar[i].toString();
                    im->setBiasFiles(bfile);
                }


                if (o["Tag"] == im->tag())
                {
                    //                  qDebug() << o;
                    im->storeJson(o);
                    if (im->imageAutoloading())
                    {
                        im->loadImage( o);
                        if (im->shallUnbias())
                        {

                            //                            this->getBiasField(0);

                        }
                    }
                    InputImageMetaData meta;


                    meta.file_path  = im->basePath(o);
                    meta.hash       = o["DataHash"].toString();
                    meta.pos        = o["Pos"].toString();
                    meta.fieldId    = getKeyFromJSON("FieldId" , o);//o["FieldId"].toInt();
                    meta.TimePos    = getKeyFromJSON("TimePos" , o);//o["TimePos"].toInt();
                    meta.zPos       = getKeyFromJSON("zPos"    , o); //o["zPos"].toInt();
                    meta.channel    = getKeyFromJSON("Channels", o);// o["Channels"].toArray().first().toInt();
                    meta.commitName = o["CommitName"].toString();

                    //                  qDebug() << meta.fieldId << meta.TimePos << meta.zPos << meta.channel << o;
                    _meta.push_back(meta);

                    break;
                }
            }
        }

    }
        else
        {
            qDebug() << "Warning registered parameter doesn't exist";
        }
    {
        QMutexLocker locker(&mutex);
        if (_meta.size())
        {
            //            qDebug() << "Expected to find data as input Please debug" << getPath();
            _hashtoBiasCount[_meta.first().hash]++;
            //qDebug() << "number of workers on " << hash << _hashtoBiasCount[_meta.first().hash];
        }
        else
            qDebug() << "Expected to find data as input Please debug" << getPath();

    }

    foreach ( RegistrableParent* regs, _results)
    {
        QString dhash = QString("%1%2").arg(hash).arg(p++);
        regs->setHash(dhash);
    }
}


// Call this routinely from inside the plugin to feedback status information
// To the end user (usefull for lengthy processes)
void CheckoutProcessPluginInterface::statusMessage(float overall, QString message, float step, QString stepMessage)
{
    _message = message;
    _stepMessage = stepMessage;

    this->overallEvolution = overall;
    this->stepEvolution = step;
}

void CheckoutProcessPluginInterface::printMessage(QString message)
{
    _infos << message;
    mutex.lock();
    {
        qDebug() << (quint64)QThread::currentThreadId() << message;
    }
    mutex.unlock();
}

void CheckoutProcessPluginInterface::setStatusMessage(QJsonObject ob)
{
    _message = ob["Message"].toString();
    _stepMessage = ob["StepMessage"].toString();
    overallEvolution = ob["Evolution"].toDouble();
    stepEvolution = ob["stepEvolution"].toDouble();

}

void CheckoutProcessPluginInterface::finished()
{
    QMutexLocker locker(&mutex);

    _state = Finished;
    if (_meta.size())
    {
        QString hash = _meta.front().hash;
        _hashtoBiasCount[hash]--;
        if (_hashtoBiasCount[hash] == 0)
        {
            //qDebug() << hash << "finished";
            for (int i = 0; i < _hashtoBias[hash].size(); ++i)
                delete _hashtoBias[hash].at(i);
            _hashtoBias.remove(hash);
            _hashtoBiasCount.remove(hash);
        }
    }

}

void CheckoutProcessPluginInterface::started(qint64 time)
{
    _state = Running;
    _result["LoadingTime"] = QString("%1").arg(time);
}

CheckoutProcessPluginInterface::State CheckoutProcessPluginInterface::processState() { return _state; }

void CheckoutProcessPluginInterface::crashed() {
    _state = Crashed;
}

bool CheckoutProcessPluginInterface::isFinished()
{
    return (_state == Finished) || (_state == Crashed);
}

#include <RegistrableImageType.h>

void CheckoutProcessPluginInterface::setColormap(void *data, CheckoutProcessPluginInterface::Colormap color)
{
    for (auto p : _results)
    {
        RegistrableImageParent* im = dynamic_cast<RegistrableImageParent*>(p);
        if (im && im->hasData(data))
        {
            im->setColormap(color);
        }
    }
}

QString CheckoutProcessPluginInterface::user()
{
    return _callParams["Username"].toString()+ "@" +  _callParams["Computer"].toString();
}


QString CheckoutProcessPluginInterface::getDataStorePath()
{
    return CheckoutProcess::handler().getStoragePath();
}



QJsonObject CheckoutProcessPluginInterface::createStatusMessage()
{
    QJsonObject ob;

    if (_infos.size() > 1)
    {
        ob["Infos"] = QJsonArray::fromStringList(_infos);
        mutex.lock();
        {
            qDebug() << ob["Infos"];
        }
        mutex.unlock();
        _infos.clear();
        _infos << QString("[%1]").arg((quint64)QThread::currentThreadId());
    }

    ob["Message"] = _message;
    ob["StepMessage"] = _stepMessage;
    ob["Evolution"] = overallEvolution;
    ob["stepEvolution"] = stepEvolution;
    ob["State"] = QString(_state == Running ? "Running" : (_state == Finished ? "Running" : "NotStarted")); // not allowed to change state to finished here
    ob["Pos"] = getPosition();

    if (_state == Finished)
        ob["Result"] = _result;

    return ob;
}


#include <checkoutprocess.h>

// FIXME: properly handle list/vector/map data
QJsonObject CheckoutProcessPluginInterface::gatherData(qint64 time)
{
    if (_callParams.isEmpty())
    {
        qDebug() << "Call list parameters empty";
        return QJsonObject();
    }

    QJsonObject ob;
    QJsonArray arr;

    QJsonArray metaArr;
    foreach (InputImageMetaData meta, _meta)
        metaArr.append(meta.toJSON());

    // qDebug() <<"Writing" << _results.size();
    // bool mem = _callParams["LocalRun"].toBool();

    QString hash = _callParams["Process_hash"].toString();
    bool isBatch = false;
    if (_callParams.contains("BatchRun"))
        isBatch = true;

    int c=0;
    foreach (RegistrableParent* p, _results)
    {
        QJsonObject cobj;

        p->setFinished();
        //        p->keepInMem(mem);
        p->setHash(QString("%1%2").arg(hash).arg(c++));  // Set hash for results when live set result maps


        cobj["Process"] = path;
        cobj["Meta"] = metaArr;
        cobj["Data"] = p->toString();

        if (isBatch)// correct results for dynamic generation
            p->setAsOptional(false);

        p->write(cobj);

        // qDebug() << "Payload" << cobj["Payload"];

        cobj.remove("Comment");
        cobj.remove("CoreProcess_hash");
        arr.append(cobj);
    }

    ob["Infos"] = QJsonArray::fromStringList(_infos);
    ob["LoadingTime"] = _result["LoadingTime"];
    ob["Data"] = arr;
    ob["Path"] = getPath();
    ob["ElapsedTime"] = QString("%1").arg(time);
    ob["State"] = QString(_state == Running ? "Running" : (_state == Finished ? "Finished" : "NotStarted"));
    ob["Pos"] = getPosition();

    ob["shallDisplay"] = _shallDisplay;
//    qDebug() << "In plugin processStartId" << processStartId << _callParams["ProcessStartId"];


    ob["ProcessStartId"] = processStartId;
    auto d = QStringList() << "XP" << "DataHash" << "CommitName" << "ReplyTo"
                           << "Parameters" << "StartTime" << "TaskID" << "WorkID"
                           << "WellTags" << "PostProcesses" << "PostProcessesScreen"
                           << "Process_hash" << "Project" << "Computer" << "Username"
                           << "ThreadID" << "Client";
    for (auto & s: d)
        ob[s] = _callParams[s];


    if (_state == Finished)
        ob["Result"] = _result;

    return ob;
}

void CheckoutProcessPluginInterface::read(const QJsonObject &json)
{
    _callParams = json;
    if (path != json["Path"].toString())
   {
        qDebug() << "Warning algorithm is not properly called, wrong parameters"
                 << path << "vs" << json["Path"].toString();
        return;
    }
    //   qDebug() << "#### StartTime" << _callParams["StartTime"];

    setPosition(json["Pos"].toString());
    _shallDisplay = json["shallDisplay"].toBool();
    processStartId =  json["ProcessStartId"].toInt();
    qDebug() << "processStartId" << processStartId;

    QJsonArray params = json["Parameters"].toArray();
    for (int i = 0; i < params.size(); ++i)
    {

//        qDebug() << params[i].toObject()["Tag"] << params[i].toObject()["Value"];
        RegistrableParent* regs = _parameters[params[i].toObject()["Tag"].toString()];
        //qDebug() << params[i].toObject();
        if (!regs) qDebug() << "Error getting algorithm parameters" << params[i].toObject()["Tag"];
        else regs->read(params[i].toObject());
    }


    params = json["ReturnData"].toArray();
    for (int i = 0; i < params.size(); ++i)
    {
        RegistrableParent* regs = _results[params[i].toObject()["Tag"].toString()];
        if (!regs) qDebug() << "Error getting algorithm parameters" << params[i].toObject()["Tag"];
        else regs->read(params[i].toObject());
    }

    QJsonArray arr = json["Parameters"].toArray(), rra;
    for (int i = 0; i < arr.size(); ++i)
    {

        QJsonObject ob = arr.at(i).toObject();

        QJsonObject r;


        if (ob.contains("Tag"))
            r["Tag"] = ob["Tag"];
        if (ob.contains("Value"))
            r["Value"] = ob["Value"];
        if (ob.contains("Channels"))
            r["Channels"] = ob["Channels"];
        if (ob.contains("Data"))
            r["Data"] = ob["Data"];

        if (ob.contains("isOptional"))
            r["isOptional"] = ob["isOptional"];
        if (ob.contains("optionalState"))
            r["optionalState"] = ob["optionalState"];

        rra.push_back(r);


    }

    json["Parameters"] = rra;
    arr = json["ReturnData"].toArray();
    for (int i = 0; i < arr.size(); ++i)
    {

        QJsonObject ob = arr.at(i).toObject();

        QJsonObject r;


        if (ob.contains("Tag"))
            r["Tag"] = ob["Tag"];
        if (ob.contains("Value"))
            r["Value"] = ob["Value"];
        if (ob.contains("Channels"))
            r["Channels"] = ob["Channels"];
        if (ob.contains("Data"))
            r["Data"] = ob["Data"];

        if (ob.contains("isOptional"))
            r["isOptional"] = ob["isOptional"];
        if (ob.contains("optionalState"))
            r["optionalState"] = ob["optionalState"];

        rra.push_back(r);


    }
    json["ReturnData"] = rra;


}

QString CheckoutProcessPluginInterface::getPosition() const
{
    return position;
}

void CheckoutProcessPluginInterface::setPosition(const QString &value)
{
    position = value;
}


QDebug operator<<(QDebug debug, const ChannelSelectionType &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Channel: " << c();

    return debug;
}
