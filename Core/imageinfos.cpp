#include "imageinfos.h"
#include "sequenceinteractor.h"
#include <algorithm>
#include <opencv2/imgproc.hpp>

#include <Core/checkouterrorhandler.h>


ImageInfos::ImageInfos(ImageInfosShared& ifo, SequenceInteractor *par, QString fname, QString platename, int channel):
    _ifo(ifo),
    _parent(par),
    _modified(true),
    _name(fname),
    _plate(platename),
    _lockImage(QMutex::NonRecursive),
    bias_correction(false),
    _saturate(true), _uninverted(true),

    _channel(channel), _binarized(false)
{
    QMutexLocker lock(&_lockImage);
    loadedWithkey = key();
    //qDebug() << "Imageinfos" << this << fname << platename << par;
    _ifo._platename_to_infos[_plate].append(this);
}

ImageInfos::~ImageInfos()
{
    _ifo._platename_to_infos[_plate].removeAll(this);
}

QString ImageInfos::key(QString k)
{
    static QString str;

    if (!k.isEmpty())
        str = k;

    return str;
}

QMutex protect_iminfos(QMutex::NonRecursive);


QPair<ImageInfosShared* , QMap<QString, ImageInfos*> *>
instanceHolder()
{

    static ImageInfosShared* data = nullptr;
    static QMap<QString, ImageInfos*> stored;
    if (data == nullptr)
    {
        data = new ImageInfosShared;
    }

    return qMakePair(data, &stored);
}


ImageInfos* ImageInfos::getInstance(SequenceInteractor* par, QString fname, QString platename, int channel,
                                    bool & exists, QString key)
{
    QPair<ImageInfosShared*, QMap<QString, ImageInfos*>*> t = instanceHolder();
    ImageInfosShared* data = t.first;
    QMap<QString, ImageInfos*>& stored = *t.second;

    QMutexLocker lock(&protect_iminfos); // Just in case 2 instances try to create   the ImageInfosShared struct

    ImageInfos* ifo = nullptr;

    // FIXME: Change stored image infos key : use XP / Workbench / deposit group
    QString k = key.isEmpty() ? ImageInfos::key() : key;

    if (data->_platename_palette_color.contains(platename + k)
            || data->_platename_palette_color.contains(platename))
        exists = true; // Avoid reupdating color on load...

    ifo = stored[fname+k];
    if (!ifo)
    {
        ifo = new ImageInfos(*data, par, fname, platename+k, channel);
        stored[fname+k] = ifo;
        exists = false;
//        qDebug() << "New Image Info instance" << ifo << fname << ImageInfos::key() << k << platename << par->getSequenceFileModel()->getOwner()->name();
    }
    else
        exists = true;

    
    return ifo;
}

void ImageInfos::deleteInstance()
{

    _ifo._infos_to_coreimage[this].clear();
    _ifo._infos_to_coreimage.remove(this);

    _ifo._platename_to_infos[_plate].removeAll(this);

    QPair<ImageInfosShared*, QMap<QString, ImageInfos*>*> t = instanceHolder();
    QMap<QString, ImageInfos*>& stored = *t.second;

    QString key;
    for (QMap<QString, ImageInfos*>::iterator it = stored.begin(); it != stored.end(); ++it)
    {
        if (it.value() == this)
            key = it.key();
    }
    _image.release();
    stored.remove(key);
}


