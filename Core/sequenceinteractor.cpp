#include "sequenceinteractor.h"

#include <QImage>

#include "Gui/ctkWidgets/ctkDoubleRangeSlider.h"

#include <QtConcurrent/QtConcurrent>
#include <QMutex>
#include <QPainter>
#include <QElapsedTimer>


QMutex sequence_interactorMutex;

SequenceInteractor::SequenceInteractor(): _mdl(0), _timepoint(1), _field(1), _zpos(1), _channel(1), _fps(25.),last_scale(-1.)
{
}

SequenceInteractor::SequenceInteractor(SequenceFileModel *mdl, QString key):
    _mdl(mdl), _timepoint(1), _field(1), _zpos(1), _channel(1),
    _fps(25.), loadkey(key), last_scale(-1.)
{
}

void SequenceInteractor::setTimePoint(unsigned t)
{
    if (_mdl->getTimePointCount() >= t)
        _timepoint = t;
}


void SequenceInteractor::setField(unsigned t)
{
    if (_mdl->getFieldCount() >= t)
        _field = t;

    QString nm = _mdl->getFile(_timepoint, _field, _zpos, _channel);
    ImageInfos* ifo = imageInfos(nm, _channel, loadkey);
    if (ifo)
    {
        QList<ImageInfos*> list = ifo->getLinkedImagesInfos();
        foreach(ImageInfos* info, list)
        {
            SequenceInteractor* inter = info->getInteractor();
            if (inter && inter != _current && inter != this)
            {
                inter->setField(t);
                inter->modifiedImage();
            }
        }
    }


}

void SequenceInteractor::setZ(unsigned z)
{
    if (_mdl->getZCount() >= z)
        _zpos = z;

    QString nm = _mdl->getFile(_timepoint, _field, _zpos, _channel);
    ImageInfos* ifo = imageInfos(nm, _channel, loadkey);
    if (ifo)
    {
        QList<ImageInfos*> list = ifo->getLinkedImagesInfos();
        foreach(ImageInfos* info, list)
        {
            SequenceInteractor* inter = info->getInteractor();
            if (inter && inter != _current)
            {
                inter->setZ(z);
                inter->modifiedImage();
            }
        }
    }
}

void SequenceInteractor::setChannel(unsigned c)
{
    if (_mdl->getChannels() >= c)
        _channel = c;
}

void SequenceInteractor::setFps(double fps)
{
    _fps = fps;

    //    foreach (ImageInfos* im, _infos.values())
    //        im->changeFps(fps);

    foreach (CoreImage* im, _ImageList)
        im->changeFps(fps);



}

QStringList SequenceInteractor::getChannelNames()
{
    return channel_names;
}

void SequenceInteractor::setChannelNames(QStringList names)
{
    channel_names = names;
}

unsigned SequenceInteractor::getFieldCount()
{
    return _mdl->getFieldCount();
}

unsigned SequenceInteractor::getChannels()
{
    return _mdl->getChannels();

}

unsigned SequenceInteractor::getTimePoint()
{
    return _timepoint;
}

unsigned SequenceInteractor::getField()
{
    return _field;
}

unsigned SequenceInteractor::getZ()
{
    return _zpos;
}

unsigned SequenceInteractor::getChannel()
{
    return _channel;
}

double SequenceInteractor::getFps()
{


    return _fps;
}

QString SequenceInteractor::getFileName()
{
    return _mdl->getOwner()->fileName();
}

unsigned SequenceInteractor::getZCount()
{
    return _mdl->getZCount();

}

unsigned SequenceInteractor::getTimePointCount()
{
    return _mdl->getTimePointCount();
}

QString SequenceInteractor::getFile()
{
    return _mdl->getFile(_timepoint, _field, _zpos, _channel);
}


QStringList SequenceInteractor::getAllChannel(int field)
{
    QStringList l;

    if (field <= 0) field = _field;
    SequenceFileModel::Channel& chans = _mdl->getChannelsFiles(_timepoint, field, _zpos);

    for (SequenceFileModel::Channel::const_iterator it = chans.cbegin(), e = chans.cend();
         it != e; ++it)
        if (!it.value().isEmpty())   l <<   it.value();

    return l;
}

