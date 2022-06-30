﻿#include "networkprocesshandler.h"
#include <QMessageBox>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
//#include <QtConcurrent>
#include <Core/networkmessages.h>
#include <Core/checkoutprocess.h>
#include <Core/config.h>

#include "qhttp/qhttpclient.hpp"
#include "qhttp/qhttpclientrequest.hpp"
#include "qhttp/qhttpclientresponse.hpp"
#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"

using namespace qhttp::client;

#undef signals
#include <arrow/api.h>
#include <arrow/filesystem/filesystem.h>
#include <arrow/ipc/writer.h>
#include <arrow/util/iterator.h>


#include <QMutex>
#include <QMutexLocker>
#include <QtConcurrent>


namespace fs = arrow::fs;

struct DataFrame
{
    QString outfile;
    QString plate;

    QMap<QString, QString> fuseT;
    QMap<QString, QMap<QString, float   >  > arrFl;
    QMap<QString, QMap<QString, QString > > arrStr;
};


CheckoutHttpClient::CheckoutHttpClient(QString host, quint16 port):  awaiting(false), icpus(0), collapse(true), run(true)
{
    iurl.setScheme("http");
    iurl.setHost(host);
    iurl.setPort(port);

    QtConcurrent::run(this, &CheckoutHttpClient::sendQueue);

    //    startTimer(500);
}

QMutex mutex_send_lock;
CheckoutHttpClient::~CheckoutHttpClient()
{
    //    qDebug() << "CheckoutHttpClient : I've been killed";
    mutex_send_lock.lock();
    run = false;
    mutex_send_lock.unlock();
}

void CheckoutHttpClient::send(QString path, QString query)
{
    send(path, query, QByteArray(), true);
}

void CheckoutHttpClient::send(QString path, QString query, QJsonArray ob, bool keepalive)
{
    //auto body = QCborValue::fromJsonValue(ob).toCbor();
    //    qDebug() << "Generating cbor array" << ob.size() << QCborArray::fromJsonArray(ob).size();

    auto body = QCborArray::fromJsonArray(ob).toCborValue().toCbor();

    qDebug() << QCborValue::fromCbor(body).toArray().size();


    //    qDebug() << "CBOR array size" << body.size();

    QUrl url=iurl;
    url.setPath(path);
    url.setQuery(query);

    //     qDebug() << "Creating Query" << url.toString() << ob;
    send(path, query, body, keepalive);
}


void CheckoutHttpClient::send(QString path, QString query, QByteArray ob, bool keepalive)
{
    QMutexLocker lock(&mutex_send_lock);

    QUrl url=iurl;
    url.setPath(path);
    url.setQuery(query);

    reqs.append(Req(url, ob, keepalive));
    //    sendQueue();
}

void CheckoutHttpClient::sendQueue()
{
    //

    //    iclient.setTimeOut(5000);

    while (run)
    {
        //        QMutexLocker lock(&mutex_send_lock);
        mutex_send_lock.lock();

        if (reqs.isEmpty())
        {
            mutex_send_lock.unlock();

            QThread::msleep(100);
            qApp->processEvents();
            continue;
        }


        auto req = reqs.takeFirst();
        QUrl url = req.url;
        auto& ob = req.data;

        mutex_send_lock.unlock();

        // Collapse request url
        QList<int> collapsed;
        if (collapse)
            for (int i = 0; i < reqs.size(); ++i)
                if (reqs.at(i).url == url && (
                            url.path().startsWith("/addData/") ||
                            url.path().startsWith("/Start")  ||
                            url.path().startsWith("/Ready") ||
                            url.path().startsWith("/ServerDone")
                            ) )
                    collapsed << i;

        if (collapsed.size() > 0)
        {
            qDebug() << "Collapsing responses (0 + " << collapsed << ")";
            QJsonArray ar = QCborValue::fromCbor(ob).toJsonValue().toArray();

            for (auto& i: collapsed)
            {
                auto r = reqs.at(i);
                QJsonArray a2 = QCborValue::fromCbor(r.data).toJsonValue().toArray();
                for (int i = 0; i < a2.size(); ++i)
                {
                    ar.append(a2.at(i));
                }
            }

            for (QList<int>::reverse_iterator it = collapsed.rbegin(), e = collapsed.rend();
                 it != e; ++it)
                reqs.removeAt(*it);

            ob = QCborValue::fromJsonValue(ar).toCbor();
        }

        qDebug() << QDateTime::currentDateTime().toString() << "Sending Queued" << url.url() << ob.size();


        iclient.push_back(new QHttpClient());


        QObject::connect(iclient.back(), &QHttpClient::disconnected, [this]() {
            finalize();
        });

        iclient.back()->request(
                    qhttp::EHTTP_POST,
                    req.url,
                    [this, ob](QHttpRequest* req){
            Q_UNUSED(this);
            auto body = ob;
            if (req)
            {

                req->addHeader("Content-Type", "application/cbor");
                req->addHeaderValue("content-length", body.size());
                req->end(body);
                //                qDebug() << "Generated request" << body.size();
            }
            else
                qDebug() << "Queue Request Error...";
        },

        [this](QHttpResponse* res) {
            if (res)
            {
                res->collectData();
                res->onEnd([this, res]() {
                    onIncomingData(res->collectedData());

                });
            }
        });

        if (iclient.back()->tcpSocket()->error() >= 0)
            qDebug() << "Error Send" << iclient.back()->tcpSocket()->errorString();

    }

}

