#include <QtCore>
#include <QtNetwork>
#include <QtConcurrent>
#include "processserver.h"

#ifdef PROCESS_SERVER_THREADED
#include "processthread.h"
#endif

#include <Core/checkoutprocess.h>
#include <Core/networkmessages.h>


inline QDataStream& operator<<(QDataStream& out, const long unsigned int & v)
{
    quint64 val = v;
    out << val;
    return out;
}

inline QDataStream& operator>> (QDataStream& in, long unsigned int& args)
{
    quint64 length;
    in >> length;
    args = length;
    return in;
}



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


#ifdef WIN32
#include <QWidget>

#include <QSystemTrayIcon>
#include <QMenu>

#include <Core/config.h>

class Control: public QWidget
{
    Q_OBJECT
public:
    Control(ProcessServer* serv): QWidget(), _serv(serv), lastNpro(0)
    {
        hide();

        quitAction = new QAction("Quit Checkout Server", this);
        connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));
        QSettings sets;


        trayIconMenu = new QMenu(this);
        //       trayIconMenu->addAction(minimizeAction);
        //       trayIconMenu->addAction(maximizeAction);
        //       trayIconMenu->addAction(restoreAction);
        //       trayIconMenu->addSeparator();
        trayIconMenu->addAction(quitAction);

        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setContextMenu(trayIconMenu);
        trayIcon->setIcon(QIcon(":/ServerIcon.png"));
        trayIcon->setToolTip(QString("Checkout Server listenning on port %1").arg(sets.value("ServerPort", 13378).toUInt()));
        trayIcon->show();

        startTimer(1000);

    }

    void showMessage(QString title, QString content)
    {
        trayIcon->showMessage(title, content);
    }

    virtual void timerEvent(QTimerEvent * event)
    {
        Q_UNUSED(event); // No need for the event timer object in this function

        QSettings sets;

        CheckoutProcess& procs = CheckoutProcess::handler();
        int npro = procs.numberOfRunningProcess();
        if (npro != 0)
            trayIcon->setToolTip(QString("Checkout Server processing %1 requests").arg(npro));
        else
            trayIcon->setToolTip(QString("Checkout Server listenning on port %1").arg(_serv->serverPort()));

        if (lastNpro != npro && npro == 0)
            trayIcon->showMessage("Checkout Server", "Checkout server has finished all his process");

        lastNpro = npro;
    }


public slots:

    void quit()
    {
        trayIcon->showMessage("Stopping Checkout Server", "Application requested checkout server exit");
        qApp->quit();
    }

protected:
    QAction* quitAction;

    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    ProcessServer* _serv;
    int lastNpro;

};

#include <processserver.moc>


#endif


ProcessServer::ProcessServer(QObject *parent) :
    QTcpServer(parent)
{
#ifdef WIN32

    _control = new Control(this);

#endif
}

void ProcessServer::incomingConnection(qintptr socketDescriptor)
{
#ifdef PROCESS_SERVER_THREADED
    qDebug() << "Receiving network connection";
    ProcessThread *thread = new ProcessThread(socketDescriptor);//, (QTcpServer*)this);

    thread->connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->connect(thread, SIGNAL(exit_signal()), this, SLOT(exit_func()));

    thread->start();
    thread->moveToThread(thread);
    threads << thread;
#else
    QTcpSocket* tcpSocket = new QTcpSocket(this);

    //qDebug() << "Incoming Socket" << tcpSocket;

    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(ready()));
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnect()));

    if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
        emit error(tcpSocket->error());
        qDebug() << "socket error" << tcpSocket->errorString();
        return;
    }

    clients [tcpSocket] = 0;
#endif

}

void ProcessServer::exit_func()
{
#ifdef WIN32
    _control->quit();
#endif
    //  qApp->quit();
}