int SequenceInteractor::getChannelsFromFileName(QString file)
{
    SequenceFileModel::Channel& chans = _mdl->getChannelsFiles(_timepoint, _field, _zpos);

    for (SequenceFileModel::Channel::iterator it = chans.begin(), e = chans.end(); it != e; ++it)
    {
        //qDebug() << it.value() << file;
        if (it.value() == file) return it.key();
    }
    return -1;
}

QStringList SequenceInteractor::getAllTimeFieldSameChannel()
{
    QStringList l;

    for (unsigned t = 1; t <= getTimePoint(); ++t)
        for (unsigned f = 1; f <= getFieldCount(); ++f)
            l << _mdl->getFile(t, f, _zpos, _channel);

    return l;
}

QImage SequenceInteractor::getAllChannelsImage()
{
    /// FIXME: Not done yet
    return QImage();
}

QString SequenceInteractor::getExperimentName()
{
    QStringList file = getFileName().split("/", Qt::SkipEmptyParts);
    file.pop_back();
    QString exp = file.last(); file.pop_back();
    exp += file.last();
    return exp;
}

void SequenceInteractor::addImage(CoreImage *ci)
{
    this->_ImageList << ci;
    //
    QString nm = _mdl->getFile(_timepoint, _field, _zpos, _channel);

    //  qDebug() << "Channel Image infos" << nm;
    //if (!_infos.contains(nm))
    //    _infos[nm] =

    ImageInfos *ifo = imageInfos(nm, _channel);

    ifo->addCoreImage(ci);
}

void SequenceInteractor::clearMemory()
{
    /*  foreach (ImageInfos* im, _infos.values())
        delete im;
    _infos.clear();//


*/
    QString exp = getExperimentName();// +_mdl->Pos();

    for (unsigned ii = 1; ii <= _mdl->getChannels(); ++ii)
    {
        QString nm = _mdl->getFile(_timepoint, _field, _zpos, ii);

        bool exists = false;
        ImageInfos* info = ImageInfos::getInstance(this, nm, exp + QString("%1").arg(ii), ii, exists, loadkey);
        info->deleteInstance();
        delete info;
    }
    //    qDebug() << "FIXME: Clear Memory called for SequenceInteractor, but may not be honored";
}


void SequenceInteractor::modifiedImage()
{
    //    qDebug() << "Interactor Modified Image";
    foreach (CoreImage* ci, _ImageList)
    {
        ci->modifiedImage();
    }

    QString nm = _mdl->getFile(_timepoint, _field, _zpos, _channel);
    ImageInfos* ifo = imageInfos(nm, _channel, loadkey);
    foreach(CoreImage* ci, ifo->getCoreImages())
    {
        ci->modifiedImage();
    }
}

QPointF SequenceInteractor::getFieldPosition(int field)
{
    if (field < 0) field = _field;
    QString x = QString("f%1s%2t%3c%4%5").arg(field).arg(_zpos).arg(_timepoint).arg(1).arg("X");
    QString y = QString("f%1s%2t%3c%4%5").arg(field).arg(_zpos).arg(_timepoint).arg(1).arg("Y");

    return QPoint(_mdl->property(x).toDouble(), _mdl->property(y).toDouble());
}

ImageInfos *SequenceInteractor::getChannelImageInfos(unsigned channel)
{
    //  if (!_mdl->hasChannel(_timepoint, _field, _zpos, channel))
    //    return getChannelImageInfos(channel+1);

    QString nm = _mdl->getFile(_timepoint, _field, _zpos, channel);

    //  qDebug() << "Channel Image infos" << nm;
    //if (!_infos.contains(nm))
    //    _infos[nm] =

    return imageInfos(nm, channel, loadkey);

    //return _infos[nm];
}


