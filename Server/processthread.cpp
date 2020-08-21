
#include <QtCore>
#include <QDebug>
#include "processthread.h"

#include <Core/checkoutprocess.h>

#include <Core/networkmessages.h>

ProcessThread::ProcessThread(qintptr socketDesc, QObject *parent):
    QThread(parent),
  socketDescriptor(socketDesc), blockSize(0)
{
//    qDebug() << "Adding Connection" << socketDesc;
}

void ProcessThread::run()
{
  tcpSocket = new QTcpSocket;

  connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(ready()));
  connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnect()));

  if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
      emit error(tcpSocket->error());
      qDebug() << tcpSocket->errorString();
      return;
    }


//  qDebug()  << tcpSocket->peerAddress() << socketDescriptor << " Client connected";

  // make this thread a loop,
  // thread will stay alive so that signal/slot to function properly
  // not dropped out in the middle when thread dies

  exec();
}

void ProcessThread::disconnect()
{
//  qDebug() << "Disconnect";
  QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>(sender());
  if (tcpSocket) tcpSocket->deleteLater();
  exit(0);
 }


// Handler of network data
// Each Server can be queried to:
// Supply available processings (processes )
// Supply processing details (proc_details $process_name)
// Supply current workload (workload)
// To return a number of processing slots (work_slots)
// Launch of a processing with supplied userinformation and data (start_proc $jsondescr)
// Get status of a process (proc_status $hash)
void ProcessThread::ready()
{
      qDebug() << "Data ready";
  QDataStream in(tcpSocket);
  in.setVersion(QDataStream::Qt_5_3);
  //    qDebug() << tcpSocket->bytesAvailable() ;
  if (blockSize == 0) {
      if (tcpSocket->bytesAvailable() < (int)sizeof(quint16))
        return;

      in >> blockSize;
    }
  //    qDebug() << blockSize;

  if (tcpSocket->bytesAvailable() < blockSize)
    return;

  QString keyword;
  in >> keyword;

  qDebug() << "Received Keyword" << keyword;

  // If message in socket contains "processes" , call replyList();
  if (keyword  == getNetworkMessageString(Processes))
    replyList();

  blockSize = 0;
  return;

  // else if message is query + name of process call
  if (keyword  == getNetworkMessageString(Process_Details))
    {
      QString token;
      in >> token;
      replyProcess(token);
    }

  if (keyword == getNetworkMessageString(Process_Start))
    {
      QString token;
      in >> token;
      QByteArray arr;
      in >> arr;

      QJsonArray ob = QJsonDocument::fromBinaryData(arr).array();
      startProcess(token, ob);
    }

  if (keyword == getNetworkMessageString(Process_Status))
    {
      QStringList hash;
      in >> hash;
      updateProcessMessage(hash);
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

       QByteArray data = CheckoutProcess::handler().detachPayload(hash);

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
//          tcpSocket->waitForBytesWritten();
//          tcpSocket->disconnectFromHost();
//          tcpSocket->waitForDisconnected();
      }
  }

  if (keyword == getNetworkMessageString(Delete_Payload))
  {
      QString hash;
      in >> hash;

      QByteArray data = CheckoutProcess::handler().detachPayload(hash);
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
//          tcpSocket->waitForBytesWritten();
//          tcpSocket->disconnectFromHost();
//    //      tcpSocket->waitForDisconnected();
      }


  }


  blockSize  = 0;
}

void ProcessThread::replyList()
{
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_3);
  out << (uint)0;

  CheckoutProcess& procs = CheckoutProcess::handler();

  out << QString(getNetworkMessageString(Processes));
  out << procs.pluginPaths();
  out.device()->seek(0);
  out << (uint)(block.size() - sizeof(uint));

  if (block.size())
  {
      tcpSocket->write(block);
      tcpSocket->waitForBytesWritten();
      tcpSocket->disconnectFromHost();
      tcpSocket->waitForDisconnected();
  }
}

void ProcessThread::replyProcess(QString str)
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
//      tcpSocket->waitForBytesWritten();
//      tcpSocket->disconnectFromHost();
    //  tcpSocket->waitForDisconnected();
  }
//  qDebug() << str << ob;
}

void ProcessThread::startProcess(QString proc, QJsonArray ob)
{
//  qDebug()  << "Starting" << proc << ob;

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
//      tcpSocket->waitForBytesWritten();
//      tcpSocket->disconnectFromHost();
//   //   tcpSocket->waitForDisconnected();
  }

  procs.startProcessServer(proc, ob);
}

void ProcessThread::updateProcessMessage(QStringList hash)
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
//      tcpSocket->waitForBytesWritten();
//      tcpSocket->disconnectFromHost();
  //    tcpSocket->waitForDisconnected();
  }
}

void ProcessThread::exit_server()
{
  emit exit_signal();
}