cv::Mat ImageInfos::image(float scale, bool reload)
{
    QMap<unsigned, QColor> ext_cmap;
    QMutexLocker lock(&_lockImage);


    if (reload && !_name.startsWith(":/mem/"))
        _image = cv::Mat();

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

        if (scale < 1.0) // Only resize after loading data
            cv::resize(_image, _image, cv::Size(), scale, scale, cv::INTER_AREA);

        _size = QSize(_image.cols, _image.rows);

        if (_image.empty())
        {
            CheckoutErrorHandler::getInstance().addError(QString("Loading image went wrong %1").arg(_name));
            return _image;
        }

        if (_image.type() != CV_16U)
        {
            cv::Mat t;
            _image.convertTo(t, CV_16U);
            _image = t;
        }

        float min  = _ifo._platename_to_colorCode[_plate].min;
        float max = _ifo._platename_to_colorCode[_plate].max;
        for (int i = 0; i < _image.rows; ++i)
            for (int j = 0; j < _image.cols; ++j)
            {
                unsigned int v = _image.at<unsigned short>(i,j);
                if (v < min) min = v;
                else if ( v > max) max = v;
            }

        _ifo._platename_to_colorCode[_plate].min = std::min(min, _ifo._platename_to_colorCode[_plate].min);
        _ifo._platename_to_colorCode[_plate].max = std::max(max, _ifo._platename_to_colorCode[_plate].max);

        if (_ifo._platename_to_colorCode[_plate]._dispMax == -std::numeric_limits<float>::infinity())
            _ifo._platename_to_colorCode[_plate]._dispMax = _ifo._platename_to_colorCode[_plate].max;
        if (_ifo._platename_to_colorCode[_plate]._dispMin == std::numeric_limits<float>::infinity())
            _ifo._platename_to_colorCode[_plate]._dispMin = _ifo._platename_to_colorCode[_plate].min;

        _nbcolors = max-min;
        if (max >= 16) _nbcolors+=1;
        if (_nbcolors <= 16 && min >= 0)
        {
            // if ext_cmap
            int si = max-min+1;
            int start = _ifo._per_plateid[_plate];

            _ifo._platename_palette_color[_plate].resize(16);
            _ifo._platename_palette_state[_plate].fill(1, si);

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

                _ifo._platename_palette_color[_plate][i] = thecols[(start+i)%16];

            for (auto it = ext_cmap.begin(); it != ext_cmap.end(); ++it)
                _ifo._platename_palette_color[_plate][it.key()] = it.value();

            _ifo._per_plateid[_plate] = (start+si)%16;
        }

    }

    return _image;
}

cv::Mat ImageInfos::bias(int channel, float )
{
    if (_ifo.bias_field[_plate].contains(channel))
        return _ifo.bias_field[_plate][channel];

    cv::Mat bias;

    SequenceFileModel* mdl = _parent->getSequenceFileModel();
    QString bias_file = mdl->property(QString("ShadingCorrectionSource_ch%1").arg(channel));
    QFileInfo path(mdl->getFile(1, 1, 1, channel));
    bias_file = path.absolutePath() + "/" + bias_file;
    if (_ifo.bias_single_loader.contains(bias_file))
        bias = _ifo.bias_single_loader[bias_file];
    else
    {
        qDebug() << "Loading bias file" << bias_file;

        bias = cv::imread(bias_file.toStdString(), 2);
        _ifo.bias_single_loader[bias_file] = bias;
    }

    _ifo.bias_field[_plate][channel] = bias;
    return bias;
}

void ImageInfos::addCoreImage(CoreImage *ifo)
{
    if (!_ifo._infos_to_coreimage[this].contains(ifo))
        _ifo._infos_to_coreimage[this].append(ifo);
}



SequenceInteractor *ImageInfos::getInteractor()
{
    return _parent;
}

QList<ImageInfos *> ImageInfos::getLinkedImagesInfos()
{
    return  _ifo._platename_to_infos[_plate];
}

void ImageInfos::toggleBiasCorrection()
{
    this->bias_correction = !this->bias_correction;
}

void ImageInfos::toggleSaturate(){
    _saturate = !_saturate;


    propagate();
}

bool ImageInfos::isSaturated() { return _saturate; }

void ImageInfos::toggleInverted()
{
    _uninverted = !_uninverted;

    propagate();
}

bool ImageInfos::isInverted() { return !_uninverted; }

void ImageInfos::toggleBinarized()
{
    _binarized = !_binarized;

    propagate();
}

bool ImageInfos::isBinarized() { return _binarized; }

void ImageInfos::setColorMap(QString name){
    _colormap = name;
    propagate();
}




QString ImageInfos::colormap() { return _colormap; }

void ImageInfos::propagate()
{

    foreach (ImageInfos* ifo, _ifo._platename_to_infos[_plate])
    {
        if (ifo && ifo != this &&
                _channel == ifo->_channel) // Only propagate status to same channel & plates
        {
            ifo->_uninverted = this->_uninverted;
            ifo->_saturate = this->_saturate;
            ifo->_colormap = this->_colormap;
            ifo->_binarized = this->_binarized;
            ifo->_parent->modifiedImage();
        }

    }
    _parent->modifiedImage();


}

QList<CoreImage*> ImageInfos::getCoreImages()
{
    return _ifo._infos_to_coreimage[this];
}

bool ImageInfos::isTime() const
{
    return _parent->getTimePointCount() != 1;
}

double ImageInfos::getFps() const
{
    return _parent->getFps();
}


void ImageInfos::setColor(QColor c, bool refresh)
{
    _modified = true;
    c.setHsv(c.hsvHue(), c.hsvSaturation(), 255);

    //    qDebug() << "Setting color" << this << c;

    _ifo._platename_to_colorCode[_plate]._r = c.red();
    _ifo._platename_to_colorCode[_plate]._g = c.green();
    _ifo._platename_to_colorCode[_plate]._b = c.blue();

    if (refresh)
    {
        Update();
    }
}