// Handler of network data
// Each Server can be queried to:
// Supply available processings (processes )
// Supply processing details (proc_details $process_name)
// Supply current workload (workload)
// To return a number of processing slots (work_slots)
// Launch of a processing with supplied userinformation and data (start_proc $jsondescr)
// Get status of a process (proc_status $hash)
void ProcessServer::ready()
{
    //  qDebug() << "Data ready"         ;

    QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (!tcpSocket) return;
    //qDebug() << "ready  Socket" << tcpSocket;

    uint blockSize = clients[tcpSocket];

    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_5_3);
    //    qDebug() << tcpSocket->bytesAvailable() ;
    if (blockSize == 0) {
        if (tcpSocket->bytesAvailable() < (int)sizeof(quint16))
            return;

        in >> blockSize;
        tcpSocket->setReadBufferSize(blockSize);

        clients[tcpSocket] = blockSize;
    }
    //    qDebug() << blockSize;

    if (tcpSocket->bytesAvailable() < blockSize)
        return;

    QString keyword;
    in >> keyword;

    // qDebug() << "Received Keyword" << keyword;

    // If message in socket contains "processes" , call replyList();
    if (keyword == getNetworkMessageString(Processes))
    {
        replyList(tcpSocket);
    }
    // else if message is query + name of process call
    if (keyword  == getNetworkMessageString(Process_Details))
    {
        QString token;
        in >> token;
        replyProcess(tcpSocket, token);
    }

    if (keyword == getNetworkMessageString(Process_Start))
    {
        QString token;
        in >> token;
        QByteArray arr;
        in >> arr;

        QJsonArray ob = QJsonDocument::fromBinaryData(arr).array();
        startProcess(tcpSocket, token, &ob);
    }

    if (keyword == getNetworkMessageString(Process_Status))
    {
        QStringList hash;
        in >> hash;
        updateProcessMessage(tcpSocket, hash);
    }

    if (keyword == getNetworkMessageString(Exit_Server))
    {
        qDebug() << "Exit Server";
        tcpSocket->disconnectFromHost();
        //   tcpSocket->waitForDisconnected();
        exit_server();
    }


    if (keyword == getNetworkMessageString(Process_Payload))
    {
        QString hash;
        in >> hash;

        std::vector<unsigned char> data =
                CheckoutProcess::handler().detachPayload(hash);

        //       qDebug() << "Client request for payload " << hash << data.size();

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_3);
        out << (uint)(0);
        out << QString(getNetworkMessageString(Process_Payload));
        //      out << (uint)(data.size());
        out << hash;
        out << data;

        out.device()->seek(0);
        out << (uint)(block.size() - sizeof(uint));

        if (block.size())
        {
            tcpSocket->write(block);
            tcpSocket->waitForBytesWritten();
            //   tcpSocket->disconnectFromHost();
            //          tcpSocket->waitForDisconnected();
        }

    }

    if (keyword == getNetworkMessageString(Delete_Payload))
    {
        QString hash;
        in >> hash;

        std::vector<unsigned char> data
                = CheckoutProcess::handler().detachPayload(hash);
        data.clear();
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_3);
        out << (uint)(0);
        out << QString(getNetworkMessageString(Delete_Payload));

        out << hash;
        out.device()->seek(0);
        out << (uint)(block.size() - sizeof(uint));

        if (block.size())
        {
            tcpSocket->write(block);
            tcpSocket->waitForBytesWritten();
            tcpSocket->disconnectFromHost();
            //    //      tcpSocket->waitForDisconnected();
        }
    }

    clients[tcpSocket] = 0;
    
}

void ProcessServer::replyList(QTcpSocket* tcpSocket)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;

    CheckoutProcess& procs = CheckoutProcess::handler();

    out << QString(getNetworkMessageString(Processes));

    out << procs.pluginPaths();

    unsigned int processor_count = std::thread::hardware_concurrency();

    out << processor_count;

    out << QString(CHECKOUT_VERSION);


    qDebug() << CHECKOUT_VERSION << processor_count;



    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    if (block.size())
    {
        //qDebug() << "h1";
        tcpSocket->write(block);
        //qDebug() << "h2";
        tcpSocket->waitForBytesWritten();
        //qDebug() << "h3";
        //tcpSocket->disconnectFromHost();
        //	qDebug() << "h4";
        //        tcpSocket->close();
        // tcpSocket->waitForDisconnected();
    }
}

void ProcessServer::replyProcess(QTcpSocket* tcpSocket, QString str)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;

    CheckoutProcess& procs = CheckoutProcess::handler();
    QJsonObject ob;
    procs.getParameters(str, ob);

    out << QString(getNetworkMessageString(Process_Details));

    QJsonDocument doc(ob);
    QByteArray arr = doc.toBinaryData();
    out << arr;

    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    if (block.size())
    {
        tcpSocket->write(block);
        tcpSocket->waitForBytesWritten();
        //tcpSocket->disconnectFromHost();
        //  tcpSocket->waitForDisconnected();
    }
    //  qDebug() << str << ob;
}

