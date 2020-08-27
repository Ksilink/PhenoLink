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
};


class DllCoreExport ImageInfos: public QObject
{
    Q_OBJECT
public:
    ImageInfos(SequenceInteractor* par, QString fname, QString platename);
    ~ImageInfos();

    cv::Mat& image();


    inline bool active() const { return _platename_to_colorCode[_plate]._active; }

    inline bool isTime() const ;

    inline float getMin() const {return _platename_to_colorCode[_plate].min; }
    inline float getMax() const { return _platename_to_colorCode[_plate].max; }

    inline float getDispMin() const {return _platename_to_colorCode[_plate]._dispMin; }
    inline float getDispMax() const { return _platename_to_colorCode[_plate]._dispMax; }
    int nbColors() { return _nbcolors; }

    inline unsigned char Red() const{ return _platename_to_colorCode[_plate]._r; }
    inline unsigned char Green() const { return _platename_to_colorCode[_plate]._g; }
    inline unsigned char Blue() const { return _platename_to_colorCode[_plate]._b; }

    void setColor(unsigned char r, unsigned char g, unsigned char b);


    void setColor(int code, unsigned char r, unsigned char g, unsigned char b);


    QColor getColor();

    void setRed(unsigned char r) {_modified = true; _platename_to_colorCode[_plate]._r = r; }
    void setGreen(unsigned char r) { _modified = true; _platename_to_colorCode[_plate]._g = r; }
    void setBlue(unsigned char r) {_modified = true; _platename_to_colorCode[_plate]._b = r; }
    void setDefaultColor(int chan);



    bool isModified() { return _modified; }
    void resetModify() { _modified = false; }
    void setModified(bool val = true) { _modified = val; }

    QColor getColor(int v);
    QVector<QColor> getPalette();
    void setPalette(QMap<unsigned, QColor> pal);

    QVector<int> getState();
public slots:
    void changeColorState(int chan);

    void rangeChanged(double mi, double ma) ;

    void  forceMinValue(double val);
    void  forceMaxValue(double val);


    void rangeMinValueChanged(double mi) ;
    void rangeMaxValueChanged(double ma) ;

    void setActive(bool value);

    void setColor(QColor c);

protected:

    SequenceInteractor* _parent;
    bool _modified;
    int _nbcolors;
    QString _name, _plate;
    static QMap<QString, CommonColorCode> _platename_to_colorCode;
    static QMap<QString, QList<ImageInfos*> > _platename_to_infos;

    static QMap<QString, QVector<QColor> > _platename_palette_color;
    static QMap<QString, QVector<int> > _platename_palette_state;
    static QMap<QString, int> _per_plateid;

    cv::Mat _image;
    QMutex _lockImage;

};


#endif // IMAGEINFOS_H
