#include "sequenceinteractor.h"
#include <tuple>

#include <QImage>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "Gui/ctkWidgets/ctkDoubleRangeSlider.h"

#include <QtConcurrent/QtConcurrent>
#include <QMutex>
#include <QPainter>
#include <QElapsedTimer>


QMutex sequence_interactorMutex(QMutex::NonRecursive);
QMutex lock_infos(QMutex::NonRecursive);

SequenceInteractor::SequenceInteractor() :
    _mdl(0),
    _timepoint(1), _field(1), _zpos(1), _channel(1), _fps(25.),
    //    disp_tile(false), tile_id(0),
    last_scale(-1.), _updating(false), _changed(true), _overlay_width(1)
{
}

SequenceInteractor::SequenceInteractor(SequenceFileModel* mdl, QString key) :
    _mdl(mdl), _timepoint(1), _field(1), _zpos(1), _channel(1),
    _fps(25.), loadkey(key), last_scale(-1.), _updating(false), _changed(true), _overlay_width(1)
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
            foreach(ImageInfos * info, list)
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
            foreach(ImageInfos * info, list)
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

    foreach(CoreImage * im, _ImageList)
        im->changeFps(fps);

}

QString SequenceInteractor::getOverlayCode(QString name)
{
    if (!overlay_coding.contains(name))
        overlay_coding[name].second = "jet"; // set default overlay

    return overlay_coding[name].first;
}

QString SequenceInteractor::getOverlayColor(QString name)
{
    if (overlay_coding.contains(name))
        return overlay_coding[name].second;
    return "jet";
}

void SequenceInteractor::exportOverlay(QString name, QString tofile)
{
    if (!_mdl)
        return;
    int cns = _mdl->getMetaChannels(_timepoint, _field, _zpos);

    for (int c = 1; c <= cns; ++c)
    {
        QMap<QString, StructuredMetaData>& data = _mdl->getMetas(_timepoint, _field, _zpos, c);
        if (data.contains(name))
        {
            auto k = data[name];
            QString cols = k.property("ChannelNames");
            cols = cols.replace(';', ',');
            QFile of(tofile);
            if (of.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream str(&of);
                str << cols << "\r\n";
                cv::Mat& feat = k.content();

                for (int r = 0; r < feat.rows; ++r)
                {
                    for (int c = 0; c < feat.cols; ++c)
                    {
                        if (c != 0) str << ',';
                        str << feat.at<float>(r, c);
                    }
                    str << "\r\n";
                }
            }
        }
    }
}

void SequenceInteractor::overlayChange(QString name, QString id)
{
    //    qDebug() << "Overlay selection changed" << name << id;
    if (!overlay_coding.contains(name))
        overlay_coding[name].second = "jet";

    overlay_coding[name].first = id;
    modifiedImage();
}

void SequenceInteractor::overlayChangeCmap(QString name, QString id)
{
    overlay_coding[name].second = id;
    modifiedImage();
}


void SequenceInteractor::setOverlayId(QString name, int tile)
{
    //qDebug() << "Changing Tile" << tile;
    disp_overlay[name].second = tile;
    //    tile_id = tile;
    if (disp_overlay[name].first) // only update if disp is on!
        modifiedImage();
}

void SequenceInteractor::toggleOverlay(QString name, bool disp)
{
    //qDebug() << "Toggling Tile disp:" << disp;
    if (!disp_overlay.contains(name))
        disp_overlay[name].second = -1;

    disp_overlay[name].first = disp;

    //    disp_tile = disp;
    modifiedImage();
}

bool SequenceInteractor::overlayDisplayed(QString name)
{
    if (!disp_overlay.contains(name))
    {
        disp_overlay[name].first = false;
        disp_overlay[name].second = -1;
    }
    return disp_overlay[name].first;
}