void ProcessServer::startProcess(QTcpSocket* tcpSocket, QString proc, QJsonArray *obp)
{
    QJsonArray& ob = *obp;
   // qDebug() << "Starting" << proc << ob;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;

    CheckoutProcess& procs = CheckoutProcess::handler();

    QJsonArray Core,Run;

    for (int i = 0; i < ob.size(); ++i)
    {
        QJsonObject obj = ob.at(i).toObject();
        QJsonDocument doc(obj);
        QByteArray arr = doc.toBinaryData();
        arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
        QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

        Core.append(obj["CoreProcess_hash"]);
        QString sHash = hash.toHex();
        Run.append(QString(sHash));
        obj["Process_hash"] = sHash;
        if (tcpSocket->peerAddress()  == tcpSocket->localAddress())
            obj["LocalRun"] = true;

        ob.replace(i, obj);
    }

    QJsonObject obj;
    obj["ArrayCoreProcess"] = Core;
    obj["ArrayRunProcess"] = Run;


    QJsonDocument doc(obj);

    out << QString(getNetworkMessageString(Process_Start));
    out << doc.toBinaryData();

    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));

    if (block.size())
    {
        tcpSocket->write(block);
        tcpSocket->waitForBytesWritten();
        //  tcpSocket->disconnectFromHost();
        //   //   tcpSocket->waitForDisconnected();
    }

    QtConcurrent::run(&procs, &CheckoutProcess::startProcessServer,
                      proc, ob);
    //    procs.startProcessServer(proc, ob);
}

void ProcessServer::updateProcessMessage(QTcpSocket* tcpSocket, QStringList hash)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);
    out << (uint)0;

    CheckoutProcess& procs = CheckoutProcess::handler();

    QJsonArray array;
    int count = 0;
    foreach (QString h, hash)
    {
        QJsonObject ob = procs.refreshProcessStatus(h);
        if (ob["State"] == "Finished")
        {
            QJsonArray par = ob["Parameters"].toArray();

            for (int i = 0; i < par.size(); ++i)
            {
                if (par.at(i).toObject().contains("Payload"))
                {
                    count++;
                    break;
                }
            }
            if (count >= 5)
            {
                ob.remove("Parameters");
                ob["State"] = "Pending";
            }
        }
        ob["Hash"] = h;
        array.append(ob);
    }
    QJsonDocument doc(array);

    QByteArray arr = doc.toBinaryData();
    out << QString(getNetworkMessageString(Process_Status));
    out << arr;

    out.device()->seek(0);
    out << (uint)(block.size() - sizeof(uint));


    if (block.size())
    {
        tcpSocket->write(block);
        tcpSocket->waitForBytesWritten();
        // tcpSocket->disconnectFromHost();
        //    tcpSocket->waitForDisconnected();
    }
}

void ProcessServer::exit_server()
{
    emit exit_signal();
}

void ProcessServer::disconnect()
{
    // qDebug() << "Disconnect";
    QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (tcpSocket)
    {
        tcpSocket ->deleteLater();
        clients.remove(tcpSocket);
    }
    // exit(0);
}




#include <QMessageBox>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
//#include <QtConcurrent>
#include <Core/networkmessages.h>
#include <Core/checkoutprocess.h>

struct CheckoutHost
{
    QHostAddress address;
    quint16      port;
};

class  NetworkProcessFastHandler
{
public:
    NetworkProcessFastHandler();

    static NetworkProcessFastHandler& handler();
    void establishNetworkAvailability();

    QStringList getProcesses();
    void getParameters(QString process);
    void startProcess(QString process, QJsonArray ob);
    void getProcessMessageStatus(QString process, QList<QString> hash);

    void queryPayload(QString hash);

    void deletePayload(QString hash);

    void exitServer();


protected:
    void writeInitialQuery(QTcpSocket *soc);

    QTcpSocket* getNewSocket(CheckoutHost *h);

    void handleMessageProcesses(QString& keyword, QDataStream& in, QTcpSocket *tcpSocket);
    void handleMessageProcessDetails(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);
    void handleMessageProcessStart(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);
    void handleMessageProcessStatus(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);
    void handleMessageProcessPayload(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);

