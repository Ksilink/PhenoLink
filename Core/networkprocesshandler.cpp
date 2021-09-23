#include "networkprocesshandler.h"
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



CheckoutHttpClient::CheckoutHttpClient(QString host, quint16 port):  awaiting(false), icpus(0)
{
//    iclient.setTimeOut(5000);
    QObject::connect(&iclient, &QHttpClient::disconnected, [this]() {
        finalize();
    });

    iurl.setScheme("http");
    iurl.setHost(host);
    iurl.setPort(port);


    startTimer(500);
}

CheckoutHttpClient::~CheckoutHttpClient()
{
    qDebug() << "CheckoutHttpClient : I've been killed";
}

void CheckoutHttpClient::send(QString path, QString query)
{
    send(path, query, QByteArray(), true);
}

void CheckoutHttpClient::send(QString path, QString query, QJsonArray ob, bool keepalive)
{
     auto body = QCborValue::fromJsonValue(ob).toCbor();
     QUrl url=iurl;
     url.setPath(path);
     url.setQuery(query);

//     qDebug() << "Creating Query" << url.toString() << ob;
     send(path, query, body, keepalive);
}

void CheckoutHttpClient::send(QString path, QString query, QByteArray ob, bool keepalive)
{

    QUrl url=iurl;
    url.setPath(path);
    url.setQuery(query);

    reqs.append(Req(url, ob, keepalive));
    if (reqs.isEmpty())
       sendQueue();
}
void CheckoutHttpClient::sendQueue()
{
    if (reqs.isEmpty())
        return;
//    if (awaiting)
//        return;


    auto req = reqs.takeFirst();
    QUrl url = req.url;
    auto ob = req.data;
    auto keepalive = req.keepalive;

    qDebug() << "Sending Queued" << url;
    awaiting = true;
    iclient.request(
                qhttp::EHTTP_POST,
                req.url,
                [ob, keepalive](QHttpRequest* req){
        auto body = ob;
//        req->addHeader("connection", keepalive ? "keep-alive" : "close");
        req->addHeader("Content-Type", "application/cbor");
        req->addHeaderValue("content-length", body.length());
        req->end(body);
        qDebug() << "Request" << req->connection()->tcpSocket()->peerAddress()
                 << req->connection()->tcpSocket()->peerPort() << (keepalive ? "keep-alive" : "close");
                    ;

    },

    [this](QHttpResponse* res) {
        res->collectData();
        res->onEnd([this, res](){
            onIncomingData(res->collectedData());
            awaiting = false; // finished current send
      //      sendQueue(); // send next message
            qDebug() << "Response received";
        });
    });

    if (iclient.tcpSocket()->error() != QTcpSocket::UnknownSocketError)
        qDebug() << "Send" << iclient.tcpSocket()->errorString();

}

void CheckoutHttpClient::timerEvent(QTimerEvent *event)
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

void CheckoutHttpClient::finalize() {
    qDebug() << "Disconnected";    //        qDebug("totally %d request/response pairs have been transmitted in %lld [mSec].\n",
    ////               istan, itick.tock()
    //               );
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
#if FIXME_HTTP
    // qDebug()  << "Status Query" << hash.size() << h->address << h->port;
    // First Create the socket
    QTcpSocket* soc = new QTcpSocket;
    if (!soc)
    {
        qDebug() << "Socket Allocation error";
        return QJsonArray();
    }

    soc->abort();
    soc->connectToHost(h->address, h->port);
    soc->connect(soc, SIGNAL(error(QAbstractSocket::SocketError)),
                 owner, SLOT(displayError(QAbstractSocket::SocketError)));

    if (!soc->waitForConnected(800))
        qDebug() << "socket error : " << soc->errorString();
    // Write the command
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;
    out << QString(getNetworkMessageString(Process_Status));
    out << hash;
    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    soc->write(block);

    //    soc->waitForBytesWritten();
    bool dataready = soc->waitForReadyRead();

    if (!dataready && soc->bytesAvailable() <= 0)
    {
        // Error
        qDebug() << "Timeout error" << soc->errorString();
        return QJsonArray();
    }

    // Retrieve the server response
    QDataStream in(soc);
    in.setVersion(QDataStream::Qt_5_3);
    int blockSize;
    in >> blockSize;

    soc->setReadBufferSize(blockSize);
    while (soc->bytesAvailable() < blockSize)
    {
        if (!soc->waitForReadyRead())
        { // Timeout or
            qDebug() << "Network timeout error" << soc->errorString();
            return QJsonArray();
        }
    }

    QString keyword;
    in >> keyword;

    static const QString name = getNetworkMessageString(Process_Status);
    QJsonArray res;
    if (keyword == name)
    {
        QByteArray arr;
        in >> arr;
        if (arr.size())
        {
            res = QJsonDocument::fromBinaryData(arr).array();
            //          qDebug() << name << ob;
            //            emit updateProcessStatusMessage(ob);
        }
        else
        {
            qDebug() << name << "Empty proc status...";
        }
    }
    else
        qDebug() << "Erroneous reply to " << name << "query (received:" << keyword << "len: " << blockSize << ")";

    soc->close();
    soc->deleteLater();
    // qDebug()  << "Status Query" << hash.size();

    return res;
#endif
    return QJsonArray();
}


