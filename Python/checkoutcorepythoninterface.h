#ifndef CHECKOUTCOREPYTHONINTERFACE_H
#define CHECKOUTCOREPYTHONINTERFACE_H

#include <QString>
#include <Gui/mainwindow.h>

#include "Core/Dll.h"

class DllPythonExport CheckoutCorePythonInterface: public QObject
{
  Q_OBJECT
public:
  CheckoutCorePythonInterface();
  ~CheckoutCorePythonInterface();


  void runCode(QString code);
  void runFile(QString filename);

  void setSelectedWellPlates(QStringList wellp);
  QStringList getSelectedWellplates();

  void co_addDirectory(QString name);
  void co_deleteDirectory(QString name);
  void co_loadWellplates(QStringList plates);

  void co_displayWells();

  void co_addImages(QStringList files);

signals:

  void addDirectory(QString name);
  void deleteDirectory(QString name);
  void loadWellplates(QStringList plates);

  void addImage(QStringList mdl);

  void displayWellSelection();

protected:
  QStringList selectedWellplates;

};

#endif // CHECKOUTCOREPYTHONINTERFACE_H