    void handleMessageDeletePayload(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket);


private :
    void readResponse();
    void displayError(QAbstractSocket::SocketError socketError);
    void watcherProcessStatusFinished();


protected:
    // Allows to link a process with a network connection
    QList<CheckoutHost*> activeHosts;

    QMap<QString, QList<CheckoutHost*> > procMapper;
    QList<QTcpSocket*> activeProcess;
    QMap<QTcpSocket*, uint > _blockSize;
    bool _waiting_Update;


};



NetworkProcessFastHandler::NetworkProcessFastHandler():
  _waiting_Update(false)
{
}

NetworkProcessFastHandler &NetworkProcessFastHandler::handler()
{
  static NetworkProcessFastHandler* handler = 0;
  if (!handler)
    handler = new NetworkProcessFastHandler();

  return *handler;
}

void NetworkProcessFastHandler::establishNetworkAvailability()
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
      activeHosts << h;

      QTcpSocket* soc = getNewSocket(h);
      writeInitialQuery(soc);
      //        qDebug() << soc->error() << soc->errorString() << soc->state();

      activeProcess << soc;
      qDebug() <<"Process from server"<< h->address << h->port << soc;
  }


}


void NetworkProcessFastHandler::writeInitialQuery(QTcpSocket* soc)
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

QTcpSocket *NetworkProcessFastHandler::getNewSocket(CheckoutHost* h)
{
  QTcpSocket* soc = new QTcpSocket;
  if (!soc)
    {
      qDebug() << "Socket Allocation error";
      return 0;
    }
  soc->abort();
  soc->connectToHost(h->address, h->port);

  if (!soc->waitForConnected(800))
    qDebug() << "socket error : " << soc->errorString();

  return soc;
}

QStringList NetworkProcessFastHandler::getProcesses()
{
  QStringList l;

  l =  procMapper.keys();
  //  qDebug() << "Network Handler list: " << l;
  return l;
}

void NetworkProcessFastHandler::getParameters(QString process)
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
      qDebug() << "Get Network process handler";
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
      soc->waitForBytesWritten();
  }

}

void NetworkProcessFastHandler::startProcess(QString process, QJsonArray ob)
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
      qDebug() << "Get Network process handler";
      return;
    }

  QTcpSocket* soc =  getNewSocket(h);

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

}