// The Process message status can be a bit long, though the current network approach is not at it's best handling
// the message at threaded approach would be better of handling this network information
void NetworkProcessHandler::getProcessMessageStatus(QString process, QList<QString > hash)
{
#if FIXME_HTTP
    QSettings set;
    int Server_query_max_hash = set.value("maxRefreshQuery", 2000).toInt();
    //    qDebug() << "Get States from server" << hash.size();

    if (_waiting_Update) {  /*qDebug() << "Waiting for last reply...";*/ return; }

    if (hash.size() <= 0) { qDebug() << "Empty hash..."; return; }

    // Try to connect to any host having the same process
    if (procMapper[process].isEmpty())
    {
        qDebug() << "Empty process List" << process;
        return;
    }

    QMap<CheckoutHost*, QStringList > dispatch;
    foreach (QString hsh, hash)
    {
        //        qDebug() << "Searching where was dispatched " <<runningProcs[hsh]<< hsh;
        dispatch[runningProcs[hsh]] << hsh;
    }

    for (QMap<CheckoutHost*, QStringList>::iterator it = dispatch.begin();
         it != dispatch.end(); ++it)
    {
        CheckoutHost* h = it.key();
        if (!h)
        {
            qDebug() << "getProcessMessageStatus: Get Network process handler";
            return;
        }
        //CheckoutHost* h, QList<QString > hash, NetworkProcessHandler* owner)
        //qDebug() << "Query state:" << h->address << h->port << *it;


        if (it->size() > Server_query_max_hash) // Split the server query burden if hash list is very long
            // Avoids too large Json's reply
        {
            QStringList hshs = *it;

            for (int i =0; i < (int)ceil(it->size() / Server_query_max_hash); ++i)
            {

                QStringList t;
                for (int s = 0; s < Server_query_max_hash && i*Server_query_max_hash+s<hshs.size(); ++s )
                    t.push_back(hshs.at(i*Server_query_max_hash+s));

                QFutureWatcher<QJsonArray>* watcher = new QFutureWatcher<QJsonArray>();
                connect(watcher, SIGNAL(finished()), this, SLOT(watcherProcessStatusFinished()));

                QFuture<QJsonArray> future = QtConcurrent::run(ProcessMessageStatusFunctor, h, t, this);
                watcher->setFuture(future);
            }
        }
        else
        {

            QFutureWatcher<QJsonArray>* watcher = new QFutureWatcher<QJsonArray>();

            connect(watcher, SIGNAL(finished()), this, SLOT(watcherProcessStatusFinished()));

            QFuture<QJsonArray> future = QtConcurrent::run(ProcessMessageStatusFunctor, h, *it, this);
            watcher->setFuture(future);
        }
    }
    _waiting_Update = true;
#endif
}


QJsonArray FilterObject(QString hash, QJsonObject ds)
{
    QJsonArray res;
    QJsonObject ob;
    ob["hash"] = hash;

    if (ds.contains("Data"))
    {
        auto arr = ds["Data"].toArray();
        for (auto itd : arr)
        {
            auto obj = itd.toObject();
            if (obj["Data"].toString() != "Image results" )
            {
//                if (obj.contains("isOptional") && !obj["optionalState"].toBool())
//                    continue;

                //qDebug() << obj;


                ob[obj["Tag"].toString()] = obj["Data"];
                ob[QString("%1_Agg").arg(obj["Tag"].toString())] = obj["Aggregation"];

                if (obj.contains("Meta"))
                {
                    auto met = obj["Meta"].toArray()[0].toObject();

                    auto txt = QStringList() << "FieldId" << "Pos" << "TimePos" << "channel" << "zPos" << "DataHash";
                    for (auto s: txt)
                        if (met.contains(s))
                        {

                            ob[s] = met[s];
                        }
//                        else { qDebug() << s << "Not found"; }
                }
//                else
//                {
//                    qDebug() << "######################################" << Qt::endl
//                             <<"No Meta" << Qt::endl
//                            << obj;
//                }
            }
        }

    }
    res << ob;
    return res;
}


