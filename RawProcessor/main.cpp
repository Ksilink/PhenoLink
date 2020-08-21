#include <ostream>
#include <QApplication>

#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <QStandardPaths>

#include <QtSql>
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
  a.setApplicationName("Checkout");
  a.setApplicationVersion(CHECKOUT_VERSION);
  a.setApplicationDisplayName(QString("Checkout %1").arg(CHECKOUT_VERSION));
  a.setOrganizationDomain("WD");
  a.setOrganizationName("WD");

  QDir dir( QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());
//  dir.mkpath(dir.absolutePath());

//  _logFile.setFileName(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/WD_CheckoutLog.txt");
//  _logFile.open(QIODevice::WriteOnly);


//  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
//  db.setDatabaseName(dir.absolutePath() + "/WD_Checkout" + QString("%1").arg(CHECKOUT_VERSION) + ".db");
//  bool ok = db.open();

//  if (!ok)
//    {
//      QMessageBox::warning(0x0, "Database warning", "Database was not loadable");
//      return -1;
//    }

  PluginManager::loadPlugins(true);
  RawProcMainWin w;

  w.show();
  int res = a.exec();

  return res;
}

