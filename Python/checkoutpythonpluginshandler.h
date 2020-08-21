#ifndef CHECKOUTPYTHONPLUGINSHANDLER_H
#define CHECKOUTPYTHONPLUGINSHANDLER_H

#include <Core/checkoutprocessplugininterface.h>

class QFileSystemWatcher;
class PythonProcess;

class DllPythonExport CheckoutPythonPluginsHandler: public QObject
{
  Q_OBJECT
public:
  CheckoutPythonPluginsHandler();
  ~CheckoutPythonPluginsHandler();

  void monitorPythonPluginPath(QStringList paths);


public slots:
  void directoryChanged(const QString& path);
  void fileChanged(const QString& path);

protected:
  QFileSystemWatcher *_fileWatcher;
  QFileSystemWatcher *_directoryWatcher;

  QMap<QString, PythonProcess*> _scripts;
};

#endif // CHECKOUTPYTHONPLUGINSHANDLER_H