QCborArray filterBinary(QString hash, QJsonObject ds)
{
    QCborArray res;


    if (ds.contains("Data"))
    {
        auto arr = ds["Data"].toArray();
        for (auto itd : arr)
        {
            auto obj = itd.toObject();
          //  qDebug() << "Filtering" << obj["Tag"] << obj;
            if (obj["Data"].toString() == "Image results" && obj.contains("Payload"))
            {
                if (obj.contains("isOptional") && !obj["optionalState"].toBool())
                {
                    auto ps = obj["Payload"].toArray();
                    QCborArray cbar;
                    for (auto pp : ps)
                    {
                        auto pay = pp.toObject();
                        if (pay.contains("DataHash"))
                        {
                            QString dhash = pay["DataHash"].toString();
                            auto buf = CheckoutProcess::handler().detachPayload(dhash);
                            buf.clear();
                        }
                    }
                    continue;
                }

                QCborMap ob;
                auto txt = QStringList() << "ContentType" << "ImageType" << "ChannelNames"
                                         << "Tag";
                for (auto s: txt)
                    if (obj.contains(s))
                    {
                        ob.insert(QCborValue(s), QCborValue::fromJsonValue(obj[s]));
                    }
                QString dhash;

                if (obj.contains("Meta"))
                {
                    auto met = obj["Meta"].toArray()[0].toObject();
                    auto txt = QStringList() << "FieldId" << "Pos" << "TimePos"
                            << "channel" << "zPos" << "DataHash" ;
                    for (auto s: txt)
                        if (met.contains(s))
                        {
                            ob.insert(QCborValue(s), QCborValue::fromJsonValue(met[s]));
                        }
                    dhash = met["DataHash"].toString();
                }
                else
                {
                    qDebug() << "######################################" << Qt::endl
                             <<"No Meta" << Qt::endl
                            << obj;
                    continue;
                }
                qDebug() << "Ready data" << ob.toJsonObject();
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
                        auto data = QByteArray(reinterpret_cast<const char*>(buf.data()), (int)buf.size());
                        qDebug() << "Got binary data: " << dhash << buf.size() << data.size();
                        mm.insert(QCborValue("BinaryData"), QCborValue(data));
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
                res << ob;
            }
        }
    }
    return res;
}


void NetworkProcessHandler::finishedProcess(QString hash, QJsonObject res)
{
    QString address=res["ReplyTo"].toString();
///    qDebug() << hash << res << address;
    CheckoutHttpClient* client = NULL;
    for (CheckoutHttpClient* cl : alive_replies)  if (address == cl->iurl.host())  client = cl;
    if (!client) { client = new CheckoutHttpClient(address, 8020); alive_replies << client; }
   // else { qDebug() << "Reusing Client"; }
    QString commitname = res["CommitName"].toString();
    if (commitname.isEmpty()) commitname = "Default";
    // now we can setup the reply !

    QJsonArray data = FilterObject(hash, res);
   // qDebug() << "Sending dataset" << data;
    client->send(QString("/addData/%1").arg(commitname), QString(), data);



    QCborArray bin = filterBinary(hash, res);
    for (auto b: bin)
    {
        client->send(QString("/addImage/"), QString(), b.toCbor());
    }


}

void NetworkProcessHandler::removeHash(QString hash)
{
    // Just finished a item
//    qDebug() << "Finished Job" << hash;
    runningProcs.remove(hash);
    emit finishedJob();
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

        emit processStarted(coreHash, hash);
    }

}


QStringList NetworkProcessHandler::remainingProcess()
{
    return runningProcs.keys();
}

void NetworkProcessHandler::displayError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);

    QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (!tcpSocket) return;



    //  QMessageBox::information(0, tr("Checkout Network Client"),
    //                           tr("The following error occurred: %1.")
    //                           .arg(tcpSocket->errorString()));

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