void SequenceInteractor::setCurrent(SequenceInteractor *i)
{
    _current = i;
    //    qDebug() << "Setting images as current: " << _mdl->pos() << _mdl->Pos();
    if (_mdl && _mdl->getOwner())
        _mdl->getOwner()->setCurrent(_mdl->pos(), true);
}

SequenceFileModel *SequenceInteractor::getSequenceFileModel()
{
    return _mdl;
}

SequenceInteractor *SequenceInteractor::current()
{
    return _current;
}


SequenceInteractor* SequenceInteractor::_current = 0;



//QPixmap SequenceInteractor::getPixmap(float scale)
//{
////  _cachePixmap ;

//  return QPixmap::fromImage(getImage(scale));
//}

void callImage(ImageInfos* img)
{
    img->image();
}


QMutex lock_infos;

ImageInfos* SequenceInteractor::imageInfos(QString file, int channel, QString key)
{
    // FIXME: Change image infos key : use XP / Workbench / deposit group

    // qDebug() << "Get interactor for file object: " << file << channel << getExperimentName();
    /* lock_infos.lock();
    ImageInfos* info = _infos[file];
    lock_infos.unlock();

    if (!info)*/
    //{ // Change behavior: Linking data at the XP level
    // To be added workbench Id + selectionModifier
    QString exp = getExperimentName();// +_mdl->Pos();
    int ii = channel < 0 ? getChannelsFromFileName(file) : channel;
    
    // getWellPos();
    //        qDebug() << "Building Image info" << file << exp << ii;
    bool exists = false;
    ImageInfos* info = ImageInfos::getInstance(this, file, exp + QString("%1").arg(ii), ii, exists, key);
    if (!exists)
    {

        if (_mdl->getOwner()->hasProperty("ChannelsColor" + QString("%1").arg(ii)))
        {
            QColor col;
            QString cname = _mdl->getOwner()->property("ChannelsColor" + QString("%1").arg(ii));
            col.setNamedColor(cname);
            info->setColor(col, false);
        }
        else
            info->setDefaultColor(ii);

        // Also setup the channel names if needed
        if (!_mdl->getChannelNames().isEmpty())
        {
            QString name = _mdl->getChannelNames()[ii-1];
            info->setChannelName(name);
        }
    }
    /*    lock_infos.lock();ta
        _infos[file] = info;
        lock_infos.unlock();*/
    //}

    return info;
}



double mse(cv::Mat i1, cv::Mat i2)
{
    double ms = 0;

    int rows  = i1.rows, cols = i1.cols;
    assert(i1.rows == i2.rows && i1.cols == i2.cols);

    for (int i = 0; i < rows; ++i)
    {
        unsigned short* l = i1.ptr<unsigned short>(i);
        unsigned short* r = i2.ptr<unsigned short>(i);

        for (int j = 0; j < cols; ++j, ++l,++r)
        {
            double d = (*l - *r);
            ms += d*d;
        }

    }

    return ms / (cols*rows);
}

QPoint refineLeft(cv::Mat& left, cv::Mat& right)
{
    QPoint res;
    double mii = std::numeric_limits<double>::max();
    int overlap = 100;

    //    int rw = left.rows, cl = left.cols;
    for (int r = 0; r < overlap; ++r)
    {
        for (int c = 1; c < overlap; ++c)
        {
            cv::Rect2d le( left.cols-c, 0, c, left.rows-r);
            cv::Rect2d ri(0,r,  c, left.rows-r);
            // FIXME : Should square measure here !
            double s = mse(left(le), right(ri));
            if (s < mii)
            {
                mii  = s;
                res = QPoint(c,r);
            }
            le = cv::Rect2d(left.cols - c, r, c, left.rows - r);
            ri = cv::Rect2d(0, 0, c, left.rows - r);
            s = mse(left(le), right(ri));
            if (s < mii)
            {
                mii = s;
                res = QPoint(c, -r);
            }
        }
    }

    return res;
}

QPoint refineLower(cv::Mat& up, cv::Mat& down)
{

    QPoint res;
    return res;
}


