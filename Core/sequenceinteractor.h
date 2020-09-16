#ifndef SEQUENCEINTERACTOR_H
#define SEQUENCEINTERACTOR_H

#include <QtCore>

#include "wellplatemodel.h"

#include "Core/Dll.h"
#include <opencv2/highgui/highgui.hpp>
#include <QColor>

#include <QPixmap>
#include "imageinfos.h"

class ImageForm;

class DllCoreExport SequenceInteractor: public QObject
{
  Q_OBJECT
public:
    SequenceInteractor();
    SequenceInteractor(SequenceFileModel* mdl, QString key);

  unsigned getTimePointCount();
  unsigned getFieldCount();
  unsigned getZCount();
  unsigned getChannels();

  unsigned getTimePoint();
  unsigned getField();
  unsigned getZ();
  unsigned getChannel();


  double getFps();

  QString getFileName();


  // Returns file for current selection
  QString getFile();
  void preloadImage();
  QPixmap getPixmap(bool packed = true, float scale = 1.);
  QImage getPixmapChannels(int field, float scale = 1.);
  QList<unsigned> getData(QPointF d);


  // Return all channels file for current selection
  QStringList getAllChannel(int field = -1);
  int getChannelsFromFileName(QString file);

  QImage getAllChannelsImage();

//  void connectChannelRange(ctkDoubleRangeSlider* sld, unsigned channel);
  ImageInfos* getChannelImageInfos(unsigned channel);

  void setCurrent(SequenceInteractor* i);

  SequenceFileModel* getSequenceFileModel();

  SequenceInteractor* current();

  QString getExperimentName();

  void addImage(CoreImage*ci);
  void clearMemory();
//signals:
  void modifiedImage();
  QPointF getFieldPosition(int field = -1);
//protected:
  ImageInfos *imageInfos(QString file, int channel = -1, QString key = QString());

public slots:

  void setTimePoint(unsigned t);
  void setField(unsigned t);
  void setZ(unsigned z);
  void setChannel(unsigned c);
  void setFps(double fps);


protected:

  SequenceFileModel* _mdl;
  unsigned _timepoint, _field, _zpos, _channel;
  double _fps;

  QString loadkey;

  //QList<ImageForm*> linked_images;
  //QMap<QString, ImageInfos*> _infos;

  QList<CoreImage*> _ImageList;
  QPixmap           _cachePixmap; // Cache temporary image, pixmap are implicitly shared
  static SequenceInteractor* _current;
  float last_scale;
};


#endif // SEQUENCEINTERACTOR_H
