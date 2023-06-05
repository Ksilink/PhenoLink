#include "checkoutdataloaderplugininterface.h"
#include "Core/wellplatemodel.h"

#include <QDateTime>

#include <QStringList>
#include <QDebug>

CheckoutDataLoader &CheckoutDataLoader::handler()
{
  static CheckoutDataLoader* dl = 0;
  if (!dl) dl = new CheckoutDataLoader();

  return *dl;
}

QStringList CheckoutDataLoader::handledFiles()
{
  QStringList r;
  foreach (CheckoutDataLoaderPluginInterface* itf, _plugins)
    {
      r.append(itf->handledFiles());
    }
  return r;
}

bool CheckoutDataLoader::isFileHandled(QString s)
{
  foreach (CheckoutDataLoaderPluginInterface* itf, _plugins)
    {
      if (itf->isFileHandled(s))
        {
          return true;
        }
    }
  return false;
}

ExperimentFileModel *CheckoutDataLoader::getExperimentModel(QString file)
{
  _errors = QString();
  foreach (CheckoutDataLoaderPluginInterface* itf, _plugins)
    {
      if (itf->isFileHandled(file))
        {
          ExperimentFileModel* mdl = itf->getExperimentModel(file);
          QString temp = itf->errorString();

          if (!temp.isEmpty())

            _errors += QString("Plugin %1: Message: %2").arg(itf->pluginName()).arg(temp);

          return mdl;
        }
    }
  return 0;
}

void CheckoutDataLoader::addPlugin(CheckoutDataLoaderPluginInterface *plugin)
{
//	qDebug() << "Adding plugin : " << plugin;
  _plugins << plugin;
}

QString &CheckoutDataLoader::errorMessage()
{
  return _errors;
}
