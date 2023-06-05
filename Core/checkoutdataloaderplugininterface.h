#ifndef CHECKOUTDATALOADERPLUGININTERFACE_H
#define CHECKOUTDATALOADERPLUGININTERFACE_H

#include <QtPlugin>

#include <QString>
#include <QStringList>

#include "Dll.h"

class ExperimentFileModel;

#define CheckoutDataLoaderPluginInterface_iid "fr.wiest-daessle.Checkout.DataLoaderInterface"

class CheckoutDataLoaderPluginInterface
{
public:

  virtual ~CheckoutDataLoaderPluginInterface() {}
  virtual QString pluginName() = 0;
  virtual QStringList handledFiles() = 0;

  virtual bool isFileHandled(QString file) = 0;

  virtual ExperimentFileModel* getExperimentModel(QString file) = 0;
  virtual QString errorString() = 0;


};

Q_DECLARE_INTERFACE(CheckoutDataLoaderPluginInterface, CheckoutDataLoaderPluginInterface_iid)

class ExperimentFileModel;

class DllPluginManagerExport CheckoutDataLoader
{
  /// the Data Loader interface that encapsulate all the CheckoutDataLoaderPluginInterface
public:
  static CheckoutDataLoader& handler();

  QStringList handledFiles();
  bool isFileHandled(QString s);
  ExperimentFileModel* getExperimentModel(QString file);

  void addPlugin(CheckoutDataLoaderPluginInterface* plugin);

  QString& errorMessage();

protected:
  QList<CheckoutDataLoaderPluginInterface*> _plugins;
  QString _errors;

};



#endif // CHECKOUTDATALOADERPLUGININTERFACE_H
