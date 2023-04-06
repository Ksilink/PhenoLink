#include <ostream>
#include <QApplication>

#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <QStandardPaths>

#include <QDir>

#include "Core/config.h"

#include "Core/pluginmanager.h"
#include "Core/networkprocesshandler.h"

#include "RawProcessor/rawprocmainwin.h"

#include <QMessageBox>

int main(int argc, char *argv[])
{
  //    qInstallMessageHandler(myMessageOutput);

  QApplication a(argc, argv);
  a.setApplicationName("PhenoLink");
  a.setApplicationVersion(CHECKOUT_VERSION);
  a.setApplicationDisplayName(QString("PhenoLink %1").arg(CHECKOUT_VERSION));
  a.setOrganizationDomain("WD");
  a.setOrganizationName("WD");

  QDir dir( QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());

  PluginManager::loadPlugins(true);
  RawProcMainWin w;

  w.show();
  int res = a.exec();

  return res;
}