void CheckoutHttpClient::timerEvent(QTimerEvent * )
{
    sendQueue();
}

void CheckoutHttpClient::onIncomingData(const QByteArray& data)
{
    auto val = QCborValue::fromCbor(data).toJsonValue();
    auto ob = val.toObject();


    if (ob.contains("CPU"))
        icpus = ob["CPU"].toInt();

    if (ob.contains("Processes"))
    {
        NetworkProcessHandler::handler().setProcesses(ob["Processes"].toArray(), this);
        return;
    }
    if (ob.contains("authors"))
    {
        NetworkProcessHandler::handler().setParameters(ob);
        return;
    }
    if (ob.contains("ArrayCoreProcess") &&
            ob.contains("ArrayRunProcess"))
    {
        QJsonArray Core = ob["ArrayCoreProcess"].toArray();
        QJsonArray Run = ob["ArrayRunProcess"].toArray();

        NetworkProcessHandler::handler().handleHashMapping(Core, Run);
        return;
    }

    qDebug()  << "HTTP Response" << val;
}

void CheckoutHttpClient::finalize()
{
    QMutexLocker lock(&mutex_send_lock);
    auto ic = static_cast<QHttpClient*>(sender());
    iclient.removeAll(ic);
}



QTextStream *hash_logfile = nullptr;

inline QDataStream& operator<< (QDataStream& out, const std::vector<unsigned char>& args)
{
    out << (quint64)args.size();
    for (const auto& val : args )
    {
        out << val;
    }
    return out;
}

inline QDataStream& operator>> (QDataStream& in, std::vector<unsigned char>& args)
{
    quint64 length=0;
    in >> length;
    args.clear();
    args.reserve(length);
    for (quint64 i = 0 ; i < length ; ++i)
    {
        typedef typename std::vector<unsigned char>::value_type valType;
        valType obj;
        in >> obj;
        args.push_back(obj);
    }
    return in;
}


