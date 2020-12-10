#include "networkprocesshandler.h"
#include <QMessageBox>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
//#include <QtConcurrent>
#include <Core/networkmessages.h>
#include <Core/checkoutprocess.h>
#include <Core/config.h>



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

    foreach (QJsonValue ss, data)
    {
        QJsonObject o = ss.toObject();
        CheckoutHost* h = new CheckoutHost;
        h->address = QHostAddress(o["Host"].toString());
        h->port = o["Port"].toInt();

        QTcpSocket* soc = getNewSocket(h);
        if (soc->state() != QAbstractSocket::ConnectedState)
        {
            qDebug() << "Network connection to " << h->address << h->port << " impossible";
            qDebug() << soc->errorString();
            soc->close();
            delete soc;
            continue;
        }

        activeHosts << h;

        writeInitialQuery(soc);


        activeProcess << soc;
        qDebug() <<"Process from server"<< h->address << h->port << soc;
    }


}


void NetworkProcessHandler::writeInitialQuery(QTcpSocket* soc)
{
    //    qDebug() << soc->error() << soc->errorString() << soc->state();

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;
    out << QString(getNetworkMessageString(Processes));
    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    if (block.size())
    {
        soc->write(block);
        soc->waitForBytesWritten();
    }
    // qDebug() << block.size();
}

QTcpSocket *NetworkProcessHandler::getNewSocket(CheckoutHost* h, bool with_sig)
{
    QTcpSocket* soc = new QTcpSocket;
    if (!soc)
    {
        qDebug() << "Socket Allocation error";
        return 0;
    }
    soc->abort();
    soc->connectToHost(h->address, h->port);
    if (with_sig)
    {
        connect(soc, SIGNAL(readyRead()), this, SLOT(readResponse()));
        connect(soc, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(displayError(QAbstractSocket::SocketError)));
    }
    if (!soc->waitForConnected(800))
        qDebug() << "socket error : " << soc->errorString();

    return soc;
}

QStringList NetworkProcessHandler::getProcesses()
{
    QStringList l;

    l =  procMapper.keys();
    //  qDebug() << "Network Handler list: " << l;
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
    CheckoutHost* h = procMapper[process].first();
    if (!h)
    {
        qDebug() << "getParameters: Get Network process handler";
        return;
    }

    qDebug() << "Query Process details" << process;

    QTcpSocket* soc = getNewSocket(h);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;
    out << QString(getNetworkMessageString(Process_Details));
    out << process;
    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    if (block.size())
    {
        soc->write(block);
        //       soc->waitForBytesWritten();
    }

    //   activeProcess << soc;
}