int SequenceInteractor::getOverlayId(QString name)
{
    if (!disp_overlay.contains(name))
        disp_overlay[name].second = -1;

    return disp_overlay[name].second;
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

QSet<int> SequenceInteractor::getChannelsIds()
{
    return _mdl->getChannelsIds();
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
        if (!it.value().isEmpty())   l << qMakePair(it.key(), it.value());

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

QString SequenceInteractor::getProjectName()
{
    return  _mdl->getOwner()->getProjectName();
}

QString SequenceInteractor::getExperimentName()
{
    QStringList file = getFileName().split("/", Qt::SkipEmptyParts);
    file.pop_back();
    QString exp = file.last(); file.pop_back();
    exp += file.last();
    return exp;
}

void SequenceInteractor::addImage(CoreImage* ci)
{

    //    qDebug() << "Interactor" << this << "Adding image" << ci;

    if (!_ImageList.contains(ci))
        this->_ImageList << ci;
    //
    QString nm = _mdl->getFile(_timepoint, _field, _zpos, _channel);

    //  qDebug() << "Channel Image infos" << nm;
    //if (!_infos.contains(nm))
    //    _infos[nm] =

    ImageInfos* ifo = imageInfos(nm, _channel);

    ifo->addCoreImage(ci);
}

void SequenceInteractor::clearMemory(CoreImage* im)
{
    lock_infos.lock();
    _ImageList.removeAll(im);

    QString exp = getExperimentName();// +_mdl->Pos();

    for (unsigned ii = 1; ii <= _mdl->getChannels(); ++ii)
    {
        QString nm = _mdl->getFile(_timepoint, _field, _zpos, ii);

        bool exists = false;
        ImageInfos* info = ImageInfos::getInstance(this, nm, exp + QString("%1").arg(ii), ii, exists, loadkey);
        info->deleteInstance();
        delete info;
        _infos.remove(nm);
    }
    lock_infos.unlock();
    //    qDebug() << "FIXME: Clear Memory called for SequenceInteractor, but may not be honored";
}


void SequenceInteractor::modifiedImage()
{
   // lock_infos.lock();
    //    qDebug() << "Interactor Modified Image";
    foreach(CoreImage * ci, _ImageList)
    {
        ci->modifiedImage();
    }

    QString nm = _mdl->getFile(_timepoint, _field, _zpos, _channel);
    ImageInfos* ifo = imageInfos(nm, _channel, loadkey);
    foreach(CoreImage * ci, ifo->getCoreImages())
    {
        ci->modifiedImage();
    }
   // lock_infos.unlock();
}

QPointF SequenceInteractor::getFieldPosition(int field)
{
    if (field < 0) field = _field;

    QRegExp x(QString("f%1s.*X").arg(field));
    QRegExp y(QString("f%1s.*Y").arg(field));

    return QPoint(_mdl->property(x).toDouble(), _mdl->property(y).toDouble());
}

ImageInfos* SequenceInteractor::getChannelImageInfos(unsigned channel)
{

    QString nm = _mdl->getFile(_timepoint, _field, _zpos, channel);

    ImageInfos* res = imageInfos(nm, channel, loadkey);

    return res;
}


void SequenceInteractor::setCurrent(SequenceInteractor* i)
{
    if (i != _current)
    {
        _current = i;
        i->_changed = true;
        _changed = true;
        //    qDebug() << "Setting images as current: " << _mdl->pos() << _mdl->Pos();
        if (_mdl && _mdl->getOwner())
            _mdl->getOwner()->setCurrent(_mdl->pos(), true);
    }
}

bool SequenceInteractor::currentChanged()
{
    bool res = _changed;
    _changed = false;
    return res;
}

SequenceFileModel* SequenceInteractor::getSequenceFileModel()
{
    return _mdl;
}

SequenceInteractor* SequenceInteractor::current()
{
    return _current;
}


SequenceInteractor* SequenceInteractor::_current = 0;


void callImage(ImageInfos* img)
{
    img->image();
}


ImageInfos* SequenceInteractor::imageInfos(QString file, int channel, QString key)
{
    // FIXME: Change image infos key : use XP / Workbench / deposit group

    // qDebug() << "Get interactor for file object: " << file << channel << getExperimentName();
    lock_infos.lock();
    ImageInfos* info = _infos[file];
    lock_infos.unlock();

    if (!info)
    { // Change behavior: Linking data at the XP level
        // To be added workbench Id + selectionModifier
        QString exp = getExperimentName();// +_mdl->Pos();
        int ii = channel < 0 ? getChannelsFromFileName(file) : channel;

        bool exists = false;
        info = ImageInfos::getInstance(this, file, exp + QString("%1").arg(ii), ii, exists, key);

        _infos[file] = info;

        if (_mdl->getOwner()->hasProperty("ChannelsColor" + QString("%1").arg(ii)))
        {
            QColor col;
            QString cname = _mdl->getOwner()->property(QString("ChannelsColor%1").arg(ii));
            col.setNamedColor(cname);
            info->setColor(col, false);
        }
        else
            info->setDefaultColor(ii, false);

        // Also setup the channel names if needed
        //qDebug() << " --> DEBUG = " << _mdl->getChannelNames().size()<< _mdl->getChannelNames();
        if (_mdl->getChannelNames().size()>=ii)
        {
            QString name = _mdl->getChannelNames()[ii-1];
            info->setChannelName(name);
        }
    }


    return info;
}



double mse(cv::Mat i1, cv::Mat i2)
{
    double ms = 0;

    int rows = i1.rows, cols = i1.cols;
    assert(i1.rows == i2.rows && i1.cols == i2.cols);

    for (int i = 0; i < rows; ++i)
    {
        unsigned short* l = i1.ptr<unsigned short>(i);
        unsigned short* r = i2.ptr<unsigned short>(i);

        for (int j = 0; j < cols; ++j, ++l, ++r)
        {
            double d = (*l - *r);
            ms += d * d;
        }

    }

    return ms / (cols * rows);
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
                cv::Rect2d le(left.cols - c, 0, c, left.rows - r);
                cv::Rect2d ri(0, r, c, left.rows - r);

                double s = mse(left(le), right(ri));
                if (s < mii)
                {
                    mii = s;
                    res = QPoint(-c, -r);
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
                cv::Rect2d u = cv::Rect2d(c, up.rows - r, up.cols - c, r);
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

    QList<std::tuple<int, cv::Mat, cv::Mat, bool /*left/up*/, bool/*1st/2nd step*/> > data_mapping;

    this->pixOffset.resize(_mdl->getFieldCount());


    auto toField = _mdl->getOwner()->getFieldPosition();

    for (auto it = toField.begin(), end = toField.end(); it != end; ++it)
    {
        for (auto sit = it.value().begin(), send = it.value().end(); sit != send; ++sit)
        {
            //qDebug() << "Field " << sit.value() << " is at position" << it.key() << sit.key();
            if (ref.empty())
            {
                QString file = _mdl->getFile(_timepoint, sit.value(), _zpos, 1);
                ref = imageInfos(file, -1, loadkey)->image();
                continue;
            }
            QString file = _mdl->getFile(_timepoint, sit.value(), _zpos, 1);
            cv::Mat right = imageInfos(file, -1, loadkey)->image();

            std::tuple<int, cv::Mat, cv::Mat, bool /*left/up*/, bool/*1st/2nd step*/> tuple;

            tuple = std::make_tuple(sit.value() - 1, ref, right, sit == it.value().begin(), true);
            data_mapping.push_back(tuple);
            tuple = std::make_tuple(sit.value() - 1, ref, right, sit == it.value().begin(), false);
            data_mapping.push_back(tuple);

        }
        QString file = _mdl->getFile(_timepoint, it.value().begin().value(), _zpos, 1);
        ref = imageInfos(file, -1, loadkey)->image();
    }

    // Now i'd like to use the QtConcurrent Map function !!!
    struct Mapping {

        typedef std::tuple<int, double, QPoint> result_type;

        std::tuple<int, double, QPoint> operator()(std::tuple<int, cv::Mat, cv::Mat, bool /*left/up*/, bool/*1st/2nd step*/> data)
        {
            int xp = std::get<0>(data);

            cv::Mat a = std::get<1>(data);
            cv::Mat b = std::get<2>(data);

            bool left = std::get<3>(data);
            bool first = std::get<4>(data);

            std::tuple<double, QPoint> res;
            if (left)
                res = refineLeft(a, b, 200, first);
            else
                res = refineLower(a, b, 200, first);

            qDebug() << xp << (int)first << (left ? "RefineLeft" : "RefineLower") << std::get<0>(res) << std::get<1>(res);
            return std::make_tuple(xp, std::get<0>(res), std::get<1>(res));
        }

    };

    QList< std::tuple<int, double, QPoint> > res
            = QtConcurrent::blockingMapped(data_mapping, Mapping()); //apply the mapping

    //    for (auto d : data_mapping)
    //    {
    //        Mapping m;
    //        res.append(m(d));
    //    }


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
            if (start) { start = false; continue; }
            int field = toField[r][c] - 1;
            QPoint of = proj[field].second;
            if (r == 0) offset = pixOffset[toField[r][c - 1] - 1];
            if (c > 0) offset = pixOffset[toField[r][c] - 1];
            this->pixOffset[field] = offset + of;
        }
    }

}

void SequenceInteractor::preloadImage()
{

    QList<QPair<int, QString> > list = getAllChannel();
    for (QList<QPair<int, QString> >::iterator it = list.begin(), e = list.end(); it != e; ++it)
    {
        imageInfos(it->second, it->first, loadkey)->image();
    }

}


QPoint getMatrixSize(SequenceFileModel* seq, unsigned fieldc, int z, int t, int c)
{

    QSet<double> x, y;

    for (unsigned field = 1; field < fieldc; ++field)
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
    StitchStruct(SequenceInteractor* seq, bool bias_cor, float scale) : _sc(scale), bias_correction(bias_cor), seq(seq)
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
        // Need to check that all the objects are constructed in the main thread
        initImageInfos(_field);
        QImage toPix = getPixmapChannels(_field, bias_correction);
        _cachePixmap = QPixmap::fromImage(toPix);
        return _cachePixmap;
    }
    else // Unpack the data !!
    {
        QSettings set;
        float scale = set.value("unpackScaling", 1.0).toDouble();


        QList<int> perf;
        for (unsigned i = 0; i < _mdl->getFieldCount(); ++i) {
            initImageInfos(i + 1);
            perf.append(i + 1);
        }


        // Need to check that all the objects are constructed in the main thread
        QList<QPair<int, QImage> > toStitch = QtConcurrent::blockingMapped(perf, StitchStruct(this, bias_correction, scale));

        //        for (auto pr: perf)
        //        {
        //            StitchStruct s(this, bias_correction, scale);
        //            toStitch.append(s(pr));
        //        }

        auto li = _mdl->getOwner()->getFieldPosition();

        int rows = 0, cols = 0;
        for (int i = 0; i < toStitch.size(); ++i)
        {
            auto d = toStitch[i];
            rows = std::max(rows, li.size() * d.second.width());
            cols = std::max(cols, li.first().size() * d.second.height());
        }


        QVector<QPoint> proj(perf.size());
        for (auto a = li.begin(), ae = li.end(); a != ae; ++a)
            for (auto b = a->begin(), be = a->end(); b != be; ++b)
                proj[b.value() - 1] = QPoint(a.key(), b.key());


        QImage toPix(rows, cols, QImage::Format_RGBA8888);

        toPix.fill(Qt::black);
        for (unsigned i = 0; i < _mdl->getFieldCount(); ++i)
        {

            //            QPointF p = getFieldPos(_mdl, toStitch[i].first, _zpos, _timepoint, _channel);
            auto p = proj[i];

            int x = p.x();
            int y = p.y();

            QPainter pa(&toPix);
            QPoint offset = QPoint(x * toStitch[i].second.width(), y * toStitch[i].second.height());
            //qDebug() << "Field" << i << toStitch[i].first << "X Y:" << x << y << "Offset:" << offset;
            if (pixOffset.size() > 0)
                offset += pixOffset[i];

            pa.drawImage(offset, toStitch[i].second);
        }

        _cachePixmap = QPixmap::fromImage(toPix);
        return _cachePixmap;
    }
}