QJsonArray setMessage(QString key, QString value)
{
    QJsonArray arr;

    QJsonObject ob;
    ob.insert(key, QJsonValue(value));
    arr.push_back(ob);

    return arr;
}
// Modified Core/Server Exchange to better handle heavy interaction (here high network load)
// Large image sets are at risk for the handling of data this may lock the user interface with heavy processing
QJsonArray ProcessMessageStatusFunctor(CheckoutHost* h, QList<QString > hash, NetworkProcessFastHandler* owner)
{
//    qDebug()  << "Status Query" << hash.size();
  // First Create the socket
  QTcpSocket* soc = new QTcpSocket;
  if (!soc)
    {
      qDebug() << "Socket Allocation error";
      return QJsonArray();
    }

  soc->abort();
  soc->connectToHost(h->address, h->port);

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
  soc->waitForBytesWritten();

  // Analysis of the outcome of waitForReadyReady is 100% necessary for windows servers
  // since, the underlying system call function may fail differently
  // than the one from unix/posix systems
  bool ready = soc->waitForReadyRead();

  if (!ready && soc->bytesAvailable() <= 0)  // ready may be false, but data may still be available
  { // Skip processing only if no data is available at all
      qDebug() << "Socket error timeout " << soc->errorString();
      return setMessage("Error", "Timeout error "+soc->errorString());
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
        {
          qDebug() << "Socket error timeout" << soc->errorString();
          return setMessage("Error", "Timeout error "+soc->errorString());
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
void NetworkProcessFastHandler::getProcessMessageStatus(QString process, QList<QString > hash)
{
  if (_waiting_Update) { /* qDebug() << "Waiting for last reply..."; */return; }
  if (hash.size() <= 0) return;

  // Try to connect to any host having the same process
  if (procMapper[process].isEmpty())
    {
      qDebug() << "Empty process List" << process;
      return;
    }
  CheckoutHost* h = procMapper[process].first();
  if (!h)
    {
      qDebug() << "Get Network process handler";
      return;
    }

  QFutureWatcher<QJsonArray>* watcher = new QFutureWatcher<QJsonArray>();


  QFuture<QJsonArray> future = QtConcurrent::run(ProcessMessageStatusFunctor, h, hash, this);
  watcher->setFuture(future);

  _waiting_Update = true;
}

void NetworkProcessFastHandler::queryPayload(QString hash)
{
  CheckoutHost* h = procMapper.first().first();

  if (!h)
    {
      qDebug() << "Get Network process handler";
      return;
    }

  //  qDebug() << "Query Payload";

  QTcpSocket* soc =  getNewSocket(h);

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_3);
  out << (uint)0;
  out << QString(getNetworkMessageString(Process_Payload));
  out << hash;
  out.device()->seek(0);
  out << (uint)(block.size() - sizeof(uint));

  if (block.size())
  {
      soc->write(block);
      soc->waitForBytesWritten();
  }

}

void NetworkProcessFastHandler::deletePayload(QString hash)
{
  CheckoutHost* h = procMapper.first().first();

  if (!h)
    {
      qDebug() << "Get Network process handler";
      return;
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



void NetworkProcessFastHandler::exitServer()
{

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



void NetworkProcessFastHandler::handleMessageProcesses(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket)
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
      foreach (QString p, l)  procMapper[p] << host;
      //        qDebug() << procMapper.keys();
    }
}

void NetworkProcessFastHandler::handleMessageProcessDetails(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket)
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


  }
}

void NetworkProcessFastHandler::handleMessageProcessStart(QString& keyword, QDataStream& in,
                                                          QTcpSocket* tcpSocket)
{
  static const QString name = getNetworkMessageString(Process_Start);

  if (keyword == name)
    {

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
          //          qDebug() << "Process UID" << coreHash << hash;
           }
    }
}

void NetworkProcessFastHandler::handleMessageProcessStatus(QString& keyword, QDataStream& in, QTcpSocket* tcpSocket)
{
  static const QString name = getNetworkMessageString(Process_Status);

  if (keyword == name)
    {
      QByteArray arr;
      in >> arr;
      if (arr.size())
        {
          QJsonArray ob = QJsonDocument::fromBinaryData(arr).array();
          //          qDebug() << name << ob;
          }
      else
        {
          qDebug() << name << "Empty proc status...";
        }
      _waiting_Update = false;
    }
}

void NetworkProcessFastHandler::handleMessageProcessPayload(QString &keyword, QDataStream &in, QTcpSocket *tcpSocket)
{
  static const QString name = getNetworkMessageString(Process_Payload);
  if (keyword == name)
    {

      QString hash;

      in >> hash;

      std::vector<unsigned char> arr;
      in >> arr;

      // got the payload here
      CheckoutProcess::handler().attachPayload(hash, arr);
      }
}

void NetworkProcessFastHandler::handleMessageDeletePayload(QString &keyword, QDataStream &in, QTcpSocket *tcpSocket)
{
  static const QString name = getNetworkMessageString(Delete_Payload);
  if (keyword == name)
    {

      QString hash;

      in >> hash;

  }
}

void NetworkProcessFastHandler::readResponse()
{
  QTcpSocket* tcpSocket =0;// qobject_cast<QTcpSocket*>(sender());
  if (!tcpSocket) return;

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

void NetworkProcessFastHandler::displayError(QAbstractSocket::SocketError socketError)
{
  QTcpSocket* tcpSocket = 0;// qobject_cast<QTcpSocket*>(sender());
  if (!tcpSocket) return;



//  QMessageBox::information(0, tr("Checkout Network Client"),
//                           tr("The following error occurred: %1.")
//                           .arg(tcpSocket->errorString()));

}

void NetworkProcessFastHandler::watcherProcessStatusFinished()
{
  QFutureWatcher<QJsonArray>* watcher = 0;//dynamic_cast<QFutureWatcher<QJsonArray>* >(sender());

  if (!watcher) {
      qDebug() << "Error retreiving watchers finished state";
      return;
    }
  //emit  updateProcessStatusMessage(watcher->future().result());

  watcher->deleteLater();
  _waiting_Update = false;
}