void NetworkProcessHandler::startProcess(QString process, QJsonArray ob)
{
    QList<CheckoutHost*> procsList = procMapper[process];

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

    //    qDebug() << "Starting with server" << last_serv_pos;

    last_serv_pos = last_serv_pos % procsList.size();

    qDebug() << "Starting with server" << last_serv_pos;

    for (int i = 0; i < last_serv_pos; ++i)
        procsList.push_back(procsList.takeFirst());

    //    foreach (CheckoutHost* h, procsList)
    //        qDebug() << h->address << h->port;

    int lastItem = 0;
    foreach(CheckoutHost* h, procsList)
    {
        //      CheckoutHost* h = procMapper[process].first();
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

void NetworkProcessHandler::startProcess(CheckoutHost *h, QString process, QJsonArray ob)
{
    if (!h)
    {
        qDebug() << "Error cannot Get Network process handler";
        return;
    }

    QTcpSocket* soc =  getNewSocket(h, false);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;
    out << QString(getNetworkMessageString(Process_Start));
    out << process;

    QJsonDocument doc(ob);
    QByteArray arr = doc.toBinaryData();
    out << arr;

    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    if (block.size())
    {
        soc->write(block);
        soc->waitForBytesWritten();
    }

    bool ready = soc->waitForReadyRead(2000);

    // If false this may be due to timeout, need to check
    if (ready && soc->bytesAvailable())
        readResponse(soc);
    else // Need to modify signature of caller to handle return informations
    {
        qDebug() << "Error Starting process, no return from server request => " << h->address << h->port;

        _error_list << qMakePair(h, qMakePair(process, ob));
    }
}


// Modified Core/Server Exchange to better handle heavy interaction (here high network load)
// Large image sets are at risk for the handling of data this may lock the user interface with heavy processing
QJsonArray ProcessMessageStatusFunctor(CheckoutHost* h, QList<QString > hash, NetworkProcessHandler* owner)
{
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

}


// The Process message status can be a bit long, though the current network approach is not at it's best handling
// the message at threaded approach would be better of handling this network information
void NetworkProcessHandler::getProcessMessageStatus(QString process, QList<QString > hash)
{
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
}

void NetworkProcessHandler::queryPayload(QString ohash)
{
    QString hash=ohash;
    hash.truncate(32);
    //    CheckoutHost* h = procMapper.first().first();
    (*hash_logfile) << "Payload " << ohash << Qt::endl;
    CheckoutHost* h = runningProcs[hash];

    if (!h)
    {
        qDebug() << "queryPayload: Get Network process handler" << hash;
        return;
    }
    else
    {
        runningProcs.remove(ohash);
        qDebug() << "query Payload: Network Stack remaining hash" << runningProcs.size();
    }

    qDebug() << "Query Payload";

    QTcpSocket* soc =  getNewSocket(h);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;
    out << QString(getNetworkMessageString(Process_Payload));
    out << ohash;
    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    if (block.size())
    {
        soc->write(block);
        soc->waitForBytesWritten();
    }

}

void NetworkProcessHandler::deletePayload(QString hash)
{
    //    CheckoutHost* h = procMapper.first().first();
    CheckoutHost* h = runningProcs[hash];
    runningProcs.remove(hash);

    if (!h)
    {
        //    qDebug() << "deletePayload: Get Network process handler" << hash;
        return;
    }
    else
    {
        //  qDebug() << "deletePayload: sending delete command for" << hash;
        qDebug() << "Delete payload: Network Stack remaining hash" << runningProcs.size();
    }

    //  qDebug() << "Query Payload";

    QTcpSocket* soc =  getNewSocket(h);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;
    out << QString(getNetworkMessageString(Delete_Payload));
    out << hash;
    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    if (block.size())
    {
        soc->write(block);
        soc->waitForBytesWritten();
    }


}

void NetworkProcessHandler::processFinished(QString hash)
{
    if (runningProcs.contains(hash))
        runningProcs.remove(hash);

    qDebug() << "Process finished : "<< hash << "Network Stack remaining hash" << runningProcs.size();
    (*hash_logfile) << "Finished " << hash << Qt::endl;
    //    qDebug() << "Query clear mem" << hash;
    //    CheckoutProcess::handler().deletePayload(hash);

}

void NetworkProcessHandler::removeHash(QString hash)
{
    runningProcs.remove(hash);
}



void NetworkProcessHandler::exitServer()
{
    // FIXME: Should only kill Owned server and not all the queued ones...
    CheckoutHost* h = procMapper.first().first();

    QTcpSocket* soc =  getNewSocket(h);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);

    out << (uint)0;
    out << QString(getNetworkMessageString(Exit_Server));

    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));
    qDebug() << "Exit server";
    if (block.size())
    {
        soc->write(block);
        soc->waitForBytesWritten();
    }

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



void NetworkProcessHandler::handleMessageProcesses(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket)
{
    static const QString name = getNetworkMessageString(Processes);

    if (keyword == name)
    {
        QHostAddress add = tcpSocket->peerAddress();
        quint16 port = tcpSocket->peerPort();
        CheckoutHost* host = 0;
        foreach (CheckoutHost* h, activeHosts)
            if (h->address == add && h->port == port)
                host = h;

        if (host == 0)
        {
            qDebug() << "Received answer from host, but not in active list..." << add << port;
            host = new CheckoutHost;
            host->address = add;
            host->port = port;
            activeHosts << host;
        }


        QStringList l;
        in >> l;


        unsigned int processor_count = 0;
        in >> processor_count;
        qDebug() << host->address <<":"<< host->port << "Reports: " << processor_count << "CPU";

        QString version;

        in >> version;

        if (version != CHECKOUT_VERSION)
        {
            qDebug() << "Server working with version: " << version << "but client is" << CHECKOUT_VERSION;
        }



        foreach (QString p, l)
            if (p != "") { procMapper[p] << host; }
        //        qDebug() << procMapper.keys();
        emit newProcessList();
    }
}

void NetworkProcessHandler::handleMessageProcessDetails(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket)
{
    static const QString name = getNetworkMessageString(Process_Details);

    if (keyword == name)
    {
        //        qDebug()  << "Answered!";
        QHostAddress add = tcpSocket->peerAddress();
        quint16 port = tcpSocket->peerPort();
        CheckoutHost* host = 0;
        foreach (CheckoutHost* h, activeHosts)
            if (h->address == add && h->port == port)
                host = h;
        if (host == 0)
        {
            qDebug() << "Received answer from host, but not in active list..." << add << port;
            host = new CheckoutHost;
            host->address = add;
            host->port = port;
            activeHosts << host;
        }

        QByteArray data;
        in >> data;
        QJsonObject ob = QJsonDocument::fromBinaryData(data).object();

        emit parametersReady(ob);
    }
}

