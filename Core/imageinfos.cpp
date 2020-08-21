#include "imageinfos.h"
#include "sequenceinteractor.h"
#include <algorithm>

QMap<QString, CommonColorCode>      ImageInfos::_platename_to_colorCode;
QMap<QString, QList<ImageInfos*> >  ImageInfos::_platename_to_infos;
QMap<QString, QVector<QColor> >     ImageInfos::_platename_palette_color;
QMap<QString, QVector<int> >        ImageInfos::_platename_palette_state;
QMap<QString, int>                  ImageInfos::_per_plateid;

//QMutex platenameProtect;

ImageInfos::ImageInfos(SequenceInteractor *par, QString fname, QString platename):
    _parent(par),
    _modified(true),
    _name(fname),
    _plate(platename)
{
    _platename_to_infos[platename] << this;

    //    qDebug() << "Imageinfos" << this << fname << platename << par;

}

ImageInfos::~ImageInfos()
{
    _platename_to_infos[_plate].removeAll(this);
}




cv::Mat &ImageInfos::image()
{
    QMap<unsigned, QColor> ext_cmap;
    QMutexLocker lock(&_lockImage);

    if (_image.empty())
    {

        if (_name.startsWith(":/mem/"))
        {
            cv::Mat* mat  = static_cast<cv::Mat*>(MemoryHandler::handler().getData(_name));
            if (mat)
            {
                _image = mat->clone();
                //              mat->copyTo(_image);
                mat->release();
                ext_cmap = MemoryHandler::handler().getColor(_name);
                MemoryHandler::handler().release(_name);
            }

        }
        else
            _image = cv::imread(_name.toStdString(), 2);

        if (_image.type() != CV_16U)
        {
            cv::Mat t;
            _image.convertTo(t, CV_16U);
            _image = t;
        }

        float min  = _platename_to_colorCode[_plate].min;
        float max = _platename_to_colorCode[_plate].max;
        for (int i = 0; i < _image.rows; ++i)
            for (int j = 0; j < _image.cols; ++j)
            {
                unsigned int v = _image.at<unsigned short>(i,j);
                if (v < min) min = v;
                else if ( v > max) max = v;
            }

        _platename_to_colorCode[_plate].min = std::min(min, _platename_to_colorCode[_plate].min);
        _platename_to_colorCode[_plate].max = std::max(max, _platename_to_colorCode[_plate].max );

        if (_platename_to_colorCode[_plate]._dispMax == -std::numeric_limits<float>::infinity())
            _platename_to_colorCode[_plate]._dispMax = _platename_to_colorCode[_plate].max;
        if (_platename_to_colorCode[_plate]._dispMin == std::numeric_limits<float>::infinity())
            _platename_to_colorCode[_plate]._dispMin = _platename_to_colorCode[_plate].min;

        _nbcolors = max-min;
        if (max >= 16) _nbcolors+=1;
        if (_nbcolors <= 16 && min >= 0)
        {
// if ext_cmap
            int si = max-min+1;
            int start = _per_plateid[_plate];

            _platename_palette_color[_plate].resize(16);
            _platename_palette_state[_plate].fill(1, si);

            QVector<QColor> thecols(16);

            thecols[0] = (Qt::black);
            thecols[1] = (Qt::yellow);
            thecols[2] = (Qt::red);
            thecols[3] = (Qt::green);
            thecols[4] = (Qt::blue);
            thecols[5] = (Qt::cyan);
            thecols[6] = (Qt::magenta);
            thecols[7] = (Qt::darkRed);
            thecols[8] = (Qt::darkGreen);
            thecols[9] = (Qt::darkBlue);
            thecols[10] = (Qt::darkCyan);
            thecols[11] = (Qt::darkMagenta);
            thecols[12] = (Qt::darkYellow);
            thecols[13] = (Qt::gray);
            thecols[14] = (Qt::lightGray);
            thecols[15] = (Qt::darkGray);

            for (int i = 0; i < 16; ++i)

                _platename_palette_color[_plate][i] = thecols[(start+i)%16];

            for (auto it = ext_cmap.begin(); it != ext_cmap.end(); ++it)
                _platename_palette_color[_plate][it.key()] = it.value();

            _per_plateid[_plate] = (start+si)%16;
        }

    }

    //  _modified = false;
    return _image;
}

void ImageInfos::setColor(unsigned char r, unsigned char g, unsigned char b)
{
    _modified = true;
    _platename_to_colorCode[_plate]._r = r;
    _platename_to_colorCode[_plate]._g = g;
    _platename_to_colorCode[_plate]._b = b;
}

