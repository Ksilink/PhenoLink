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
}

void SequenceInteractor::setZ(unsigned z)
{
    if (_mdl->getZCount() >= z)
        _zpos = z;
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
    _infos.clear();*/
    qDebug() << "FIXME: Clear Memory called for SequenceInteractor, but may not be honored";
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
    ImageInfos* info = ImageInfos::getInstance(this, file, exp + QString("%1").arg(ii), exists, key);
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
    }
    /*    lock_infos.lock();ta
        _infos[file] = info;
        lock_infos.unlock();*/
    //}

    return info;
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
    StitchStruct(SequenceInteractor* seq, float scale): _sc(scale), seq(seq)
    {}

    typedef QPair<int, QImage> result_type;

    QPair<int, QImage> operator()(int field)
    {
        return  qMakePair(field, seq->getPixmapChannels(field, _sc));
    }

    float _sc;
    SequenceInteractor* seq;
};

// Warning: Critical function for speed
// ENHANCE: Modify the handling of scale information

QPixmap SequenceInteractor::getPixmap(bool packed, float scale)
{
    qDebug() << "getPixmap" << packed;
    Q_UNUSED(scale);
    if (packed)
    {
        QImage toPix = getPixmapChannels(_field);
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
        QList<QPair<int, QImage> > toStitch = QtConcurrent::blockingMapped(perf, StitchStruct(this, scale));


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

QImage SequenceInteractor::getPixmapChannels(int field, float scale)
{
    QStringList list = getAllChannel(field);

    QList<ImageInfos*> img;
    QList<cv::Mat> images;//(list.size());
    int ii = 0;
    for (QStringList::iterator it = list.begin(), e = list.end(); it != e; ++it,++ii)
    {
        img.append(imageInfos(*it,ii+1, loadkey));
    }


    images = QtConcurrent::blockingMapped(img, Loader(scale, last_scale == scale));


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

        if (!img[c]->active()) { if (ncolors < 16) lastPal += ncolors; continue; }

        if (img[c]->getMin() >= 0 && ncolors < 16)
        {
            QVector<QColor> pa = img[c]->getPalette();
            QVector<int> state = img[c]->getState();
            if (state.size() < 16) state.resize(16);
            //            QRgb black = QColor(0,0,0).rgb();
            for (int i = 0; i < rows; ++i)
            {
                unsigned short *p = images[c].ptr<unsigned short>(i);
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
        else
        {
            const float mi = img[c]->getDispMin(),
                    ma = img[c]->getDispMax();
            const int R = img[c]->Red();
            const int G = img[c]->Green();
            const int B = img[c]->Blue();

            float mami = ma - mi;
            try {
                for (int i = 0; i < rows; ++i)
                {
                    unsigned short* p = images[c].ptr<unsigned short>(i);
                    QRgb* pix = (QRgb*)toPix.scanLine(i);
                    for (int j = 0; j < cols; ++j, ++p)
                    {
                        const unsigned short v = *p;

                        const float f = std::min(1.f, std::max(0.f, (v - mi) / (mami)));

                        pix[j] = qRgb(std::min(255.f, qRed(pix[j]) + f * B),
                                      std::min(255.f, qGreen(pix[j]) + f * G),
                                      std::min(255.f, qBlue(pix[j]) + f * R));
                    }
                }
            } catch(...)
            {
                toPix.fill(Qt::black);
            }
        }
    }
    return toPix;
}


QList<unsigned> SequenceInteractor::getData(QPointF d)
{
    QStringList list = getAllChannel();

    QList<unsigned> res;
    int ii = 0;

    for (QStringList::iterator it = list.begin(), e = list.end(); it != e; ++it, ++ii)
    {
        cv::Mat m = imageInfos(*it, -1, loadkey)->image();
        if (m.rows > d.y() && m.cols > d.x())
            res << m.at<unsigned short>((int)d.y(), (int)d.x());
    }

    return res;
}
