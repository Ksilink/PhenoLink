#ifndef IMAGEINFOS_H
#define IMAGEINFOS_H



#include <QtCore>

#include "wellplatemodel.h"

#include "Core/Dll.h"
#include <opencv2/highgui/highgui.hpp>
#include <QColor>

class  SequenceInteractor;

struct CommonColorCode
{
public:
    CommonColorCode():
        min(std::numeric_limits<float>::infinity()),
        max(-std::numeric_limits<float>::infinity()),
        _dispMin(std::numeric_limits<float>::infinity()),
        _dispMax(-std::numeric_limits<float>::infinity()),
        _active(true)
    {}

    float min, max;
    float _dispMin, _dispMax;

    unsigned char _r, _g, _b;
    bool _active;
};

class CoreImage
{
public:
    virtual void modifiedImage() = 0;
    virtual void changeFps(double fps) = 0;
};

class ImageInfos;

struct ImageInfosShared
{
    QMap<QString, CommonColorCode> _platename_to_colorCode;
    QMap<QString, QList<ImageInfos*> > _platename_to_infos;
    QMap<ImageInfos*, QList<CoreImage* > > _infos_to_coreimage;
    QMap<QString, QMap<int, cv::Mat> > bias_field; // Per plate bias field, channel
    QMap<QString, cv::Mat> bias_single_loader; // Use this to only load once the file!

    QMap<QString, QVector<QColor> > _platename_palette_color;
    QMap<QString, QVector<int> > _platename_palette_state;
    QMap<QString, int> _per_plateid;
};


class DllCoreExport ImageInfos: public QObject
{
    Q_OBJECT
public:

    ImageInfos(ImageInfosShared& ifo, SequenceInteractor* par, QString fname, QString platename, int channel);
    ~ImageInfos();


    static QString key(QString k = QString());

    static ImageInfos* getInstance(SequenceInteractor* par, QString fname, QString platename,int channel,
                                   bool& exists, QString key = QString());

    static QMap<QString, ImageInfos *> getInstances();

    void deleteInstance();


    cv::Mat image(float scale = 1., bool reload = false);
    cv::Mat bias(int channel, float scale = 1.);

    void addCoreImage(CoreImage *ifo);
    inline bool active() const { return _ifo._platename_to_colorCode[_plate]._active; }

    QList<CoreImage*> getCoreImages();

    SequenceInteractor* getInteractor();
    QList<ImageInfos*> getLinkedImagesInfos();

    void toggleBiasCorrection();
    void toggleSaturate();
    bool isSaturated();

    void toggleInverted();
    bool isInverted();


    void toggleBinarized();

    bool isBinarized();


    void setColorMap(QString name);
    QString colormap() ;

    void propagate();

    bool isTime() const ;
    double getFps() const;

    inline float getMin() const {return _ifo._platename_to_colorCode[_plate].min; }
    inline float getMax() const { return _ifo._platename_to_colorCode[_plate].max; }

    inline float getDispMin() const {return _ifo._platename_to_colorCode[_plate]._dispMin; qDebug() << "Disp Min" << _ifo._platename_to_colorCode[_plate]._dispMin; }
    inline float getDispMax() const { return _ifo._platename_to_colorCode[_plate]._dispMax; qDebug() << "Disp Max" << _ifo._platename_to_colorCode[_plate]._dispMax; }
    int nbColors() { return _nbcolors; }

    inline unsigned char Red() const{ return _ifo._platename_to_colorCode[_plate]._r; }
    inline unsigned char Green() const { return _ifo._platename_to_colorCode[_plate]._g; }
    inline unsigned char Blue() const { return _ifo._platename_to_colorCode[_plate]._b; }

    void setColor(unsigned char r, unsigned char g, unsigned char b);


    void setColor(int code, unsigned char r, unsigned char g, unsigned char b);


    QColor getColor();

    void setRed(unsigned char r) {_modified = true; _ifo._platename_to_colorCode[_plate]._r = r; }
    void setGreen(unsigned char r) { _modified = true; _ifo._platename_to_colorCode[_plate]._g = r; }
    void setBlue(unsigned char r) {_modified = true; _ifo._platename_to_colorCode[_plate]._b = r; }
    void setDefaultColor(int chan, bool refresh=true);



    bool isModified() { return _modified; }
    void resetModify() { _modified = false; }
    void setModified(bool val = true) { _modified = val; }

    QColor getColor(int v);
    QVector<QColor> getPalette();
    void setPalette(QMap<unsigned, QColor> pal);

    QVector<int> getState();

    void Update();
    QSize imSize();

    bool overlayDisplayed(QString name);
    int getOverlayId(QString name);
    int getOverlayMin(QString name);
    int getOverlayMax(QString name);

    double getOverlayWidth();

    QJsonObject getStoredParams();
    void timerEvent(QTimerEvent *event);

public slots:
    void changeColorState(int chan);

    void rangeChanged(double mi, double ma) ;

    void  forceMinValue(double val);
    void  forceMaxValue(double val);

    void changeFps(double fps);

    void rangeMinValueChanged(double mi) ;
    void rangeMaxValueChanged(double ma) ;

    void setActive(bool value, bool update=true);
    void setColor(QColor c, bool refresh=true);

    void setChannelName(QString name);
    QString getChannelName();


    void displayTile(bool disp);
    void setTile( int tile);

//    void update_range_ontime();


signals:
    void updateRange(double, double);

protected:


    QString loadedWithkey;
    QString channelName;

    ImageInfosShared& _ifo;
    SequenceInteractor* _parent;
    bool _modified;
    int _nbcolors;
    QString _name, _plate;

    cv::Mat _image;
    QMutex _lockImage;
    double _fps;
    bool bias_correction;
    bool _saturate, _uninverted;
    int _channel;
    QString _colormap;
    bool _binarized;
    QSize _size;

    QTimer* range_timer;
    int range_timerId;

};


#endif // IMAGEINFOS_H