struct Loader
{
    Loader(float scale, bool changed_scale) : _scale(scale), ch_sc(changed_scale)
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
    int ncolors = imifo->nbColors();

    if (state.size() < 16) state.resize(16);
    //            QRgb black = QColor(0,0,0).rgb();
    for (int i = 0; i < rows; ++i)
    {
        unsigned short* p = image.ptr<unsigned short>(i);
        QRgb* pix = (QRgb*)toPix.scanLine(i);
        for (int j = 0; j < cols; ++j, ++p)
        {
            const unsigned short v = *p;
            if (v != 0 && state[v]) // FIXME Need to fuse the image data and not clear it....
                pix[j] = qRgb(std::min(255, qRed(pix[j]) + pa[(v + lastPal) % 16].red()),
                        std::min(255, qGreen(pix[j]) + pa[(v + lastPal) % 16].green()),
                        std::min(255, qBlue(pix[j]) + pa[(v + lastPal) % 16].blue()));
            else pix[j] = qRgb(std::min(255, qRed(pix[j]) + 0),
                               std::min(255, qGreen(pix[j]) + 0),
                               std::min(255, qBlue(pix[j]) + 0));
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
            unsigned short v = *p;
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
void colorizeImageUnbias(ImageInfos* imifo, cv::Mat& image, cv::Mat& bias, QImage& toPix, int rows, int cols, bool binarize)
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
        for (int j = 0; j < cols; ++j, ++p, ++b)
        {
            unsigned short v = *p / (*b / 10000.);
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
            unsigned short v = *p;
            if (v == 0)
            {
                pix[j] = qRgb(0, 0, 0);
            }
            else
            {
                std::minstd_rand0 nb(v);
                int r = nb(), g = nb(), b = nb(); // get a random value

                pix[j] = qRgb(std::min(255.f, qRed(pix[j]) + (float)(r & 0xFF)),
                              std::min(255.f, qGreen(pix[j]) + (float)(g & 0xFF)),
                              std::min(255.f, qBlue(pix[j]) + (float)(b & 0xFF)));
            }
        }
    }
}

template <bool Saturate, bool Inverted>
void colorMapImage(ImageInfos* imifo, cv::Mat& image, QImage& toPix, int rows, int cols)
{

    // Should get the colormap
    const float mi = imifo->getDispMin(),
            ma = imifo->getDispMax();
    using namespace colormap;

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
    //pal.rescale(0, 1);//
    float mami = ma - mi;
    for (int i = 0; i < rows; ++i)
    {
        unsigned short* p = image.ptr<unsigned short>(i);

        QRgb* pix = (QRgb*)toPix.scanLine(i);
        for (int j = 0; j < cols; ++j, ++p)
        {
            unsigned short v = *p;
            if (!Saturate)
                v = v > ma ? mi : v;
            float f = std::min(1.f, std::max(0.f, (v - mi) / (mami)));
            if (Inverted) f = 1 - f;

            auto colo = pal(f);
            pix[j] = qRgb(std::min(255.f, qRed(pix[j]) + (float)colo[0]),
                    std::min(255.f, qGreen(pix[j]) + (float)colo[1]),
                    std::min(255.f, qBlue(pix[j]) + (float)colo[2]));
        }
    }
}


void SequenceInteractor::initImageInfos(int field)
{
    QList<QPair<int, QString> > list = getAllChannel(field);
    for (QList<QPair<int, QString> >::iterator it = list.begin(), e = list.end(); it != e; ++it)
    {
        QString file = it->second;
        int channel = it->first;
        int ii = channel < 0 ? getChannelsFromFileName(file) : channel;
        bool exists;
        ImageInfos::getInstance(this, file, getExperimentName() + QString("%1").arg(ii), ii, exists, loadkey);
    }
}

/* #region Main */

QImage SequenceInteractor::getPixmapChannels(int field, bool bias_correction, float scale)
{
    QList<QPair<int, QString> > list = getAllChannel(field);

    QList<ImageInfos*> img;
    QList<cv::Mat> images;//(list.size());
    int ii = 0;
    for (QList<QPair<int, QString> >::iterator it = list.begin(), e = list.end(); it != e; ++it, ++ii)
    {
        img.append(imageInfos(it->second, it->first, loadkey)); // Auto channel determination
    }
    if (img.isEmpty())
        return QImage();

    images = QtConcurrent::blockingMapped(img, Loader(scale, last_scale != scale));

    //    for (auto im : img)
    //    {
    //        Loader l(scale, last_scale != scale);
    //        images.append(l(im));
    //    }

    last_scale = scale;


    //  qDebug() << t.elapsed() << "ms";
    //  qDebug() << "Got image list";
    int rows = 0, cols = 0;

    for (int i = 0; i < images.size(); ++i)
    {
        rows = std::max(rows, images[i].rows);
        cols = std::max(cols, images[i].cols);
    }

    //  qDebug() << rows << cols ;

    QImage toPix(cols, rows, QImage::Format_RGBA8888);

    toPix.fill(Qt::black);

    int lastPal = 0;
    for (int c = 0; c < list.size(); ++c)
    {
        if (images[c].empty()) continue;

        int ncolors = img[c]->nbColors();
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
                    cv::Mat bias = img[c]->bias(c + 1);

                    if (saturate && inverted)
                        colorizeImageUnbias<true, true>(img[c], images[c], bias, toPix, rows, cols, binarize);
                    if (saturate && !inverted)
                        colorizeImageUnbias<true, false>(img[c], images[c], bias, toPix, rows, cols, binarize);
                    if (!saturate && inverted)
                        colorizeImageUnbias<false, true>(img[c], images[c], bias, toPix, rows, cols, binarize);
                    if (!saturate && !inverted)
                        colorizeImageUnbias<false, false>(img[c], images[c], bias, toPix, rows, cols, binarize);

                }
                else
                {
                    if (saturate && inverted)
                        colorizeImage<true, true>(img[c], images[c], toPix, rows, cols, binarize);
                    if (saturate && !inverted)
                        colorizeImage<true, false>(img[c], images[c], toPix, rows, cols, binarize);
                    if (!saturate && inverted)
                        colorizeImage<false, true>(img[c], images[c], toPix, rows, cols, binarize);
                    if (!saturate && !inverted)
                        colorizeImage<false, false>(img[c], images[c], toPix, rows, cols, binarize);

                }
            }
            else {
                if (saturate && inverted)
                    colorMapImage<true, true>(img[c], images[c], toPix, rows, cols);
                if (saturate && !inverted)
                    colorMapImage<true, false>(img[c], images[c], toPix, rows, cols);
                if (!saturate && inverted)
                    colorMapImage<false, true>(img[c], images[c], toPix, rows, cols);
                if (!saturate && !inverted)
                    colorMapImage<false, false>(img[c], images[c], toPix, rows, cols);

            }

        }
    }
    return toPix;
}