void SequenceInteractor::refinePacking()
{
    // Parellelize stuffs !


    // Assume single channel is enough !

    // Need to compute the unpacking starting from upper left hand corner
    //for (unsigned i = 0; i < _mdl->getFieldCount(); ++i)
    QPoint offset;
    cv::Mat ref;
    
    this->pixOffset.resize(_mdl->getFieldCount());

    for (auto it = toField.begin(), end = toField.end(); it != end; ++it)
    {
        for (auto sit = it.value().begin(), send = it.value().end(); sit != send; ++sit)
        {
            if (ref.empty())
            {
                QString file = _mdl->getFile(_timepoint, sit.value(), _zpos, 1);
                ref = imageInfos(file, -1, loadkey)->image();
                continue;
            }
            QString file = _mdl->getFile(_timepoint, sit.value(), _zpos, 1);
            cv::Mat right = imageInfos(file, -1, loadkey)->image();

            QPoint of;
            if (sit == it.value().begin())
            {
                of = refineLower(ref, right);
            }
            else
            {
                of = refineLeft(ref, right);
            }
            qDebug() << "Refine Unpack: " << it.key() << sit.key() << sit.value() << of;
            this->pixOffset[sit.value()-1] = of;// combine previous with current
            offset += of;
            ref = right;
        }
        QString file = _mdl->getFile(_timepoint, it.value().begin().value(), _zpos, 1);
        offset = pixOffset[it.value().begin().value()-1];
        ref = imageInfos(file, -1, loadkey)->image();
    }


}

void SequenceInteractor::preloadImage()
{

    //QString exp = getExperimentName();

    //  t.start();
    QStringList list = getAllChannel();
    for (QStringList::iterator it = list.begin(), e = list.end(); it != e; ++it)
    {
        imageInfos(*it, -1, loadkey)->image();
    }

}


QPoint getMatrixSize(SequenceFileModel* seq, unsigned fieldc,  int z, int t, int c)
{

    QSet<double> x,y;

    for (unsigned field = 1; field < fieldc; ++ field)
    {
        QString k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("X");
        x.insert(seq->property(k).toDouble());
        k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("Y");
        y.insert(seq->property(k).toDouble());

    }

    return QPoint(x.size(), y.size());
}


QPair<QList<double>, QList<double> > getWellPos(SequenceFileModel* seq, unsigned fieldc,  int z, int t, int c)
{

    QSet<double> x,y;

    for (unsigned field = 1; field < fieldc; ++ field)
    {
        QString k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("X");
        x.insert(seq->property(k).toDouble());
        k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("Y");
        y.insert(seq->property(k).toDouble());

    }
    QList<double> xl(x.begin(), x.end()), yl(y.begin(), y.end());
    std::sort(xl.begin(), xl.end());
    std::sort(yl.begin(), yl.end());
    //    qDebug() <<  xl << yl;
    //    xl.indexOf(), yl.indexOf()
    return qMakePair(xl,yl);
}

QPointF getFieldPos(SequenceFileModel* seq, int field, int z, int t, int c)
{
    QString k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("X");
    double x = seq->property(k).toDouble();
    k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("Y");
    double y = seq->property(k).toDouble();

    return QPointF(x,y);
}



struct StitchStruct
{
    StitchStruct(SequenceInteractor* seq, bool bias_cor, float scale): _sc(scale),  bias_correction(bias_cor), seq(seq)
    {}

    typedef QPair<int, QImage> result_type;

    QPair<int, QImage> operator()(int field)
    {
        return  qMakePair(field, seq->getPixmapChannels(field, bias_correction, _sc));
    }

    float _sc;
    bool bias_correction;
    SequenceInteractor* seq;

};

// Warning: Critical function for speed
// ENHANCE: Modify the handling of scale information

