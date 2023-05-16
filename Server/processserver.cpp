#include <QtCore>
#include <QtNetwork>
#include <QtConcurrent>
#include "processserver.h"

#ifdef PROCESS_SERVER_THREADED
#include "processthread.h"
#endif

#include <Core/config.h>
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
        _cancelMenu = trayIconMenu->addMenu("Cancel Processes:");
        trayIconMenu->addAction(quitAction);

        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setContextMenu(trayIconMenu);
        trayIcon->setIcon(QIcon(":/ServerIcon.png"));
        trayIcon->setToolTip(QString("PhenoLink Server listenning on port %1").arg(sets.value("ServerPort", 13378).toUInt()));
        trayIcon->show();

        startTimer(2000);

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
        QString tooltip;
        if (npro != 0)
            tooltip = QString("PhenoLink Server processing %1 requests").arg(npro);
        else
            tooltip = QString("PhenoLink Server listenning on port %1").arg(_serv->serverPort());

        QStringList missing_users;
        for (auto user : procs.users())
        {
            tooltip = QString("%1\r\n%2").arg(tooltip).arg(user);
            bool missing = true;
            for (auto me: _users)
                if (me->text() == user)
                {
                    missing = false;
                    break;
                }
            if (missing) missing_users << user;
        }

        for (auto old: _users)
        {
            bool rem = true;
            for (auto ne: procs.users())
            {
                if (ne == old->text())
                    rem = false;
            }
            if (rem)
                _users.removeOne(old);
        }
        trayIcon->setToolTip(tooltip);

        if (lastNpro != npro && npro == 0)
            trayIcon->showMessage("PhenoLink Server", "PhenoLink server has finished all his process");

        lastNpro = npro;

        for (auto user: missing_users)
            _users.append(_cancelMenu->addAction(user));



    }


public slots:

    void quit()
    {
        trayIcon->showMessage("Stopping Checkout Server", "Application requested checkout server exit");
        qApp->quit();
    }

    void cancelUser()
    {
        // Play with QSender grab name & call QProcess Cancel func.
        QAction* origin = qobject_cast<QAction*>(sender());
        if (!origin) return;
        QString user = origin->text();
        CheckoutProcess::handler().cancelUser(user);
    }

protected:
    QAction* quitAction;
    QMenu*   _cancelMenu;
    QList<QAction*> _users;

    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    ProcessServer* _serv;
    int lastNpro;

};

#include <processserver.moc>

#else

void ProcessServer::setDriveMap(QString map)
{
   CheckoutProcess::handler().setDriveMap(map);
}

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