/* endregion */

QList<unsigned> SequenceInteractor::getData(QPointF d, int& field, bool packed, bool bias)
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

        cv::Mat m = imageInfos(getFile(), -1, loadkey)->image();
        int cx = floor(d.x() / m.cols);
        int cy = floor(d.y() / m.rows);

        //auto origD = d;

        d.setX(d.x() - cx * m.cols);
        d.setY(d.y() - cy * m.rows);

        auto toField = _mdl->getOwner()->getFieldPosition();
        if (!toField.contains(cx) || !toField[cx].contains(cy))
        {
            /*  qDebug() << "Field not found for displaying value:"
                << "intial coordinates: " << origD
                << "Searching pixel pos: " << d << " Pos correction " << cx << cy;*/
            field = -1;
            res << -1;
        }
        else
        {

            int f = toField[cx][cy];
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
    return qRgb((float)(r & 0xFF), (float)(g & 0xFF), (float)(b & 0xFF));
}


void getMinMax(cv::Mat& ob, int f, float& cmin, float& cmax)
{
    if (f < 0)
        return;
    cmin = std::numeric_limits<float>::max();
    cmax = -std::numeric_limits<float>::max();

    for (int r = 0; r < ob.rows; ++r)
    {
        float v = ob.at<float>(r, f);
        if (v < cmin) cmin = v;
        else if (v > cmax) cmax = v;
    }

}