void ImageInfos::setColor(int code, unsigned char r, unsigned char g, unsigned char b)
{
    _modified = true;
    //        _platename_to_colorCode[_plate]._r = r;
    //        _platename_to_colorCode[_plate]._g = g;
    //        _platename_to_colorCode[_plate]._b = b;

    if ( _platename_palette_color[_plate].size() < code)
        _platename_palette_color[_plate].resize(code);
    _platename_palette_color[_plate][code].setRgb(r,g,b);


}


QColor ImageInfos::getColor(int v)
{
    return _platename_palette_color[_plate][v];
}

QVector<QColor> ImageInfos::getPalette()
{
    return _platename_palette_color[_plate];
}

void ImageInfos::setPalette(QMap<unsigned, QColor> pal)
{

    unsigned max = 0;
    for (auto it = pal.begin(); it != pal.end(); ++it)
    {
        max = std::max(it.key(), max);
    }

    _platename_palette_color[_plate].resize(max);

    for (unsigned i = 0; i < max; ++i)
    {
        if (pal.contains(i))
            _platename_palette_color[_plate][i] = pal[i];
    }

}



QVector<int> ImageInfos::getState()
{
    return _platename_palette_state[_plate];
}

QColor ImageInfos::getColor()
{
    return QColor::fromRgb(_platename_to_colorCode[_plate]._r,_platename_to_colorCode[_plate]._g,_platename_to_colorCode[_plate]._b);
}

void ImageInfos::setDefaultColor(int channel)
{
    channel --;
    _modified = true;
    setColor(255,255,255); // Default to white

    if (channel == 0) setColor(255,0,0);
    if (channel == 1) setColor(0,255,0);
    if (channel == 2) setColor(0,0,255);
    if (channel == 3) setColor(255,0,255);
    if (channel == 4) setColor(255,255,0);
    if (channel == 5) setColor(0,255,255);

}

void ImageInfos::changeColorState(int chan)
{
    if ( _platename_palette_color[_plate].size() < chan)
        _platename_palette_state[_plate].resize(chan);
    _platename_palette_state[_plate][chan] = !_platename_palette_state[_plate][chan];
    foreach (ImageInfos* ifo, _platename_to_infos[_plate])
    {
        ifo->_parent->modifiedImage();

    }
}

void ImageInfos::rangeChanged(double mi, double ma)
{
    _modified = true;

    _platename_to_colorCode[_plate]._dispMin = mi;
    _platename_to_colorCode[_plate]._dispMax = ma;

    foreach (ImageInfos* ifo, _platename_to_infos[_plate])
    {
        ifo->_parent->modifiedImage();

    }
}

void ImageInfos::forceMinValue(double val)
{
    _modified = true;
    //  qDebug() << "Image min " << _platename_to_colorCode[_plate].min << _platename_to_colorCode[_plate].max << val;
    _platename_to_colorCode[_plate].min = val;
    //  _platename_to_colorCode[_plate]._dispMin = val;

    foreach (ImageInfos* ifo, _platename_to_infos[_plate])
    {
        ifo->_parent->modifiedImage();

    }
}

void ImageInfos::forceMaxValue(double val)
{
    _modified = true;
    // s qDebug() << "Image max " << _platename_to_colorCode[_plate].min << _platename_to_colorCode[_plate].max << val;
    _platename_to_colorCode[_plate].max = val;
    //  _platename_to_colorCode[_plate]._dispMax = val;
    foreach (ImageInfos* ifo, _platename_to_infos[_plate])
    {
        ifo->_parent->modifiedImage();

    }
}

void ImageInfos::rangeMinValueChanged(double mi)
{
    _modified = true;
    //  qDebug() << "Image infos " << mi << ma;
    _platename_to_colorCode[_plate]._dispMin = mi;

    foreach (ImageInfos* ifo, _platename_to_infos[_plate])
    {
        ifo->_parent->modifiedImage();

    }
}

void ImageInfos::rangeMaxValueChanged(double ma)
{
    _modified = true;
    //  qDebug() << "Image infos " << mi << ma;
    _platename_to_colorCode[_plate]._dispMax = ma;

    foreach (ImageInfos* ifo, _platename_to_infos[_plate])
    {
        ifo->_parent->modifiedImage();
    }
}


void ImageInfos::setActive(bool value)
{
    _modified = true;
    _platename_to_colorCode[_plate]._active = value;
    foreach (ImageInfos* ifo, _platename_to_infos[_plate])
        ifo->_parent->modifiedImage();
}

void ImageInfos::setColor(QColor c)
{
    _modified = true;
    //  qDebug() << "Setting color" <<  c;
    c.setHsv(c.hsvHue(), c.hsvSaturation(), 255);

    _platename_to_colorCode[_plate]._r = c.red();
    _platename_to_colorCode[_plate]._g = c.green();
    _platename_to_colorCode[_plate]._b = c.blue();

    foreach (ImageInfos* ifo, _platename_to_infos[_plate])
        ifo->_parent->modifiedImage();
}