QPixmap SequenceInteractor::getPixmap(bool packed, bool bias_correction, float scale)
{
    //    qDebug() << "getPixmap" << packed;
    Q_UNUSED(scale);
    if (packed)
    {
        QImage toPix = getPixmapChannels(_field, bias_correction);
        _cachePixmap = QPixmap::fromImage(toPix);
        return _cachePixmap;
    }
    else // Unpack the data !!
    {
        QSettings set;
        float scale = set.value("unpackScaling", 1.0).toDouble();


        QPair<QList<double>, QList<double> > li =
                getWellPos(_mdl, _mdl->getFieldCount(), _zpos, _timepoint, _channel);

        QList<int> perf; for (unsigned i = 0; i < _mdl->getFieldCount(); ++i) perf.append( i+1);
        QList<QPair<int, QImage> > toStitch = QtConcurrent::blockingMapped(perf, StitchStruct(this, bias_correction, scale));


        const int rows = li.first.size() * toStitch[0].second.width();
        const int cols = li.second.size() * toStitch[0].second.height();
        QImage toPix(rows, cols, QImage::Format_RGBA8888);

        toPix.fill(Qt::black);
        for (unsigned i = 0; i < _mdl->getFieldCount(); ++i)
        {

            QPointF p = getFieldPos(_mdl, toStitch[i].first, _zpos, _timepoint, _channel);

            int x = li.first.indexOf(p.x());
            int y = li.second.size() - li.second.indexOf(p.y()) - 1;
            toField[x][y] = i+1;

            QPainter pa(&toPix);
            QPoint offset = QPoint(x*toStitch[0].second.width(), y * toStitch[0].second.height());
            if (pixOffset.size()  > 0)
                offset += pixOffset[i];

            pa.drawImage(offset, toStitch[i].second);
        }

        _cachePixmap = QPixmap::fromImage(toPix);
        return _cachePixmap;
    }
}

struct Loader
{
    Loader(float scale, bool changed_scale): _scale(scale), ch_sc(changed_scale)
    {}
    float _scale;
    bool ch_sc;
    typedef cv::Mat result_type;

    cv::Mat operator()(ImageInfos* img)
    {
        return img->image(_scale, ch_sc);
    }
};

void paletizeImage(ImageInfos* imifo, cv::Mat& image, QImage& toPix, int rows, int cols, int& lastPal)
{
    QVector<QColor> pa = imifo->getPalette();
    QVector<int> state = imifo->getState();
    int ncolors = imifo->nbColors() ;

    if (state.size() < 16) state.resize(16);
    //            QRgb black = QColor(0,0,0).rgb();
    for (int i = 0; i < rows; ++i)
    {
        unsigned short *p = image.ptr<unsigned short>(i);
        QRgb *pix = (QRgb*)toPix.scanLine(i);
        for (int j = 0; j < cols; ++j, ++p)
        {
            const unsigned short v = *p;
            if (v != 0 && state[v]) // FIXME Need to fuse the image data and not clear it....
                pix[j] = qRgb(std::min(255, qRed  (pix[j]) + pa[(v+lastPal)%16].red()),
                        std::min(255, qGreen(pix[j]) + pa[(v+lastPal)%16].green()),
                        std::min(255, qBlue (pix[j]) + pa[(v+lastPal)%16].blue()));
            else pix[j] = qRgb(std::min(255, qRed  (pix[j]) + 0),
                               std::min(255, qGreen(pix[j]) + 0),
                               std::min(255, qBlue (pix[j]) + 0));
        }
    }
    lastPal += ncolors;
}
template <bool Saturate, bool Inverted>
void colorizeImageUnbias(ImageInfos* imifo, cv::Mat& image,  cv::Mat& bias, QImage& toPix, int rows, int cols)
{
    const float mi = imifo->getDispMin(),
            ma = imifo->getDispMax();
    const int R = imifo->Red();
    const int G = imifo->Green();
    const int B = imifo->Blue();

    float mami = ma - mi;
    for (int i = 0; i < rows; ++i)
    {
        unsigned short* p = image.ptr<unsigned short>(i);
        unsigned short* b = bias.ptr<unsigned short>(i);

        QRgb* pix = (QRgb*)toPix.scanLine(i);
        for (int j = 0; j < cols; ++j, ++p,++b)
        {
            unsigned short v = *p  / (*b/10000.);
            if (!Saturate)
                v = v > ma ? mi : v;
            float f = std::min(1.f, std::max(0.f, (v - mi) / (mami)));
            if (Inverted) f = 1 - f;

            pix[j] = qRgb(std::min(255.f, qRed(pix[j]) + f * B),
                          std::min(255.f, qGreen(pix[j]) + f * G),
                          std::min(255.f, qBlue(pix[j]) + f * R));
        }
    }
}