// Return the list of overlays
QList<QString> SequenceInteractor::getMetaList()
{
    QStringList l;
    int cns = _mdl->getMetaChannels(_timepoint, _field, _zpos);

    for (int c = 1; c <= cns; ++c)
    {
        QMap<QString, StructuredMetaData>& data = _mdl->getMetas(_timepoint, _field, _zpos, c);
        // We can add some selectors
        l += data.keys();
    }

    return std::move(l);
}

int SequenceInteractor::getOverlayMax(QString name)
{
    int cns = _mdl->getMetaChannels(_timepoint, _field, _zpos);
    //qDebug() << _timepoint << _field << _zpos << cns;
    for (int c = 1; c <= cns; ++c)
    {
        QMap<QString, StructuredMetaData>& data = _mdl->getMetas(_timepoint, _field, _zpos, c);
        if (data.contains(name))
        {
            StructuredMetaData& k = data[name];

            cv::Mat& feat = k.content();
            return feat.rows;
        }
    }
    return -1;
}

double SequenceInteractor::getOverlayWidth()
{
    return _overlay_width;
}

void SequenceInteractor::setOverlayWidth(double v)
{
    _overlay_width = v;
    modifiedImage();
}

QStringList SequenceInteractor::getMetaTags(int )
{
    return QStringList();
}


