#include "sequenceinteractor.h"
#include <tuple>

#include <QImage>

#include "Gui/ctkWidgets/ctkDoubleRangeSlider.h"

#include <QtConcurrent/QtConcurrent>
#include <QMutex>
#include <QPainter>
#include <QElapsedTimer>


QMutex sequence_interactorMutex(QMutex::NonRecursive);

SequenceInteractor::SequenceInteractor():
    _mdl(0),
    _timepoint(1), _field(1), _zpos(1), _channel(1), _fps(25.),
    disp_tile(false), tile_id(0),
    last_scale(-1.), _updating(false)
{
}

SequenceInteractor::SequenceInteractor(SequenceFileModel *mdl, QString key):
    _mdl(mdl), _timepoint(1), _field(1), _zpos(1), _channel(1),
    _fps(25.), disp_tile(false), tile_id(0), loadkey(key), last_scale(-1.), _updating(false)
{
}

void SequenceInteractor::setTimePoint(unsigned t)
{
    if (_mdl->getTimePointCount() >= t)
        _timepoint = t;
}


void SequenceInteractor::setField(unsigned t)
{
    _updating = true;

    if (_mdl->getFieldCount() >= t)
        _field = t;

    QString nm = _mdl->getFile(_timepoint, _field, _zpos, _channel);
    QSettings set;

    if (set.value("SyncFields", true).toBool())
    {
        ImageInfos* ifo = imageInfos(nm, _channel, loadkey);

        if (ifo)
        {
            QList<ImageInfos*> list = ifo->getLinkedImagesInfos();
            foreach(ImageInfos* info, list)
            {
                SequenceInteractor* inter = info->getInteractor();

                if (inter && !inter->isUpdating())
                {
                    inter->setField(t);
                    inter->modifiedImage();
                }
            }
        }
    }
    _updating = false;

}