template <bool Saturate, bool Inverted>
void colorizeImage(ImageInfos* imifo, cv::Mat& image, QImage& toPix, int rows, int cols)
{
    const float mi = imifo->getDispMin(),
            ma = imifo->getDispMax();
    const int R = imifo->Red();
    const int G = imifo->Green();
    const int B = imifo->Blue();

    float mami = ma - mi;
    for (int i = 0; i < rows; ++i)
    {
        unsigned short* p = image.ptr<unsigned short>(i);

        QRgb* pix = (QRgb*)toPix.scanLine(i);
        for (int j = 0; j < cols; ++j, ++p)
        {
            unsigned short v = *p  ;
            if (!Saturate)
                v = v > ma ? mi : v;
            float f = std::min(1.f, std::max(0.f, (v - mi) / (mami)));
            if (Inverted) f = 1 - f;

            pix[j] = qRgb(std::min(255.f, qRed(pix[j]) + f * B),
                          std::min(255.f, qGreen(pix[j]) + f * G),
                          std::min(255.f, qBlue(pix[j]) + f * R));
        }
    }
}

#include <colormap/color.hpp>
#include <colormap/grid.hpp>
#include <colormap/palettes.hpp>

template <bool Saturate, bool Inverted>
void colorMapImage(ImageInfos* imifo, cv::Mat& image, QImage& toPix, int rows, int cols)
{

    // Should get the colormap
    const float mi = imifo->getDispMin(),
            ma = imifo->getDispMax();
    using namespace colormap ;
    QByteArray tmp = imifo->colormap().toLocal8Bit();
    const char* palette = tmp.constData();
    // get a colormap and rescale it
    auto pal = palettes.at(palette);
    //    pal.rescale(mi, ma); // We are using already scaled data!!
    pal.rescale(0, 1);//
    float mami = ma - mi;
    for (int i = 0; i < rows; ++i)
    {
        unsigned short* p = image.ptr<unsigned short>(i);

        QRgb* pix = (QRgb*)toPix.scanLine(i);
        for (int j = 0; j < cols; ++j, ++p)
        {
            unsigned short v = *p  ;
            if (!Saturate)
                v = v > ma ? mi : v;
            float f = std::min(1.f, std::max(0.f, (v - mi) / (mami)));
            if (Inverted) f = 1 - f;

            auto colo = pal(f);
            pix[j] = qRgb(std::min(255.f, qRed(pix[j]) + (float)colo[0]),
                    std::min(255.f, qGreen(pix[j])+ (float)colo[1]),
                    std::min(255.f, qBlue(pix[j]) + (float)colo[2]));
        }
    }
}