// Return selectable color coded displays

QList<QString> SequenceInteractor::getMetaOptionsList(QString meta)
{ // Returns list of displayable options
    int cns = _mdl->getMetaChannels(_timepoint, _field, _zpos);
    QStringList l;

    auto sub = QStringList() << "_X" << "_Y" << "_Top" << "_Left" << "_Width" << "_Height";

    for (int c = 1; c <= cns; ++c)
    {
        QMap<QString, StructuredMetaData>& data = _mdl->getMetas(_timepoint, _field, _zpos, c);
        // We can add some selectors
        if (data.contains(meta))
        {
            auto k = data[meta];

            QString cols = k.property("ChannelNames");

            QStringList sp = cols.split(";");

            for (auto s : sp)
            {
                bool add = true;
                for (auto ss : sub)
                {
                    if (s.contains(ss))
                    {
                        add = false;
                        break;
                    }
                }
                if (add)  l << s;
            }
        }
    }

    return std::move(l);
}

typedef struct dispType {
    dispType(int t_, int l_,
             int w_, int h_, int a_,
             int f_, colormap::map<colormap::rgb> pal_) :
        r(0),
        t(t_), l(l_),
        w(w_), h(h_), a(a_),
        f(f_), pal(pal_), color("invalid"),
        random(false), shape(Rectangle)
    {
    }

    int r, t, l, w, h, a, f;

    colormap::map<colormap::rgb> pal;
    QColor color;
    bool random;

    enum { Rectangle, Ellispse, Point, Line } shape;
    float overlay_width;

} t_dispType;

template <class GraphItem>
void adaptItem(GraphItem* item, QString tip, QString name, QRectF rect, QPointF ori, float rotation, QPen p, int pos)
{
    item->setRect(rect);

    item->setTransformOriginPoint(ori);
    item->setRotation(rotation);

    item->setToolTip(tip);
    item->setPen(p);
    item->setData(1, name);
    item->setData(2, pos);
}


template<>
void adaptItem(QGraphicsLineItem* item, QString tip, QString name, QRectF rect, QPointF ori, float rotation, QPen p, int pos)
{
    item->setLine(rect.x(), rect.y(), rect.width(), rect.height());

    if (std::abs(rotation) > 1e-9)
    {
        item->setTransformOriginPoint(ori);
        item->setRotation(rotation);
    }

    //    tip += QString("Length: %1").arg(sqrt(pow(rect.x()-rect.width(),2)+pow(rect.y()-rect.height(),2)));

    item->setToolTip(tip);
    item->setPen(p);
    item->setData(1, name);
    item->setData(2, pos);
}

