#ifndef CHECKOUTPROCESS_H
#define CHECKOUTPROCESS_H

#include <QtPlugin>

#include <QString>
#include <QStringList>

#include "Dll.h"

#include "checkoutprocessplugininterface.h"


class DllPluginManagerExport CheckoutProcess: public QObject
{
    Q_OBJECT
  /// the Data Loader interface that encapsulate all the CheckoutDataLoaderPluginInterface
public:
  CheckoutProcess();

  static CheckoutProcess& handler();

  void addProcess(CheckoutProcessPluginInterface* proc);
  void removeProcess(QString name);
  void removeProcess(CheckoutProcessPluginInterface* proc);

  QStringList paths();
  QStringList pluginPaths();
  QStringList networkPaths();

  void setProcessCounter(int* count);
  int getProcessCounter(QString hash);


  void getParameters(QString process);//, QJsonObject& params);
  void getParameters(QString process, QJsonObject& params);
  void startProcess(QString process, QJsonArray &params);

  void restartProcessOnErrors();

  void startProcessServer(QString process, QJsonArray &array);
  void refreshProcessStatus();
  QJsonObject refreshProcessStatus(QString hash);

  void finishedProcess(QString hash, QJsonObject result);
  bool shallDisplay(QString hash);

  unsigned numberOfRunningProcess();

  void exitServer();


  void queryPayload(QString hash);
  void deletePayload(QString hash);
  void attachPayload(QString hash, std::vector<unsigned char> data, bool mem = false, size_t pos = 0);

  void attachPlugin(QString hash, CheckoutProcessPluginInterface* p);

  std::vector<unsigned char> detachPayload(QString hash);

  bool hasPayload(QString hash);
  unsigned errors();



  QStringList users();
  void removeRunner(QString user, void* run);

  void cancelUser(QString user);
  void finishedProcess(QString dhash);

protected slots:

  void updatePath();
  void receivedParameters(QJsonObject obj);
  void networkProcessStarted(QString core, QString hash);
  void networkupdateProcessStatus(QJsonArray obj);
//  void clearFinishedProcesses();

protected:

  void addToComputedDataModel(QJsonObject ob);

//  void timerEvent(QTimerEvent *event);
signals:

  void newPaths();
  void parametersReady(QJsonObject obj);
  void processStarted(QString hash);
  void updateProcessStatus(QJsonArray);
  void updateDatabase();
  void processFinished(QJsonObject ob);

  void emptyProcessList();
  void payloadAvailable(QString hash);



 protected:
  QMap<QString, CheckoutProcessPluginInterface*> _plugins;
  QMap<QString, QJsonObject> _params;

  QMap<QString, QJsonObject> _process_to_start;

  QMap<QString, QString> _hash_to_core;
  QMap<QString, int*> _hash_to_save; // Store the relation between hash number and remaining process to start
  QMap<QString, bool> _display;

  // Handle the running processes, mapping the UID of the process to the process pointer
  QMap<QString, CheckoutProcessPluginInterface*> _status;
  QMap<QString, QJsonObject> _finished;

   //QMap<QString, QByteArray>  _payloads;
   QMap<QString, std::vector<unsigned char> > _payloads_vectors;

   QMap<QString, QSharedMemory*> _inmems;
   QMap<QString, CheckoutProcessPluginInterface*> _stored;
   
   QMap<QString, QList<void*> > _peruser_runners;

   int* _counter;
   QMutex mutex_dataupdate;

};



#endif // CHECKOUTPROCESS_H