QImage SequenceInteractor::getPixmapChannels(int field, bool bias_correction, float scale)
{
    QStringList list = getAllChannel(field);

    QList<ImageInfos*> img;
    QList<cv::Mat> images;//(list.size());
    int ii = 0;
    for (QStringList::iterator it = list.begin(), e = list.end(); it != e; ++it,++ii)
    {
        img.append(imageInfos(*it,ii+1, loadkey));
    }


    images = QtConcurrent::blockingMapped(img, Loader(scale, last_scale != scale));


    last_scale = scale;


    //  qDebug() << t.elapsed() << "ms";
    //  qDebug() << "Got image list";
    const int rows = images[0].rows;
    const int cols = images[0].cols;
    //  qDebug() << rows << cols ;

    QImage toPix(cols, rows, QImage::Format_RGBA8888);

    toPix.fill(Qt::black);

    int lastPal = 0;
    for (int c = 0; c < list.size(); ++c)
    {
        if (images[c].empty()) continue;

        int ncolors = img[c]->nbColors() ;
        bool saturate = img[c]->isSaturated();
        bool inverted = img[c]->isInverted();

        if (!img[c]->active()) { if (ncolors < 16) lastPal += ncolors; continue; }

        if (img[c]->getMin() >= 0 && ncolors < 16)
        {
            paletizeImage(img[c], images[c], toPix, rows, cols, lastPal);
        }
        else
        {

            if (img[c]->colormap().isEmpty())
            {

                const float mi = img[c]->getDispMin(),
                        ma = img[c]->getDispMax();
                const int R = img[c]->Red();
                const int G = img[c]->Green();
                const int B = img[c]->Blue();

                float mami = ma - mi;
                if (bias_correction)
                {
                    cv::Mat bias = img[c]->bias(c+1);

                    if (saturate && inverted )
                        colorizeImageUnbias<true, true>(img[c], images[c], bias, toPix, rows, cols);
                    if (saturate && !inverted )
                        colorizeImageUnbias<true, false>(img[c], images[c], bias, toPix, rows, cols);
                    if (!saturate && inverted )
                        colorizeImageUnbias<false, true>(img[c], images[c], bias, toPix, rows, cols);
                    if (!saturate && !inverted )
                        colorizeImageUnbias<false, false>(img[c], images[c], bias, toPix, rows, cols);

                }
                else
                {
                    if (saturate && inverted )
                        colorizeImage<true, true>(img[c], images[c], toPix, rows, cols);
                    if (saturate && !inverted )
                        colorizeImage<true, false>(img[c], images[c], toPix, rows, cols);
                    if (!saturate && inverted )
                        colorizeImage<false, true>(img[c], images[c], toPix, rows, cols);
                    if (!saturate && !inverted )
                        colorizeImage<false, false>(img[c], images[c], toPix, rows, cols);

                }
            } else {
                if (saturate && inverted )
                    colorMapImage<true, true>(img[c], images[c], toPix, rows, cols);
                if (saturate && !inverted )
                    colorMapImage<true, false>(img[c], images[c], toPix, rows, cols);
                if (!saturate && inverted )
                    colorMapImage<false, true>(img[c], images[c], toPix, rows, cols);
                if (!saturate && !inverted )
                    colorMapImage<false, false>(img[c], images[c], toPix, rows, cols);

            }

        }
    }
    return toPix;
}


QList<unsigned> SequenceInteractor::getData(QPointF d, int& field,  bool packed, bool bias)
{
    QList<unsigned> res;
    if (packed)
    {

        QStringList list = getAllChannel();
        field = _field;

        int ii = 0;

        for (QStringList::iterator it = list.begin(), e = list.end(); it != e; ++it, ++ii)
        {
            cv::Mat m = imageInfos(*it, -1, loadkey)->image();
            if (m.rows > d.y() && m.cols > d.x())
                res << m.at<unsigned short>((int)d.y(), (int)d.x());
        }
    }
    else
    {
        // Find out in what cadran we are in

        cv::Mat m = imageInfos( getFile(), -1, loadkey)->image();
        int cx = floor(d.x() / m.cols);
        int cy = floor(d.y() / m.rows);


        d.setX(d.x() - cx * m.cols);
        d.setY(d.y() - cy * m.rows);

        int f = this->toField[cx][cy];
        field = f;

        int ii = 0;
        QStringList list = getAllChannel(f);

        for (QStringList::iterator it = list.begin(), e = list.end(); it != e; ++it, ++ii)
        {
            cv::Mat m = imageInfos(*it, -1, loadkey)->image();
            if (m.rows > d.y() && m.cols > d.x())
                res << m.at<unsigned short>((int)d.y(), (int)d.x());
        }

    }
    return res;
}
