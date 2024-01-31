#ifndef SEQUENCEINTERACTOR_H
#define SEQUENCEINTERACTOR_H

#include <QtCore>

#include "wellplatemodel.h"

#include "Core/Dll.h"
#include <opencv2/highgui/highgui.hpp>
#include <QColor>

#include <QPixmap>
#include <QGraphicsItem>
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
    QPixmap getPixmap(bool packed = true, bool bias_correction = false, float scale = 1.);

    QPixmap getSubPixmap(QString overlay, int id);
    void setTag(QString overlay, int id, QStringList tags);

    QStringList getTag(QString overlay, int id);
    QStringList getOverlayValues(QString overlay, int id);
    int getTagSize(QString overlay);

    QImage getPixmapChannels(int field, bool bias_correction = false, float scale = 1.);
    QList<unsigned> getData(QPointF d, int& field, bool packed = true, bool bias = false);

    QList<QGraphicsItem*> getMeta(QGraphicsItem* parent);

    void getResolution(float& x, float &y);


    // Return all channels file for current selection
    QList<QPair<int, QString> > getAllChannel(int field = -1);
    int getChannelsFromFileName(QString file);

    QStringList getAllTimeFieldSameChannel();


    QImage getAllChannelsImage();

    //  void connectChannelRange(ctkDoubleRangeSlider* sld, unsigned channel);
    ImageInfos* getChannelImageInfos(unsigned channel);

    void setCurrent(SequenceInteractor* i);

    SequenceFileModel* getSequenceFileModel();

    SequenceInteractor* current();

    QString getExperimentName();
    QString getProjectName();

    void addImage(CoreImage*ci);
    void clearMemory(CoreImage *im);

    //signals:
    void modifiedImage();
    QPointF getFieldPosition(int field = -1);
    //protected:
    ImageInfos *imageInfos(QString file, int channel = -1, QString key = QString());
    void refinePacking();


    bool fieldDetails();
    void changeFieldDetails(bool v);

    bool overlayDisplayed(QString name);
    int getOverlayId(QString name);
    int getOverlayMax(QString name);

    double getOverlayWidth();
    void setOverlayWidth(double v);

    bool currentChanged();
    void changed();
    QList<QString> getMetaList();
    QList<QString> getMetaOptionsList(QString meta);
    void overlayChange(QString name, QString id);
    QString getOverlayCode(QString name);
    QString getOverlayColor(QString name);
    void exportOverlay(QString name, QString tofile);

    void initImageInfos(int field);

    QSet<int> getChannelsIds();
    QStringList getMetaTags(int idx);
    void addMetaTags(int idx);

public slots:

    void setTimePoint(unsigned t);
    void setField(unsigned t);
    void setZ(unsigned z);
    void setChannel(unsigned c);
    void setFps(double fps);

    void setOverlayId(QString name,  int tile);
    void toggleOverlay(QString name, bool disp);

    QStringList getChannelNames();

    void setChannelNames(QStringList names);


    bool isUpdating() {
        return _updating
                ;
    }
    void overlayChangeCmap(QString name, QString id);
    void setTagFilter(QStringList t);
    QStringList tagFilter();
protected:

    SequenceFileModel* _mdl;
    unsigned _timepoint, _field, _zpos, _channel;
    double _fps;

    QMap<QString, QPair<bool, int> > disp_overlay; // Toggle / tile id
    QMap<QString, QPair<QString, QString> > overlay_coding; // Feature / Colormap

    //  bool disp_tile;
    //x  int tile_id;

    QString loadkey;
    QStringList channel_names;

    //QList<ImageForm*> linked_images;
    QMap<QString, ImageInfos*> _infos;
    QVector<QPoint> pixOffset;

    QList<CoreImage*> _ImageList;
    QPixmap           _cachePixmap; // Cache temporary image, pixmap are implicitly shared
    static SequenceInteractor* _current;
    float last_scale;
    bool _updating, _changed;

    static double _overlay_width;

    //
    static bool showFieldDetails;


    QStringList overlay_tag_filter;

    static QColor _textColor;
    static QColor _barColor;

    static float _offsetTextX;
    static float _offsetTextY;
    static float _offsetBarX;
    static float _offsetBarY;

    static int _textSize;


};


#endif // SEQUENCEINTERACTOR_H