void NetworkProcessHandler::handleMessageProcessStart(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket)
{
    Q_UNUSED(tcpSocket);

    static const QString name = getNetworkMessageString(Process_Start);

    if (keyword == name)
    {
        //        qDebug()* << "Server return value" << name;
        QByteArray arr;
        in >> arr;
        QJsonObject obj = QJsonDocument::fromBinaryData(arr).object();

        QJsonArray Core = obj["ArrayCoreProcess"].toArray();
        QJsonArray Run = obj["ArrayRunProcess"].toArray();

        if (Core.size() != Run.size())  qDebug() << "Incoherent data size in starting processes...";

        for (int i = 0; i < std::min(Core.size(), Run.size()); ++i)
        {
            QString coreHash = Core.at(i).toString("");
            QString hash = Run.at(i).toString("");

            CheckoutHost* h = runningProcs[coreHash];
            runningProcs.remove(coreHash);
            runningProcs[hash] = h;

            if (hash_logfile)
                (*hash_logfile) << coreHash << "->" << hash << Qt::endl;

            emit processStarted(coreHash, hash);
        }
    }
}

void NetworkProcessHandler::handleMessageProcessStatus(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket)
{
    Q_UNUSED(tcpSocket);

    static const QString name = getNetworkMessageString(Process_Status);

    if (keyword == name)
    {
        QByteArray arr;
        in >> arr;
        if (arr.size())
        {
            QJsonArray ob = QJsonDocument::fromBinaryData(arr).array();
            //          qDebug() << name << ob;
            emit updateProcessStatusMessage(ob);
        }
        else
        {
            qDebug() << name << "Empty proc status...";
        }
        _waiting_Update = false;
    }
}

void NetworkProcessHandler::handleMessageProcessPayload(QString &keyword, QDataStream &in, QTcpSocket *tcpSocket)
{
    Q_UNUSED(tcpSocket);

    static const QString name = getNetworkMessageString(Process_Payload);
    if (keyword == name)
    {

        QString hash;

        in >> hash;

        std::vector<unsigned char> arr;
        in >> arr;

        // got the payload here
        CheckoutProcess::handler().attachPayload(hash, arr);
        emit payloadAvailable(hash);
    }
}

void NetworkProcessHandler::handleMessageDeletePayload(QString &keyword, QDataStream &in, QTcpSocket *tcpSocket)
{
    Q_UNUSED(tcpSocket);

    static const QString name = getNetworkMessageString(Delete_Payload);
    if (keyword == name)
    {

        QString hash;

        in >> hash;

    }
}

void NetworkProcessHandler::readResponse()
{
    QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (!tcpSocket) return;
    readResponse(tcpSocket);
}

void NetworkProcessHandler::readResponse(QTcpSocket* tcpSocket)
{
    //    qDebug() << tcpSocket->errorString() << tcpSocket->state();
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_5_3);
    uint blockSize = _blockSize[tcpSocket];
    //    qDebug() << blockSize << tcpSocket->bytesAvailable() ;
    if (blockSize == 0) {
        if (tcpSocket->bytesAvailable() < (int)sizeof(uint))
        {
            //          qDebug() << "socket loop" << tcpSocket;
            return;
        }
        in >> blockSize;
        tcpSocket->setReadBufferSize(blockSize);
        _blockSize[tcpSocket] = blockSize;
    }

    if (tcpSocket->bytesAvailable() < blockSize)
    {
        //      qDebug() << "socket loop" << tcpSocket;
        return;
    }

    QString keyword;
    in >> keyword;
    //  qDebug() << "Block Size prior to read" << blockSize;
    _blockSize.remove(tcpSocket);
    //qDebug() << "Received command" << keyword << blockSize;
    QElapsedTimer dtimer;       // Do the process timing
    dtimer.start();



    handleMessageProcesses(keyword, in, tcpSocket);
    handleMessageProcessDetails(keyword, in, tcpSocket);
    handleMessageProcessStart(keyword, in, tcpSocket);
    handleMessageProcessStatus(keyword, in, tcpSocket);
    handleMessageProcessPayload(keyword, in, tcpSocket);

    //  qDebug() << "Message" << keyword << "handled in " <<   dtimer.elapsed() << "ms";


    // Exchange is finished, close our socket
    // Only keep connection when starting processes
    tcpSocket->close();
    activeProcess.removeAll(tcpSocket);
    //    delete tcpSocket;
    tcpSocket->deleteLater();
    blockSize  = 0;
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