void drawItem(cv::Mat& feat, QStringList lcols, QString name, QList<int> feats,
              QGraphicsItemGroup* group, t_dispType d)
{
    if (d.r < feat.rows)
    {
        float x = feat.at<float>(d.r, d.t),
                y = feat.at<float>(d.r, d.l),
                width = feat.at<float>(d.r, d.w),
                height = feat.at<float>(d.r, d.h);
        QString tip = QString("Id: %1\r\n").arg(d.r);
        for (auto p : feats)
        {
            float fea = feat.at<float>(d.r, p);
            tip += QString("%1: %2\r\n").arg(lcols[p].trimmed()).arg(fea);
        }

        float fea = d.f >= 0 ? feat.at<float>(d.r, d.f) : 0;

        auto colo = d.pal(fea);

        QPen p(qRgb(colo[0], colo[1], colo[2]));

        if (d.color.isValid()) // Force a color to be set !
            p.setColor(d.color);

        if (d.random || d.f < 0)
            p.setColor(randCol(d.r));

        p.setWidthF(d.overlay_width);

        switch (d.shape) {
        case   dispType::Rectangle:
            adaptItem(new QGraphicsRectItem(group), tip, name,
                      QRectF(x, y, width, height),
                      QPointF(x + width / 2., y + height / 2.),
                      d.a >= 0 ? feat.at<float>(d.r, d.a) : 0, p, d.r);
            break;
        case dispType::Ellispse:
            adaptItem(new QGraphicsEllipseItem(group), tip, name,
                      QRectF(x, y, width, height),
                      QPointF(x + width / 2., y + height / 2.),
                      d.a >= 0 ? feat.at<float>(d.r, d.a) : 0, p, d.r);
            break;

        case dispType::Line:
            adaptItem(new QGraphicsLineItem(group), tip, name,
                      QRectF(x, y, width, height),
                      QPointF(0, 0), 0, p, d.r);
            break;

        case dispType::Point:
            adaptItem(new QGraphicsEllipseItem(group), tip, name,
                      QRectF(x - d.overlay_width / 2., y - d.overlay_width / 2., d.overlay_width, d.overlay_width),
                      QPointF(d.overlay_width / 2., d.overlay_width / 2.), 0, p, d.r);
            break;
        }

    }
}