void ImageInfos::setChannelName(QString name)
{
    channelName = name;
}

QString ImageInfos::getChannelName()
{
    return channelName;
}

void ImageInfos::setTile(int tile)
{
    //qDebug() << "Set Tile " << tile;
    _parent->setTile(tile);
    Update();
}

void ImageInfos::displayTile(bool disp)
{
    //qDebug() << "Toggling Tile disp:" << disp;
    _parent->displayTile(disp);
    Update();
}

bool ImageInfos::tileDisplayed()
{

    return _parent->tileDisplayed();
}

int ImageInfos::getTile()
{
    return _parent->getTile();
}


void ImageInfos::setColor(unsigned char r, unsigned char g, unsigned char b)
{
    setColor(QColor(r, g, b));
}

void ImageInfos::setColor(int code, unsigned char r, unsigned char g, unsigned char b)
{
    _modified = true;

    if (_ifo._platename_palette_color[_plate].size() < code)
        _ifo._platename_palette_color[_plate].resize(code);
    _ifo._platename_palette_color[_plate][code].setRgb(r,g,b);


}


QColor ImageInfos::getColor(int v)
{
    return _ifo._platename_palette_color[_plate][v];
}

QVector<QColor> ImageInfos::getPalette()
{
    return _ifo._platename_palette_color[_plate];
}

void ImageInfos::setPalette(QMap<unsigned, QColor> pal)
{

    unsigned max = 0;
    for (auto it = pal.begin(); it != pal.end(); ++it)
    {
        max = std::max(it.key(), max);
    }

    _ifo._platename_palette_color[_plate].resize(max);

    for (unsigned i = 0; i < max; ++i)
    {
        if (pal.contains(i))
            _ifo._platename_palette_color[_plate][i] = pal[i];
    }

}



QVector<int> ImageInfos::getState()
{
    return _ifo._platename_palette_state[_plate];
}

void ImageInfos::Update()
{
    foreach (ImageInfos* ifo, _ifo._platename_to_infos[_plate])
    {
        if (ifo && ifo != this)
            ifo->_parent->modifiedImage();
    }
    _parent->modifiedImage();
}

QSize ImageInfos::imSize() { return _size; }

QColor ImageInfos::getColor()
{
    return QColor::fromRgb(_ifo._platename_to_colorCode[_plate]._r, _ifo._platename_to_colorCode[_plate]._g, _ifo._platename_to_colorCode[_plate]._b);
}

void ImageInfos::setDefaultColor(int channel, bool )
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
    if (_ifo._platename_palette_color[_plate].size() < chan)
        _ifo._platename_palette_state[_plate].resize(chan);
    _ifo._platename_palette_state[_plate][chan] = !_ifo._platename_palette_state[_plate][chan];

    Update();
}

void ImageInfos::rangeChanged(double mi, double ma)
{
    _modified = true;

    _ifo._platename_to_colorCode[_plate]._dispMin = mi;
    _ifo._platename_to_colorCode[_plate]._dispMax = ma;

    Update();
}

// Changing the ranges max/min value does not need to update the image, also no need to set modified flag
void ImageInfos::forceMinValue(double val)
{
    // _modified = true;
    //  qDebug() << "Image min " << _platename_to_colorCode[_plate].min << _platename_to_colorCode[_plate].max << val;
    _ifo._platename_to_colorCode[_plate].min = val;
    //  _platename_to_colorCode[_plate]._dispMin = val;

    //   Update();
}

void ImageInfos::forceMaxValue(double val)
{
    // _modified = true;
    // s qDebug() << "Image max " << _platename_to_colorCode[_plate].min << _platename_to_colorCode[_plate].max << val;
    _ifo._platename_to_colorCode[_plate].max = val;
    //  _platename_to_colorCode[_plate]._dispMax = val;

    //  Update();
}

void ImageInfos::changeFps(double fps)
{
    _modified = true;
    _parent->setFps(fps);

    Update();
}

void ImageInfos::rangeMinValueChanged(double mi)
{
    _modified = true;
    qDebug() << "Image infos range min" << this << mi ;
    _ifo._platename_to_colorCode[_plate]._dispMin = mi;

    Update();
}

void ImageInfos::rangeMaxValueChanged(double ma)
{
    _modified = true;
    qDebug() << "Image infos Range max" << this << ma;
    _ifo._platename_to_colorCode[_plate]._dispMax = ma;

    Update();
}


void ImageInfos::setActive(bool value)
{
    _modified = true;
    _ifo._platename_to_colorCode[_plate]._active = value;

    Update();
}