NetworkProcessHandler::NetworkProcessHandler():
    _waiting_Update(false),
    last_serv_pos(0)
{

    data = new QFile( QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/HashLogs.txt");
    if (data->open(QFile::WriteOnly | QFile::Truncate)) {
        hash_logfile= new QTextStream(data);
    }

    QList<QHostAddress> list = QNetworkInterface::allAddresses();

    for(QHostAddress& addr: list)
    {
        if(!addr.isLoopback())
            if (addr.protocol() == QAbstractSocket::IPv4Protocol )
                srv = QString("_%1").arg(addr.toString().replace(".", ""));
        if (!srv.isEmpty()) break; // stop on non empty
    }
}

NetworkProcessHandler::~NetworkProcessHandler()
{
    delete data;
    delete hash_logfile;
}

NetworkProcessHandler &NetworkProcessHandler::handler()
{
    static NetworkProcessHandler* handler = 0;
    if (!handler)
        handler = new NetworkProcessHandler();

    return *handler;
}

void NetworkProcessHandler::establishNetworkAvailability()
{
    // Go through configured parameters
    QHostInfo info;
    QSettings sets;

    QJsonArray data;

    QStringList var = sets.value("Server", QStringList() << "127.0.0.1").toStringList();
    QList<QVariant> ports = sets.value("ServerP", QList<QVariant>() << QVariant((int)13378)).toList();

    for (int i = 0; i < var.size(); ++i)
    {
        QJsonObject localh;
        localh["Host"] = var.at(i);
        localh["Port"] = ports.at(i).toInt();

        data.append(localh);
    }

    if (activeHosts.size() == data.size())
        return;
    foreach (QJsonValue ss, data)
    {
        QJsonObject o = ss.toObject();
        auto h = new CheckoutHttpClient(o["Host"].toString(), o["Port"].toInt());

        h->send("/ListProcesses");
        activeHosts << h;

        qDebug() <<"Process from server"<< h->iurl;
    }


}

void NetworkProcessHandler::setNoProxyMode()
{
    srv=QString();
}

void NetworkProcessHandler::addProxyPort(uint16_t port) {
    srv = QString("%1%2").arg(srv).arg(port);
}




void NetworkProcessHandler::setProcesses(QJsonArray ar, CheckoutHttpClient* cl)
{
    //    qDebug() << "Settings Processes";

    for (int p = 0; p < ar.size(); ++p)
    {
        QString pr=    ar[p].toString();
        QList<CheckoutHttpClient*>& lp = procMapper[pr];
        if (!lp.contains(cl))
        {
            //   qDebug() << pr;
            procMapper[pr] << cl;
        }
    }

    CheckoutProcess::handler().updatePath();
}


QStringList NetworkProcessHandler::getProcesses()
{
    QStringList l;

    l =  procMapper.keys();
    // qDebug() << "Network Handler list: " << l;
    return l;
}

void NetworkProcessHandler::getParameters(QString process)
{
    // Try to connect to any host having the same process
    if (procMapper[process].isEmpty())
    {
        qDebug() << "Empty process List" << process;
        return;
    }
    CheckoutHttpClient* h = procMapper[process].first();
    if (!h)
    {
        qDebug() << "getParameters: Get Network process handler";
        return;
    }

    qDebug() << "Query Process details" << process << h->iurl;

    h->send(QString("/Process/%1").arg(process));
}


void NetworkProcessHandler::setParameters(QJsonObject ob)
{
    qDebug() << ob;
    emit parametersReady(ob);
}

void NetworkProcessHandler::startProcess(QString process, QJsonArray ob)
{
    QList<CheckoutHttpClient*> procsList = procMapper[process];

    // Try to connect to any host having the same process
    if (procsList.isEmpty())
    {
        qDebug() << "Empty process List" << process;
        return;
    }

    int itemsPerServ = (int)ceil(ob.size() / (0.0 + procsList.size()));
    qDebug() << "Need to dispatch: " << ob.size() << "item to process " << process << "found on "  << procsList.size() << "services";
    qDebug() << "Pushing " << itemsPerServ << "processes to each server";
    // Perform server order reorganisation to ensure proper dispatch among servers
    last_serv_pos = last_serv_pos % procsList.size();

    qDebug() << "Starting with server" << last_serv_pos;

    for (int i = 0; i < last_serv_pos; ++i)
        procsList.push_back(procsList.takeFirst());

    //    foreach (CheckoutHost* h, procsList)
    //        qDebug() << h->address << h->port;

    int lastItem = 0;
    if (procsList.size() == 1)
    {
        CheckoutHttpClient* h = procsList.front();

        for (int i = 0; i < ob.size(); i++)
        {
            QJsonObject t = ob.at(i).toObject();
            (*hash_logfile) << "Started Core "<< t["CoreProcess_hash"].toString() << Qt::endl;
            runningProcs[t["CoreProcess_hash"].toString()] = h;
        }

        startProcess(h, process, ob);
    }
    else
        foreach(CheckoutHttpClient* h, procsList)
        {
            QJsonArray ar;
            for (int i = 0; i < itemsPerServ && lastItem < ob.size(); ++i, ++lastItem)
            {
                ar.append(ob.at(lastItem));
                QJsonObject t = ar.at(i).toObject();
                //            qDebug() << t["CoreProcess_hash"].toString();
                (*hash_logfile) << "Started Core "<< t["CoreProcess_hash"].toString() << Qt::endl;
                runningProcs[t["CoreProcess_hash"].toString()] = h;
            }
            //        qDebug() << "Starting" << h->address << h->port << ar.size();
            if (ar.size())
            {
                static const int subs = 10;
                for (int i = 0; i < ar.size(); i += subs)
                {
                    QJsonArray sub;
                    for (int l = 0; l < subs && i+l < ar.size(); ++l)
                        sub.append(ar.at(i+l));
                    startProcess(h, process, sub);
                }
                last_serv_pos++;
            }
        }
}

void NetworkProcessHandler::startProcess(CheckoutHttpClient *h, QString process, QJsonArray ob)
{
    h->send(QString("/Start/%1").arg(process), QString(), ob);
}


// Modified Core/Server Exchange to better handle heavy interaction (here high network load)
// Large image sets are at risk for the handling of data this may lock the user interface with heavy processing
QJsonArray ProcessMessageStatusFunctor(CheckoutHost* h, QList<QString > hash, NetworkProcessHandler* owner)
{
    Q_UNUSED(h);
    Q_UNUSED(hash);
    Q_UNUSED(owner);

    return QJsonArray();
}


// The Process message status can be a bit long, though the current network approach is not at it's best handling
// the message at threaded approach would be better of handling this network information
void NetworkProcessHandler::getProcessMessageStatus(QString process, QList<QString > hash)
{
    Q_UNUSED(process);
    Q_UNUSED(hash);
}


QJsonArray NetworkProcessHandler::filterObject(QString hash, QJsonObject ds, bool last_one)
{
    QJsonArray res;
    QJsonObject ob;
    ob["hash"] = hash;


    //    qDebug() << ds;
    auto l = QStringList() << "TaskID" << "DataHash" << "WorkID";
    for (auto& k : l) if (ds.contains(k)) ob[k] = ds[k];

    QString plate = ds["XP"].toString().replace("\\","/").replace("/",""), commit = ds["CommitName"].toString();
    QString plateID = plate + commit;


    if (!plateData.contains(plateID))
        plateData.insert(plateID, new DataFrame);

    DataFrame& store =  *plateData[plateID];

    if (store.outfile.isEmpty()&&!commit.isEmpty())
    {
        QSettings set;
        QString dbP = set.value("databaseDir", "L:").toString();
#ifndef  WIN32
        if (dbP.contains(":"))
            dbP = QString("/mnt/shares/") + dbP.replace(":","");
#endif

        dbP.replace("\\", "/").replace("//", "/");

        store.outfile = QString("%1/PROJECTS/%2/Checkout_Results/%3/%4%5.fth").arg(dbP,
                                                                                   ds["Project"].toString(),
                commit, plate, srv).replace("\\", "/").replace("//", "/");
        store.plate = plate;
    }


    std::map<QString, QString> tr={ {"FieldId", "fieldId"}, {"zPos", "sliceId"},
                                    {"TimePos", "timepoint"}, {"Channel", "channel"}, {"Pos", "Well"}, {"WellTags", "tags"}};


    auto txt = QStringList() << "Pos" << "TimePos" << "FieldId" << "zPos" << "Channel"  ;


    if (ds.contains("Data"))
    {
        auto arr = ds["Data"].toArray(); // Theorically at this level only one Data in this table
        for (auto itd : arr)
        {
            auto obj = itd.toObject();
            if (obj["Data"].toString() != "Image results" )
            {

                QString dfKey;

                if (obj.contains("Meta"))
                {
                    QJsonObject met;
                    if (obj["Meta"].isArray())
                        met = obj["Meta"].toArray()[0].toObject();
                    if (obj["Meta"].isObject())
                        met = obj["Meta"].toObject();


                    ob["DataHash"] = met["DataHash"];

                    for (auto& s: txt)
                        if (met.contains(s))
                        {
                            auto v =  ds.contains(s) ? ds[s] : met[s];

                            if (commit.isEmpty())
                                ob[s] =  v;
                            else
                                dfKey += (v.isString() ? v.toString() : QString("%1").arg((int)v.toDouble())) + ";" ;
                        }
                        else
                        {
                            if (!commit.isEmpty())
                                dfKey += "-1;";
                        }
                }

                QString name = obj["Tag"].toString();

                if (!commit.isEmpty())
                {
                    // qDebug() << "Appending" << name << dfKey << obj["Data"];
                    if (obj["Data"].isDouble())
                        store.arrFl[name][dfKey] = obj["Data"].toDouble();
                    else
                        store.arrFl[name][dfKey] = obj["Data"].toString().toDouble();


                    store.fuseT[name]=obj["Aggregation"].toString();
                    store.arrStr["tags"][dfKey]=ds["WellTags"].toString();
                }
                else
                {
                    ob[name] = obj["Data"];
                    ob[QString("%1_Agg").arg(name)] = obj["Aggregation"];
                }
            }
        }
    }

    res << ob;

    //storageTimer.values().indexOf(plate)
    if (rstorageTimer.contains(plateID))
        killTimer(rstorageTimer[plateID]);

    int timer = startTimer(30000); // reset the current timer to 30s since last modification

    storageTimer[timer] = plateID;
    rstorageTimer[plateID] = timer;

    if (CheckoutProcess::handler().numberOfRunningProcess() <= 0 || last_one) // Directly save if no more process running
    {
        for (auto & k: rstorageTimer) killTimer(k); // End timers
        for (auto &k: storageTimer)   storeData(k, true); // perfom storage

        // Cleanup
        storageTimer.clear();
        rstorageTimer.clear();
    }


    return res;
}

#include <iostream>
#define ABORT_ON_FAILURE(expr)                     \
    do {                                             \
    arrow::Status status_ = (expr);                \
    if (!status_.ok()) {                           \
    std::cerr << status_.message() << std::endl; \
    abort();                                     \
    }                                              \
    } while (0);


void exportBinary(QJsonObject& ds, QJsonObject& par, QCborMap& ob) // We'd like to serialize this one
{



    QString plate = ds["XP"].toString(), commit = ds["CommitName"].toString();

    if (!commit.isEmpty() && par.contains(QString("SavePath")) && !par.value("SavePath").toString().isEmpty())
    {
        QString pos = par["Meta"].toArray().first().toObject()["Pos"].toString();

        QString tofile = QString("%1/%2/%3_%5_%4.fth").arg(par.value("SavePath").toString(),
                                                           commit, plate,
                                                           par.value("Tag").toString(),
                                                           pos);
        //        qDebug() << "Saving Generated Meta to" << tofile;
        {
            auto datasizes = ob.value("DataSizes").toArray();
            size_t size = 0;
            for (int i = 0; i < datasizes.size(); ++i)
                size += datasizes.at(i).toInteger();

            std::vector<unsigned char> data(size);

            auto vdata = ob.value(QCborValue("BinaryData")).toArray();

            size_t pos = 0;
            for (auto t: vdata)
            {
                auto temp = t.toByteArray();
                for (int i = 0; i < temp.size(); ++i, ++pos)
                    data[i]=temp.data()[i];
            }

            int r = ob.value(QCborValue("Rows")).toInteger(), c = ob.value(QCborValue("Cols")).toInteger();
            int cvtype = ob.value(QCborValue("cvType")).toInteger();
            cv::Mat feat(r, c, cvtype, (data.data()));


            std::vector<std::shared_ptr<arrow::Field> > fields;

            QStringList cols;  for (auto c : par["ChannelNames"].toArray())  cols << c.toString();

            for (int c = 0; c < feat.cols; ++c)
                if (c < cols.size())
                    fields.push_back(arrow::field(cols[c].toStdString(), arrow::float32()));
                else
                    fields.push_back(arrow::field((QString("%1").arg(c)).toStdString(), arrow::float32()));

            fields.push_back(arrow::field("tags", arrow::utf8()));

            std::vector<std::shared_ptr<arrow::Array> > dat(fields.size());

            for (int c = 0; c < feat.cols; ++c)
            {
                arrow::NumericBuilder<arrow::FloatType> bldr;
                for (int r = 0; r <feat.rows; ++r)
                {
                    ABORT_ON_FAILURE(bldr.Append(feat.at<float>(r,c)));

                }
                ABORT_ON_FAILURE(bldr.Finish(&dat[c]));
            }

            arrow::StringBuilder bldr;
            for (int r = 0; r <feat.rows; ++r)
            {
                ABORT_ON_FAILURE(bldr.AppendNull());
            }
            ABORT_ON_FAILURE(bldr.Finish(&dat[feat.cols]));
            auto schema =
                    arrow::schema(fields);
            auto table = arrow::Table::Make(schema, dat);
            std::string uri = tofile.toStdString();
            std::string root_path;

            auto r0 = fs::FileSystemFromUriOrPath(uri, &root_path);
            if (!r0.ok())
            {
                qDebug() << "Arrow Error not able to load" << tofile;
                return;
            }

            auto fs = r0.ValueOrDie();

            auto r1 = fs->OpenOutputStream(uri);

            if (!r1.ok())
            {
                qDebug() << "Arrow Error to Open Stream";
                return;
            }
            auto output = r1.ValueOrDie();
            arrow::ipc::IpcWriteOptions options = arrow::ipc::IpcWriteOptions::Defaults();
            //        options.codec = arrow::util::Codec::Create(arrow::Compression::LZ4).ValueOrDie(); //std::make_shared<arrow::util::Codec>(codec);

            auto r2 = arrow::ipc::MakeFileWriter(output.get(), table->schema(), options);

            if (!r2.ok())
            {
                qDebug() << "Arrow Unable to make file writer";
                return;
            }

            auto writer = r2.ValueOrDie();

            writer->WriteTable(*table.get());
            writer->Close();

        }
    }
}

QCborArray NetworkProcessHandler::filterBinary(QString hash, QJsonObject ds)
{

    //    QString tmp = QJsonDocument(ds).toJson();

    QCborArray res;
    QString commitName = ds["CommitName"].toString();

    //    qDebug() << "Filter Binary" << ds.keys() << ds["ProcessStartId"];

    if (ds.contains("Data"))
    {
        auto arr = ds["Data"].toArray();
        for (auto itd : arr)
        {


            auto obj = itd.toObject();

            //            qDebug() << "Filtering" << obj["Tag"] << obj.keys();
            if (obj["Data"].toString() == "Image results" && obj.contains("Payload"))
            {


                QCborMap ob;

                QString dhash;

                if (ds.contains("DataHash"))  dhash = ds["DataHash"].toString();
                if (obj.contains("Meta"))
                {
                    auto met = obj["Meta"].toArray()[0].toObject();
                    auto txt = QStringList() << "FieldId" << "Pos" << "TimePos"
                                             << "Channel" << "zPos" ;
                    for (auto s: txt)
                        if (met.contains(s))
                        {
                            if (met[s].isDouble() && met[s].toInt() < 0 )    continue;

                            ob.insert(QCborValue(s), QCborValue::fromJsonValue(met[s]));
                        }
                    if (met.contains("DataHash"))
                        dhash = met["DataHash"].toString();

                }
                else
                {
                    qDebug() << "######################################" << Qt::endl
                             <<"No Meta" << Qt::endl
                            << obj;
                    continue;
                }

                auto txt = QStringList() << "ContentType" << "ImageType" << "ChannelNames" << "Channel"
                                         << "Tag" << "Pos";
                for (auto& s: txt)
                    if (obj.contains(s))
                    {
                        ob.insert(QCborValue(s), QCborValue::fromJsonValue(obj[s]));
                    }

                //                qDebug() << "Ready data" << ob.toJsonObject();
                // Now add Image info:
                auto ps = obj["Payload"].toArray();
                QCborArray cbar;
                for (auto pp : ps)
                {
                    QCborMap mm;

                    auto pay = pp.toObject();
                    auto ql = QStringList() << "Cols" << "Rows" << "DataSizes" << "cvType" << "DataTypeSize";
                    for (auto s : ql)
                    {
                        if (pay.contains(s))
                            mm.insert(QCborValue(s), QCborValue::fromJsonValue(pay[s]));
                    }
                    //                qDebug() << "Binary ob" << ob;

                    if (pay.contains("DataHash"))
                    {
                        QString dhash = pay["DataHash"].toString();

                        auto buf = CheckoutProcess::handler().detachPayload(dhash);

                        QCborArray ar;
                        size_t pos = 0;
                        try {
                            while (pos < buf.size())
                            {
                                static const size_t mlen = 1073741824; // chunk size
                                size_t end = std::min(mlen, buf.size() - pos);
                                ar.append(QByteArray(reinterpret_cast<const char*>(&buf[pos]), (int)end));
                                pos += end;
                            }
                        }
                        catch (...)
                        {
                            QString msg("Server Out of Memory - Data cannot be transfered");
                            qDebug() << msg;
                            ob.insert(QCborValue("Message"), QCborValue(msg));
                        }



                        qDebug() << "Got binary data: " << dhash << buf.size() << ar.size() << pos;
                        mm.insert(QCborValue("BinaryData"), ar);//QCborValue(data));

                        exportBinary(ds,obj, mm);

                    }
                    else
                    {
                        qDebug() << "Should search data:" << pay << Qt::endl
                                 << "From " << obj;
                    }
                    cbar << mm;
                }
                ob.insert(QCborValue("Payload"), cbar);
                ob.insert(QCborValue("DataHash"), dhash);
                ob.insert(QCborValue("Hash"), hash);
                ob.insert(QCborValue("ProcessStartId"), ds["ProcessStartId"].toInt());

                // Only add the result object if not optional & not a run
                if (!(obj.contains("isOptional") && !obj["optionalState"].toBool()))
                    res << ob;
            }
        }
    }
    return res;
}




void NetworkProcessHandler::finishedProcess(QString hash, QJsonObject res, bool last_one)
{
    QString address=res["ReplyTo"].toString();
    ///    qDebug() << hash << res << address;
    CheckoutHttpClient* client = NULL;

    for (CheckoutHttpClient* cl : alive_replies)
        if (address == cl->iurl.host())
        {
            client = cl ;
        }

    if (!client) { client = new CheckoutHttpClient(address, 8020); alive_replies << client; }

    // else { qDebug() << "Reusing Client"; }
    QString commitname = res["CommitName"].toString();
    if (commitname.isEmpty()) commitname = "Default";
    // now we can setup the reply !

    qDebug() << "Server side Process Finished" << hash;

    QJsonArray data = filterObject(hash, res, last_one);
    QCborArray bin = filterBinary(hash, res);

    for (auto b: bin)
    {
        client->send(QString("/addImage/"), QString(), b.toCbor());
    }

    client->send(QString("/addData/%1").arg(commitname), QString(), data);

}

void NetworkProcessHandler::removeHash(QString hash)
{
    // Just finished a item
    //    qDebug() << "Finished Job" << hash;

    if (runningProcs.remove(hash) == 0)
        qDebug() << "hash " << hash << "not found";

    emit finishedJob(1);
}

void NetworkProcessHandler::removeHash(QStringList hashes)
{
    static QMap<QString, int > olds;
    for (auto& hash : hashes)
    {
        olds[hash]++;
        if (runningProcs.remove(hash) == 0)
            qDebug() << "hash " << hash << "not found" << (olds.contains(hash) ? QString("but was already seen %1").arg(olds[hash]) : QString(""));
    }

    emit finishedJob(hashes.size());
}


void NetworkProcessHandler::exitServer()
{
}

QList<QPair<QString, QJsonArray> > NetworkProcessHandler::erroneousProcesses()
{
    QList<QPair<QString, QJsonArray> > res;

    foreach(auto ev, _error_list)
    {
        res << ev.second;
    }
    return res;
}


void NetworkProcessHandler::handleHashMapping(QJsonArray Core, QJsonArray Run)
{

    if (Core.size() != Run.size())  qDebug() << "Incoherent data size in starting processes...";

    for (int i = 0; i < std::min(Core.size(), Run.size()); ++i)
    {
        QString coreHash = Core.at(i).toString("");
        QString hash = Run.at(i).toString("");

        CheckoutHttpClient* h = runningProcs[coreHash];
        runningProcs.remove(coreHash);
        runningProcs[hash] = h;

        if (hash_logfile)
            (*hash_logfile) << coreHash << "->" << hash << Qt::endl;

        //qDebug() << coreHash << "->" << hash;

        emit processStarted(coreHash, hash);
    }

}

void NetworkProcessHandler::timerEvent(QTimerEvent *event)
{

    //    qDebug() << "Timer event" << event->timerId() << storageTimer.keys();
    if (storageTimer.contains(event->timerId()))
    {
        int timer = event->timerId();
        QString d = storageTimer[timer];
        rstorageTimer.remove(d);
        storageTimer.remove(timer);

        killTimer(timer);

        storeData(d, false);
    }
}

#include <Main/checkout_arrow.h>

void NetworkProcessHandler::storeData(QString d, bool finished)
{

    // Generate the storage for the data of the time
    DataFrame& df = *plateData[d];
    //        QStringList headers =  + df.arrInt.keys() + df.arrStr.keys();


    if (df.outfile.isEmpty())        return;
    QString target = df.outfile ;

    {


        std::vector<std::shared_ptr<arrow::Field> > fields;

        auto txt = QStringList()  << "timepoint" << "fieldId" << "sliceId" << "channel"  ;
        fields.push_back(arrow::field("Well", arrow::utf8()) );
        fields.push_back(arrow::field("Plate", arrow::utf8()) );

        for (auto& k: txt)
            fields.push_back(arrow::field(k.toStdString(), arrow::int16()) );
        for (auto& h: df.arrStr.keys())
        {
            std::shared_ptr<arrow::KeyValueMetadata>  meta = NULLPTR;
            if (df.fuseT.contains(h))
                meta = arrow::KeyValueMetadata::Make({ "Aggregation" }, { df.fuseT[h].toStdString() });
            fields.push_back(arrow::field(h.toStdString(), arrow::utf8(), meta) );
        }
        for (auto& h: df.arrFl.keys())
        {
            std::shared_ptr<arrow::KeyValueMetadata>  meta = NULLPTR;
            if (df.fuseT.contains(h))
                meta = arrow::KeyValueMetadata::Make({ "Aggregation" }, { df.fuseT[h].toStdString() });

            fields.push_back(arrow::field(h.toStdString(), arrow::float32(), meta) );
        }


        QSet<QString> items;
        for (auto& i: df.arrStr.keys())
        {
            auto l = df.arrStr.value(i).keys();
            QSet<QString> k(l.begin(), l.end());
            items.unite(k);
        }

        // items contains now the unique set of keys for each data produced !
        // now we need to iterate through them to generate the data frames

        std::vector<std::shared_ptr<arrow::Array> > dat(fields.size());
        // We need to go columns wise
        {
            arrow::StringBuilder wells,plate;
            arrow::NumericBuilder<arrow::Int16Type> tp,fi,zp,ch;

            for (auto& k: items)
            {
                auto v = k.split(";");
                wells.Append(v[0].toStdString());
                plate.Append(df.plate.toStdString());

                tp.Append(v[1].toInt());
                fi.Append(v[2].toInt());
                zp.Append(v[3].toInt());
                ch.Append(v[4].toInt());
            }

            wells.Finish(&dat[0]);
            plate.Finish(&dat[1]);
            tp.Finish(&dat[2]);
            fi.Finish(&dat[3]);
            zp.Finish(&dat[4]);
            ch.Finish(&dat[5]);

            int dp = 6;
            for (auto& sk : df.arrStr.keys())
            {
                QMap<QString, QString >& mp = df.arrStr[sk];

                arrow::StringBuilder bldr;
                for (auto&  k: items)
                {
                    if (mp.contains(k))
                        bldr.Append(mp.value(k).toStdString());
                    else
                        bldr.AppendNull();
                }
                bldr.Finish(&dat[dp]);
                dp++;
            }

            for (auto& sk : df.arrFl.keys())
            {
                QMap<QString, float >& mp = df.arrFl[sk];

                arrow::NumericBuilder<arrow::FloatType> bldr;
                for (auto&  k: items)
                {
                    if (mp.contains(k))
                        bldr.Append(mp.value(k));
                    else
                        bldr.AppendNull();
                }
                bldr.Finish(&dat[dp]);
                dp++;
            }

        }

        auto schema =
                arrow::schema(fields);
        auto table = arrow::Table::Make(schema, dat);

        qDebug() << "Storing DataFile to:" << target << " Rows " << dat.front()->length();


        std::string uri = target.toStdString();
        std::string root_path;

        auto r0 = fs::FileSystemFromUriOrPath(uri, &root_path);
        if (!r0.ok())
        {
            qDebug() << "Arrow Error not able to load" << df.outfile;
            return;
        }

        auto fs = r0.ValueOrDie();

        auto r1 = fs->OpenOutputStream(uri);

        if (!r1.ok())
        {
            qDebug() << "Arrow Error to Open Stream" << df.outfile;
            return;
        }
        auto output = r1.ValueOrDie();
        arrow::ipc::IpcWriteOptions options = arrow::ipc::IpcWriteOptions::Defaults();
        //options.codec = arrow::util::Codec::Create(arrow::Compression::LZ4).ValueOrDie(); //std::make_shared<arrow::util::Codec>(codec);

        auto r2 = arrow::ipc::MakeFileWriter(output.get(), table->schema(), options);

        if (!r2.ok())
        {
            qDebug() << "Arrow Unable to make file writer";
            return;
        }

        auto writer = r2.ValueOrDie();

        writer->WriteTable(*table.get());
        writer->Close();
        output->Close();
    }

    if (finished)
    {
        QStringList l = target.split("/");

        QString file = l.last(); l.pop_back();
        QString bp = l.join("/") + "/";
        QDir f(bp);

        if (srv.isEmpty())
        {
            qDebug() << bp << file ;
            f.rename(file, file + ".torm");

            fuseArrow(bp, QStringList() << file+".torm", bp+file, df.plate);
        }
        // plateData.remove(d);
    }

}

void NetworkProcessHandler::setPythonEnvironment(QProcessEnvironment env) { python_env = env; }


QStringList NetworkProcessHandler::remainingProcess()
{
    return runningProcs.keys();
}

void NetworkProcessHandler::displayError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);

    QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (!tcpSocket) return;

}

void NetworkProcessHandler::watcherProcessStatusFinished()
{
    QFutureWatcher<QJsonArray>* watcher = dynamic_cast<QFutureWatcher<QJsonArray>* >(sender());

    if (!watcher) {
        qDebug() << "Error retreiving watchers finished state";
        return;
    }
    emit  updateProcessStatusMessage(watcher->future().result());

    watcher->deleteLater();
    _waiting_Update = false;
}
