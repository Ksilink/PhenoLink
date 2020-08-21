#ifndef SCREENSLOADER_H
#define SCREENSLOADER_H

#include <QString>

#include "wellplatemodel.h"

class ScreensLoader
{
public:
  ScreensLoader(QString file);
  virtual ExperimentFileModel* getExperimentModel() = 0;

protected:
  QString _file;
};



#endif // SCREENSLOADER_H