void SequenceInteractor::setZ(unsigned z)
{
    _updating = true;
    if (_mdl->getZCount() >= z)
        _zpos = z;

    QString nm = _mdl->getFile(_timepoint, _field, _zpos, _channel);

    QSettings set;

    if (set.value("SyncField", true).toBool())
    {
        ImageInfos* ifo = imageInfos(nm, _channel, loadkey);
        if (ifo)
        {
            QList<ImageInfos*> list = ifo->getLinkedImagesInfos();
            foreach(ImageInfos* info, list)
            {
                SequenceInteractor* inter = info->getInteractor();
                if (inter && !inter->isUpdating())
                {
                    inter->setZ(z);
                    inter->modifiedImage();
                }
            }
        }
    }
    _updating = false;
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

void SequenceInteractor::setTile(int tile)
{
    qDebug() << "Changing Tile" << tile;
    tile_id = tile;
    if (disp_tile) // only update if disp is on!
        modifiedImage();
}

void SequenceInteractor::displayTile(bool disp)
{
    qDebug() << "Toggling Tile disp:" << disp;
    disp_tile = disp;
    modifiedImage();
}

bool SequenceInteractor::tileDisplayed()
{
    return disp_tile;
}

int SequenceInteractor::getTile()
{
    return tile_id;
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


QList<QPair<int, QString> > SequenceInteractor::getAllChannel(int field)
{
    QList<QPair<int, QString> > l;

    if (field <= 0) field = _field;
    SequenceFileModel::Channel& chans = _mdl->getChannelsFiles(_timepoint, field, _zpos);

    for (SequenceFileModel::Channel::const_iterator it = chans.cbegin(), e = chans.cend();
         it != e; ++it)
        if (!it.value().isEmpty())   l <<  qMakePair(it.key(), it.value());

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


QMutex lock_infos(QMutex::NonRecursive);

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
        qDebug() << "Set with color"<< exp << ii << file << info->getColor().name();
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

std::tuple< double, QPoint> refineLeft(cv::Mat& left, cv::Mat& right, int overlap, bool first = true)
{
    QPoint res;
    double mii = std::numeric_limits<double>::max();

    //    int rw = left.rows, cl = left.cols;
    for (int r = 0; r < overlap; ++r)
    {
        for (int c = 1; c < overlap; ++c)
        {
            if (first)
            {
                cv::Rect2d le( left.cols-c, 0, c, left.rows-r);
                cv::Rect2d ri(0,r,  c, left.rows-r);

                double s = mse(left(le), right(ri));
                if (s < mii)
                {
                    mii  = s;
                    res = QPoint(-c,-r);
                }
            }
            else
            {
                cv::Rect2d le = cv::Rect2d(left.cols - c, r, c, left.rows - r);
                cv::Rect2d ri = cv::Rect2d(0, 0, c, left.rows - r);
                double s = mse(left(le), right(ri));
                if (s < mii)
                {
                    mii = s;
                    res = QPoint(-c, r);
                }
            }
        }
    }
    return std::make_tuple(mii, res);
}

std::tuple<double, QPoint> refineLower(cv::Mat& up, cv::Mat& down, int overlap, bool first = true)
{
    QPoint res;

    double mii = std::numeric_limits<double>::max();
    
    for (int r = 1; r < overlap; ++r)
    {
        for (int c = 0; c < overlap; ++c)
        {
            if (first)
            {
                cv::Rect2d u(0, up.rows - r, up.cols - c, r);
                cv::Rect2d l(c, 0, down.cols - c, r);
                double s = mse(up(u), down(l));
                if (s < mii)
                {
                    mii = s;
                    res = QPoint(c, -r);
                }
            }
            else
            {
                cv::Rect2d u = cv::Rect2d(c, up.rows - r,  up.cols - c, r);
                cv::Rect2d l = cv::Rect2d(0, 0, down.cols - c, r);
                double s = mse(up(u), down(l));;
                if (s < mii)
                {
                    mii = s;
                    res = QPoint(-c, -r);
                }
            }
        }
    }

    return std::make_tuple(mii, res);
}


void SequenceInteractor::refinePacking()
{
    // Assume single channel is enough !

    // Need to compute the unpacking starting from upper left hand corner
    //for (unsigned i = 0; i < _mdl->getFieldCount(); ++i)
    cv::Mat ref;

    QList<std::tuple<int, cv::Mat, cv::Mat, bool /*left/up*/,  bool/*1st/2nd step*/> > data_mapping;
    
    this->pixOffset.resize(_mdl->getFieldCount());


    auto toField = _mdl->getOwner()->getFieldPosition();

    for (auto it = toField.begin(), end = toField.end(); it != end; ++it)
    {
        for (auto sit = it.value().begin(), send = it.value().end(); sit != send; ++sit)
        {
            qDebug() << "Field " << sit.value() << " is at position" << it.key() << sit.key();
            if (ref.empty())
            {
                QString file = _mdl->getFile(_timepoint, sit.value(), _zpos, 1);
                ref = imageInfos(file, -1, loadkey)->image();
                continue;
            }
            QString file = _mdl->getFile(_timepoint, sit.value(), _zpos, 1);
            cv::Mat right = imageInfos(file, -1, loadkey)->image();

            std::tuple<int, cv::Mat, cv::Mat, bool /*left/up*/,  bool/*1st/2nd step*/> tuple;

            tuple = std::make_tuple(sit.value()-1, ref, right, sit == it.value().begin(), true);
            data_mapping.push_back(tuple);
            tuple = std::make_tuple(sit.value()-1, ref, right, sit == it.value().begin(), false);
            data_mapping.push_back(tuple);

        }
        QString file = _mdl->getFile(_timepoint, it.value().begin().value(), _zpos, 1);
        ref = imageInfos(file, -1, loadkey)->image();
    }

    // Now i'd like to use the QtConcurrent Map function !!!
    struct Mapping{

        typedef std::tuple<int, double, QPoint> result_type;

        std::tuple<int, double, QPoint> operator()(std::tuple<int, cv::Mat, cv::Mat, bool /*left/up*/,  bool/*1st/2nd step*/> data)
        {
            int xp = std::get<0>(data);

            cv::Mat a = std::get<1>(data);
            cv::Mat b = std::get<2>(data);

            bool left = std::get<3>(data);
            bool first = std::get<4>(data);

            std::tuple<double, QPoint> res;
            if (left)
                res = refineLeft(a,b, 200, first);
            else
                res = refineLower(a,b, 200, first);

            qDebug() << xp << (int)first << (left ? "RefineLeft" : "RefineLower") << std::get<0>(res) << std::get<1>(res);
            return std::make_tuple(xp, std::get<0>(res), std::get<1>(res));
        }

    };

    QList< std::tuple<int, double, QPoint> > res =
            QtConcurrent::blockingMapped(data_mapping, Mapping()); //apply the mapping


    QMap<int, QPair<double, QPoint> > proj;
    for (auto v : res)
    {
        if (proj.contains(std::get<0>(v)))
        {
            QPair<double, QPoint> t = proj[std::get<0>(v)];
            if (t.first > std::get<1>(v))
                proj[std::get<0>(v)] = qMakePair(std::get<1>(v), std::get<2>(v));
        }
        else
        {
            proj[std::get<0>(v)] = qMakePair(std::get<1>(v), std::get<2>(v));
        }
    }
    qDebug() << proj;
    bool start = true;
    QPoint offset;
    for (int r = 0; r < toField.size(); ++r)
    {
        for (int c = 0; c < toField[r].size(); ++c)
        {
            if (start) { start = false; continue;  }
            int field = toField[r][c] - 1;
            QPoint of = proj[field].second;
            if (r == 0) offset = pixOffset[toField[r][c - 1]-1];
            if (c > 0) offset = pixOffset[toField[r][c]-1];
            this->pixOffset[field] = offset + of;
        }
    }

}

void SequenceInteractor::preloadImage()
{

    //QString exp = getExperimentName();

    //  t.start();
    QList<QPair<int, QString> > list = getAllChannel();
    for (QList<QPair<int, QString> >::iterator it = list.begin(), e = list.end(); it != e; ++it)
    {
        imageInfos(it->second, it->first, loadkey)->image();
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


        QPair<QList<double>, QList<double> > li = _mdl->getOwner()->getFieldSpatialPositions();
        QPair<QList<bool>, QList<bool> > filt;

        for (int p = 0; p < li.first.size(); ++p)
            filt.first << false;
        for (int p = 0; p < li.second.size(); ++p)
            filt.second << false;




        QList<int> perf; for (unsigned i = 0; i < _mdl->getFieldCount(); ++i) perf.append( i+1);
        QList<QPair<int, QImage> > toStitch = QtConcurrent::blockingMapped(perf, StitchStruct(this, bias_correction, scale));

        for (unsigned i = 0; i < _mdl->getFieldCount(); ++i)
        {
            QPointF p = getFieldPos(_mdl, toStitch[i].first, _zpos, _timepoint, _channel);


            int x = li.first.indexOf(p.x());
            filt.first[x] = true;
            int y =li.second.indexOf(p.y());
            filt.second[y] = true;
        }

        QList<double> a, b;

        for (unsigned i =0; i < (unsigned)li.first.size(); ++i)
            if (filt.first[i])
                a << li.first[i];

        for (unsigned i =0; i < (unsigned)li.second.size(); ++i)
            if (filt.second[i])
                b << li.second[i];

        li.first = a;
        li.second = b;

        const int rows = li.first.size() * toStitch[0].second.width();
        const int cols = li.second.size() * toStitch[0].second.height();
        QImage toPix(rows, cols, QImage::Format_RGBA8888);

        toPix.fill(Qt::black);
        for (unsigned i = 0; i < _mdl->getFieldCount(); ++i)
        {

            QPointF p = getFieldPos(_mdl, toStitch[i].first, _zpos, _timepoint, _channel);

            int x = li.first.indexOf(p.x());
            int y = li.second.size() - li.second.indexOf(p.y()) - 1;

            QPainter pa(&toPix);
            QPoint offset = QPoint(x*toStitch[0].second.width(), y * toStitch[0].second.height());
            qDebug() << "Field" << i << toStitch[i].first << "X Y:" << x << y << "Offset:" << offset;
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
void colorizeImage(ImageInfos* imifo, cv::Mat& image, QImage& toPix, int rows, int cols, bool binarize)
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
            if (binarize)
                v = v > mi ? ma : mi;
            float f = std::min(1.f, std::max(0.f, (v - mi) / (mami)));
            if (Inverted) f = 1 - f;

            pix[j] = qRgb(std::min(255.f, qRed(pix[j]) + f * B),
                          std::min(255.f, qGreen(pix[j]) + f * G),
                          std::min(255.f, qBlue(pix[j]) + f * R));
        }
    }
}

template <bool Saturate, bool Inverted>
void colorizeImageUnbias(ImageInfos* imifo, cv::Mat& image,  cv::Mat& bias, QImage& toPix, int rows, int cols, bool binarize)
{
    if (bias.empty())
        colorizeImage<Saturate, Inverted>(imifo, image, toPix, rows, cols, binarize);

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
            if (binarize)
                v = v > mi ? ma : mi;

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
#include <random>


void randomMapImage(ImageInfos* imifo, cv::Mat& image, QImage& toPix, int rows, int cols)
{
    Q_UNUSED(imifo);

    for (int i = 0; i < rows; ++i)
    {
        unsigned short* p = image.ptr<unsigned short>(i);

        QRgb* pix = (QRgb*)toPix.scanLine(i);
        for (int j = 0; j < cols; ++j, ++p)
        {
            unsigned short v = *p  ;
            std::minstd_rand0 nb(v);
            int r = nb(), g = nb(), b = nb(); // get a random value

            pix[j] = qRgb(std::min(255.f, qRed(pix[j]) + (float)(r&0xFF)),
                          std::min(255.f, qGreen(pix[j])+ (float)(g&0xFF)),
                          std::min(255.f, qBlue(pix[j]) + (float)(b&0xFF)));
        }
    }
}

template <bool Saturate, bool Inverted>
void colorMapImage(ImageInfos* imifo, cv::Mat& image, QImage& toPix, int rows, int cols)
{

    // Should get the colormap
    const float mi = imifo->getDispMin(),
            ma = imifo->getDispMax();
    using namespace colormap ;

    if (imifo->colormap() == "random")
    {
        randomMapImage(imifo, image, toPix, rows, cols);
        return;
    }
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
/* #region Main */

QImage SequenceInteractor::getPixmapChannels(int field, bool bias_correction, float scale)
{
    QList<QPair<int, QString> > list = getAllChannel(field);

    QList<ImageInfos*> img;
    QList<cv::Mat> images;//(list.size());
    int ii = 0;
    for (QList<QPair<int, QString> >::iterator it = list.begin(), e = list.end(); it != e; ++it,++ii)
    {
        img.append(imageInfos(it->second, it->first, loadkey)); // Auto channel determination
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
        bool binarize = img[c]->isBinarized();

        if (!img[c]->active()) { if (ncolors < 16) lastPal += ncolors; continue; }

        if (img[c]->getMin() >= 0 && ncolors < 16)
        {
            paletizeImage(img[c], images[c], toPix, rows, cols, lastPal);
        }
        else
        {

            if (img[c]->colormap().isEmpty())
            {
                if (bias_correction)
                {
                    cv::Mat bias = img[c]->bias(c+1);

                    if (saturate && inverted )
                        colorizeImageUnbias<true, true>(img[c], images[c], bias, toPix, rows, cols, binarize);
                    if (saturate && !inverted )
                        colorizeImageUnbias<true, false>(img[c], images[c], bias, toPix, rows, cols, binarize);
                    if (!saturate && inverted )
                        colorizeImageUnbias<false, true>(img[c], images[c], bias, toPix, rows, cols, binarize);
                    if (!saturate && !inverted )
                        colorizeImageUnbias<false, false>(img[c], images[c], bias, toPix, rows, cols, binarize);

                }
                else
                {
                    if (saturate && inverted )
                        colorizeImage<true, true>(img[c], images[c], toPix, rows, cols, binarize);
                    if (saturate && !inverted )
                        colorizeImage<true, false>(img[c], images[c], toPix, rows, cols, binarize);
                    if (!saturate && inverted )
                        colorizeImage<false, true>(img[c], images[c], toPix, rows, cols, binarize);
                    if (!saturate && !inverted )
                        colorizeImage<false, false>(img[c], images[c], toPix, rows, cols, binarize);

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

/* endregion */

QList<unsigned> SequenceInteractor::getData(QPointF d, int& field,  bool packed, bool bias)
{
    Q_UNUSED(bias);

    QList<unsigned> res;
    if (packed)
    {

        QList<QPair<int, QString> > list = getAllChannel();
        field = _field;

        int ii = 0;

        for (QList<QPair<int, QString> >::iterator it = list.begin(), e = list.end(); it != e; ++it, ++ii)
        {
            cv::Mat m = imageInfos(it->second, it->first, loadkey)->image();
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


        int f = _mdl->getOwner()->getFieldPosition()[cx][cy];
        field = f;

        int ii = 0;
        QList<QPair<int, QString> > list = getAllChannel(f);

        for (QList<QPair<int, QString> >::iterator it = list.begin(), e = list.end(); it != e; ++it, ++ii)
        {
            cv::Mat m = imageInfos(it->second, it->first, loadkey)->image();
            if (m.rows > d.y() && m.cols > d.x())
                res << m.at<unsigned short>((int)d.y(), (int)d.x());
        }

    }
    return res;
}


int findY(QStringList list, QString x)
{
    x = x.replace("_X", "_Y");
    for (int i = 0; i < list.size(); ++i)
    {
        if (x == list[i])
            return i;
    }
    return 0;
}


QColor randCol(float v)
{
    std::minstd_rand0 nb(v);
    int r = nb(), g = nb(), b = nb(); // get a random value
    return qRgb((float)(r&0xFF),(float)(g&0xFF),(float)(b&0xFF));
}


void getMinMax(cv::Mat &ob, int f, float& cmin, float& cmax)
{
    cmin = std::numeric_limits<float>::max();
    cmax = -std::numeric_limits<float>::max();

    for (int r = 0; r < ob.rows; ++r)
    {
        float v = ob.at<float>(r,f);
        if (v < cmin) cmin = v;
        else if (v > cmax) cmax = v;
    }

}


QList<QGraphicsItem *> SequenceInteractor::getMeta(QGraphicsItem *parent)
{
    // The interactor will filter / scale & analyse the meta data to be generated
    QList<QGraphicsItem*> res;

    qDebug() << "Get Meta Called, " << disp_tile << tile_id;
    if (disp_tile)
    { // Specific code to display tile with respect to position !!!
        QSettings set;

        int rx = set.value("TileSizeX", 256).toInt() / 2,
                ry = set.value("TileSizeX", 216).toInt() / 2;


        auto item = new QGraphicsRectItem(parent);

        float x = rx * rint(tile_id % 19);
        float l = (tile_id-(tile_id % 19));
        float y = ry * rint(l / 19);

        float  width = rx,
                height= ry;


        item->setRect(QRectF(x,y,width,height));        
        item->setPen(QPen(Qt::yellow));
        qDebug() << item->rect();
        res << item;
    }


    //    SequenceFileModel::Channel& chans = _mdl->getChannelsFiles(_timepoint, _field, _zpos);

    int cns = _mdl->getMetaChannels(_timepoint, _field, _zpos);
    qDebug() << _timepoint << _field << _zpos << cns;
    for (int c = 1; c <= cns;++c)
    {
        QMap<QString, StructuredMetaData>& data = _mdl->getMetas(_timepoint, _field, _zpos, c);
        // We can add some selectors

        for (auto k: data)
        {
            //        StructuredMetaData& k = data.first();

            QString cols = k.property("ChannelNames");
            qDebug() << cols;

            if (cols.contains("_Top") && cols.contains("_Left")
                    && cols.contains("_Width") && cols.contains("_Height"))
            {
                // This is a Top Left Corner rectangle setting !

                // Find item pos:
                cv::Mat& feat = k.content();

                int t,l,w,h, f;
                QStringList lcols = cols.split(";").mid(0, feat.cols);
                QList<int> feats;

                for (int i = 0; i < lcols.size(); ++i)
                {
                    QString s = lcols[i];
                    if (s.contains("_Top"))   t = i;
                    else if (s.contains("_Left"))  l = i;
                    else if (s.contains("_Width")) w = i;
                    else if (s.contains("_Height")) h = i;
                    else { feats << i;  }
                }

                float cmin, cmax;
                f = feats.first();
                getMinMax(feat, f, cmin, cmax);


                using namespace colormap ;

                auto pal = palettes.at("jet");
                pal.rescale(cmin, cmax);


                auto group = new QGraphicsItemGroup(parent);
                for (int r = 0; r < feat.rows; ++r)
                {

                    auto item = new QGraphicsRectItem(group);
                    float y = feat.at<float>(r, t),
                            x = feat.at<float>(r, l),
                            width= feat.at<float>(r, w),
                            height= feat.at<float>(r, h);
                    item->setRect(QRectF(x,y,width,height));
                    //qDebug() << r << x << y << width << height;
                    QString tip;
                    for (auto p : feats)
                    {
                        float fea = feat.at<float>(r,p);
                        tip += QString("%1: %2\r\n").arg(lcols[p]).arg(fea);
                    }
                    item->setToolTip(tip.trimmed());
                    float fea = feat.at<float>(r,f);
                    auto colo = pal(fea);
                    item->setPen(QPen(qRgb(colo[0], colo[1], colo[2])));
                    //                item->setBrush(); // FIXME: Add color feature
                }
                res << group;
            }
            if (cols.contains("_X") && cols.contains("_Y"))
            {
                // There is a X & Y coordinate
                if (cols.count("_X") >= 2 && cols.count("_Y") >= 2)
                {
                    // Should be a vector
                    cv::Mat& feat = k.content();

                    int x1,y1, x2, y2;
                    QStringList lcols = cols.split(";").mid(0, feat.cols);

                    for (int i = 0; i < lcols.size(); ++i)
                    {
                        QString s = lcols[i];
                        if (s.contains("_X"))  {
                            x1 = i;
                            y1 = findY(lcols, s);
                            for (int j = i+1; j < lcols.size(); ++j)
                                if (lcols[j].contains("_X"))
                                {
                                    x2 = j;
                                    y2 = findY(lcols, lcols[j]);
                                    break;
                                }
                            break;
                        }
                    }
                    auto group = new QGraphicsItemGroup(parent);
                    for (int r = 0; r < feat.rows; ++r)
                    {

                        auto item = new QGraphicsLineItem (group);
                        float x = feat.at<float>(r, x1),
                                y = feat.at<float>(r, y1),
                                w= feat.at<float>(r, x2),
                                h= feat.at<float>(r, y2);
                        item->setLine(x,y,w,h);
                        float len = sqrt(pow(x-w,2)+pow(y-h,2));
                        //qDebug() << r << x << y << width << height;
                        item->setPen(QPen(randCol(r))); // Random coloring !
                        item->setToolTip(QString("Length %1").arg(len));
                    }
                    res << group;


                }
                else
                {
                    // This is a 2d point

                    cv::Mat& feat = k.content();

                    int t,l,  f;
                    QStringList lcols = cols.split(";").mid(0, feat.cols);

                    for (int i = 0; i < lcols.size(); ++i)
                    {
                        QString s = lcols[i];
                        if (s.contains("_X"))   t = i;
                        else if (s.contains("_Y"))  l = i;
                        else f = i;
                    }

                    float cmin, cmax;
                    getMinMax(feat, f, cmin, cmax);


                    using namespace colormap ;

                    auto pal = palettes.at("jet");
                    pal.rescale(cmin, cmax);




                    auto group = new QGraphicsItemGroup(parent);
                    for (int r = 0; r < feat.rows; ++r)
                    {
                        auto item = new QGraphicsEllipseItem(group);
                        // Radius of 1.5 px :)
                        float x = feat.at<float>(r, t-1),
                                y= feat.at<float>(r, l-1),
                                width= feat.at<float>(r, t+1),
                                height= feat.at<float>(r, l+1);

                        item->setRect(QRectF(x,y,width,height));
                        float fea = feat.at<float>(r,f);
                        item->setToolTip(QString("%1 %2").arg(lcols[f]).arg(fea));
                        auto colo = pal(fea);
                        item->setBrush(QBrush(qRgb(colo[0], colo[1], colo[2])));
                    }
                    res << group;


                }

            }
        }


    }
    return res;
}

void SequenceInteractor::getResolution(float &x, float &y)
{
    static float _x = 0, _y = 0;
    if (_x == 0)
    {
        _x = _mdl->property(QString("HorizontalPixelDimension_ch%1").arg(_channel)).toDouble();
        _y = _mdl->property(QString("VerticalPixelDimension_ch%1").arg(_channel)).toDouble();
    }
    x = _x;
    y = _y;
//    qDebug() << x << y;
    //     "HorizontalPixelDimension" << "VerticalPixelDimension"
    //_mdl->
}