QList<QGraphicsItem*> SequenceInteractor::getMeta(QGraphicsItem* parent)
{
    // The interactor will filter / scale & analyse the meta data to be generated
    QList<QGraphicsItem*> res;



    //    qDebug() << "Get Meta Called, " << disp_tile << tile_id;
    if (disp_overlay["Tile"].first)
    { // Specific code to display tile with respect to position !!!
        QSettings set;
        int tile_id = disp_overlay["Tile"].second;

        int rx = set.value("TileSizeX", 256).toInt() / 2,
                ry = set.value("TileSizeX", 216).toInt() / 2;


        auto item = new QGraphicsRectItem(parent);

        float x = rx * rint(tile_id % 19);
        float l = (tile_id - (tile_id % 19));
        float y = ry * rint(l / 19);

        float  width = rx * 2,
                height = ry * 2;

        item->setBrush(QBrush());
        item->setRect(QRectF(x, y, width, height));
        QPen p(Qt::yellow);
        p.setWidthF(_overlay_width);
        item->setPen(p);
        //        qDebug() << item->rect();
        res << item;
    }

    if (disp_overlay["Scale"].first)
    { // Should display the scale as an overlay
        //parent->window()
        // We shall know the pixel size
        auto sc = parent->scene();

        auto view = (sc->views().front());
        auto boundaries = parent->mapFromScene(view->mapToScene(view->viewport()->geometry()));
        if (boundaries.size() == 4)
        {
            auto key = boundaries[2];
            //

            float dx, dy;
            getResolution(dx, dy);

            auto item = new QGraphicsLineItem(parent);
            if (disp_overlay["Scale"].second >= 0)
            {
                int len = disp_overlay["Scale"].second;

                float size = len / dx;
                // We need to figure out the position we have with respect to the border...
                QPen p(Qt::yellow);
                p.setWidthF(_overlay_width);
                item->setPen(p);
                QPointF x1(key.x() - size - 50, key.y() - 20), x2(key.x() - 50, key.y() - 20);
                QLineF line(x1, x2);
                item->setLine(line);
                // Now write text above
                //               parent->par
                auto t = new QGraphicsTextItem(parent);

                t->setPlainText(QString("%1 µm").arg(len));
                t->setPos(key.x() - size / 2 - 50, key.y() - 25);

                res << item << t;
            }
        }

    }


    //    SequenceFileModel::Channel& chans = _mdl->getChannelsFiles(_timepoint, _field, _zpos);

    int cns = _mdl->getMetaChannels(_timepoint, _field, _zpos);
    //    qDebug() << "Overlays :" << _timepoint << _field << _zpos << cns;
    for (int c = 1; c <= cns; ++c)
    {
        QMap<QString, StructuredMetaData>& data = _mdl->getMetas(_timepoint, _field, _zpos, c);
        // We can add some selectors

        //        for (auto & [k: data)
        for (QMap<QString, StructuredMetaData>::iterator it = data.begin(), e = data.end(); it != e; ++it)
            if (disp_overlay.contains(it.key()) && disp_overlay[it.key()].first)
            {
                StructuredMetaData& k = it.value();

                QString cols = k.property("ChannelNames");

                cv::Mat& feat = k.content();
                QStringList lcols = cols.split(";").mid(0, feat.cols);
                QList<int> feats;

                int t = 0, l = 0, w = 0, h = 0, f = -1, a = -1;


                for (int i = 0; i < lcols.size(); ++i)
                {
                    QString s = lcols[i];
                    if (s.contains("_Top"))   l = i;
                    else if (s.contains("_Left"))  t = i;
                    else if (s.contains("_X"))   t = i;
                    else if (s.contains("_Y"))  l = i;
                    else if (s.contains("_Width")) w = i;
                    else if (s.contains("_Height")) h = i;
                    else if (s.contains("_Angle")) a = i;
                    else { feats << i; }

                    if (overlay_coding.contains(it.key()) && s == overlay_coding[it.key()].first)
                        f = i;
                }

                float cmin = 0, cmax = 1;
                if (f < 0 && feats.size())
                    f = feats.first();
                getMinMax(feat, f, cmin, cmax);


                auto pal = colormap::palettes.at("jet").rescale(cmin, cmax);
                if (colormap::palettes.count(overlay_coding[it.key()].second.toStdString()))
                {
                    pal = colormap::palettes.at(overlay_coding[it.key()].second.toStdString()).rescale(cmin, cmax);
                }
                t_dispType disp(t, l, w, h, a, f, pal);

                if (overlay_coding[it.key()].second.startsWith("#"))
                    disp.color = QColor(overlay_coding[it.key()].second);
                if (overlay_coding[it.key()].second == "random")
                    disp.random = true;

                if (cols.contains("_Width") && cols.contains("_Height"))
                {
                    if (cols.contains("_Top") && cols.contains("_Left"))
                        disp.shape = dispType::Rectangle;
                    else if (cols.contains("_X") && cols.contains("_Y"))
                        disp.shape = dispType::Ellispse;
                }
                else
                {
                    if (cols.count("_X") >= 2 && cols.count("_Y") >= 2)
                    {
                        //                        qDebug() << "Line Mode";
                        disp.shape = dispType::Line;
                        // Special case for line, adjust the column counting
                        QStringList lcols = cols.split(";").mid(0, feat.cols);

                        for (int i = 0; i < lcols.size(); ++i)
                        {
                            QString s = lcols[i];
                            if (s.contains("_X")) {
                                disp.t = i;
                                disp.l = findY(lcols, s);
                                for (int j = i + 1; j < lcols.size(); ++j)
                                    if (lcols[j].contains("_X"))
                                    {
                                        disp.w = j;
                                        disp.h = findY(lcols, lcols[j]);
                                        break;
                                    }
                                break;
                            }
                        }

                    }
                    else
                        disp.shape = dispType::Point;
                }

                disp.overlay_width = _overlay_width;


                auto group = new QGraphicsItemGroup(parent);
                group->setData(10, it.key());
                if (disp_overlay[it.key()].second >= 0)
                {
                    disp.r = disp_overlay[it.key()].second;
                    drawItem(feat, lcols, it.key(), feats, group, disp);
                }
                else
                    for (disp.r = 0; disp.r < feat.rows; ++disp.r)
                        drawItem(feat, lcols, it.key(), feats, group, disp);

                res << group;
            }
    }

    return res;
}

void SequenceInteractor::getResolution(float& x, float& y)
{
    static float _x = 0, _y = 0;
    if (_x == 0)
    {
        _x = _mdl->property(QString("HorizontalPixelDimension_ch%1").arg(_channel)).toDouble();
        _y = _mdl->property(QString("VerticalPixelDimension_ch%1").arg(_channel)).toDouble();
    }
    x = _x;
    y = _y;
}

