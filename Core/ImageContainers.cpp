#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "ImageContainers.h"

#include <QDir>

QString getbasePath(QJsonArray data)
{
    QDir dir(data.first().toString());
    return dir.dirName();
}

cv::Mat loadImage(QJsonArray data, int im = -1)
{
    std::vector<cv::Mat> vec;
    cv::Mat mat;

    if (data.size() > 1)
    {
        for (size_t i = 0; i < (size_t)data.size(); ++i)
        {
            if (im >= 0 && im != (int)i) continue;
            cv::Mat m = cv::imread(data.at((int)i).toString().toStdString(), 2);
            if (m.type() != CV_16U)
            {
                cv::Mat t;
                m.convertTo(t, CV_16U);
                m = t;
            }
            if (!m.empty())
                vec.push_back(m);
        }
        if (vec.size()>1)
            cv::merge(&vec[0], vec.size(), mat); // FIXME : Overkill case most of the time we want to process decorrelated channels !!!
        else
            if (vec.size() == 1)
                mat = vec[0];
    }
    else
    {
        mat = cv::imread(data.first().toString().toStdString(), 2);
        if (mat.type() != CV_16U)
        {
            cv::Mat t;
            mat.convertTo(t, CV_16U);
            mat = t;
        }

    }
    return mat;
}

namespace cocvMat
{
    void loadFromJSON(QJsonObject data, cv::Mat& mat, int im)
    {
        QJsonArray times =    data["Data"].toArray();

        mat = loadImage(times, im);
    }

}

ImageContainer::ImageContainer(): _loaded(false), _count(0)
{

}

ImageContainer::~ImageContainer()
{

}

size_t ImageContainer::count()
{
    return _count;
}

cv::Mat &ImageContainer::operator[](size_t i)
{
    static cv::Mat b;
    if (i < images.size())
        return images[i];
    return b;
}

cv::Mat &ImageContainer::operator()(size_t i)
{
    return images[i];
}

void ImageContainer::loadFromJSON(QJsonObject data)
{
    _loaded = true;

    cv::Mat mat;
    cocvMat::loadFromJSON(data, mat, -1);

    if (!mat.empty()) images.push_back(mat);
}

void ImageContainer::storeJson(QJsonObject json)
{
    _data = json;
    _count  = _data["Data"].toArray().size();
}

QString ImageContainer::basePath(QJsonObject data)
{
    QJsonArray times =    data["Data"].toArray();
    return getbasePath(times);
}

cv::Mat ImageContainer::getImage(size_t i)
{
    if (_loaded) return (*this)[i];


    cv::Mat mat;
    cocvMat::loadFromJSON(_data, mat, (int)i);
    return mat;
}



void ImageContainer::setCount(size_t s)
{
    images = std::vector<cv::Mat>(s);
    _count = s;
}

void ImageContainer::setImage(size_t i, cv::Mat mat)
{
    if (i < images.size())
    {
        images[i] = mat;
    }
}

void ImageContainer::deallocate()
{
    for(size_t i = 0; i < images.size(); ++i)
        images[i].release();
}

size_t ImageContainer::getChannelCount()
{
      return _data["Data"].toArray().size();
}


void TimeImage::loadFromJSON(QJsonObject data)
{
    _loaded = true;

    QJsonArray times =    data["Data"].toArray();
    for (int i = 0; i < times.size(); ++i)
    {
        QJsonObject ob = times.at(i).toObject();
        QJsonArray chans = ob["Data"].toArray();

        images.push_back(loadImage(chans));
    }
}

QString TimeImage::basePath(QJsonObject data)
{
    QJsonArray times =    data["Data"].toArray();
    return getbasePath(times);
}

cv::Mat TimeImage::getImage(size_t i)
{
    if (_loaded) return (*this)[i];

    QJsonArray times =    _data["Data"].toArray();
    {
        QJsonObject ob = times.at((int)i).toObject();
        QJsonArray chans = ob["Data"].toArray();

        return loadImage(chans);
    }
}


void StackedImage::loadFromJSON(QJsonObject data)
{

    QJsonArray times =    data["Data"].toArray();
    for (int i = 0; i < times.size(); ++i)
    {
        QJsonObject ob = times.at(i).toObject();
        QJsonArray chans = ob["Data"].toArray();

        this->images.push_back(loadImage(chans));
    }
    //        qDebug() << "LoadFromJSON not implemented for StackedImage";
}

cv::Mat StackedImage::getImage(size_t i, size_t chann)
{
    if (_loaded) return (*this)[i];

    if (!images.empty())
    {
        if (chann < 0 || images[i].depth() == 1)
            return images[i];
        else
        {
            cv::Mat spl;
            cv::extractChannel(images[i], spl, (int)chann);
            return spl;
        }
    }


    QJsonArray times =    _data["Data"].toArray();
    QJsonObject ob = times.at((int)i).toObject();
    QJsonArray chans = ob["Data"].toArray();

    return loadImage(chans, (int)chann);
}

size_t StackedImage::getChannelCount()
{
    return _data["Data"].toArray().first().toObject()["Data"].toArray().size();
}


QString StackedImage::basePath(QJsonObject data)
{
    QJsonArray times =    data["Data"].toArray();
    return getbasePath(times);
}


TimeStackedImage::TimeStackedImage(): _loaded(true) {}

size_t TimeStackedImage::count()
{
    return _times.size();
}

StackedImage &TimeStackedImage::operator[](size_t i)
{
    return _times[i];
}

void TimeStackedImage::loadFromJSON(QJsonObject data)
{
    _loaded = true;

    QJsonArray stack =    data["Data"].toArray();
    for (int i = 0; i < stack.size(); ++i)
    {
        QJsonObject ob = stack.at(i).toObject();
        StackedImage im;
        im.loadFromJSON(ob);
        _times.push_back(im);
    }
}

QString TimeStackedImage::basePath(QJsonObject data)
{
    QJsonArray stack =    data["Data"].toArray();
    QJsonObject ob = stack.first().toObject();
    StackedImage im;
    return im.basePath(ob);

}

void TimeStackedImage::storeJson(QJsonObject json)
{
    _data = json;
    _count = json["Data"].toArray().size();
}

StackedImage TimeStackedImage::getImage(size_t i)
{
    if (_loaded) return (*this)[i];

    StackedImage im;
    im.loadFromJSON(_data["Data"].toArray().at((int)i).toObject());
    return im;
}

void TimeStackedImage::deallocate()
{
    for(size_t i = 0; i < _times.size(); ++i)
        //foreach(StackedImage& mat, _times)
        _times[i].deallocate();
}




void ImageXP::loadFromJSON(QJsonObject data)
{
    Q_UNUSED(data);
    _loaded = true;

    QJsonArray field =    _data["Data"].toArray();
    for (int i = 0; i < field.size(); ++i)
    {
        QJsonObject ob = field.at(i).toObject();
        QJsonArray chans = ob["Data"].toArray();

        images.push_back(loadImage(chans));
    }
}

QString ImageXP::basePath(QJsonObject json)
{
    Q_UNUSED(json);

    QJsonArray field =    _data["Data"].toArray();
    return getbasePath(field.first().toObject()["Data"].toArray());
}

size_t ImageXP::getChannelCount()
{
    QJsonArray field =    _data["Data"].toArray();

    QJsonObject ob = field.at(0).toObject();
     QJsonArray chans = ob["Data"].toArray();
     return chans.size();
}

cv::Mat ImageXP::getImage(int i, int c)
{
    if (_loaded) {
        if (c >= 0)
        {
            cv::Mat res;
            cv::extractChannel((*this)[i], res, c);
            return res;
        }
        else
            return (*this)[i];
    }

    QJsonArray field =    _data["Data"].toArray();

    QJsonObject ob = field.at(i).toObject();
    QJsonArray chans = ob["Data"].toArray();

    return loadImage(chans,c);

}


TimeImageXP::TimeImageXP(): _loaded(false) {}

size_t TimeImageXP::count()
{
    return _count;
}

TimeImage &TimeImageXP::operator[](size_t i)
{
    return _xp[i];
}

void TimeImageXP::loadFromJSON(QJsonObject data)
{
    Q_UNUSED(data);
    //  qDebug() << "LoadFromJSON not implemented for TimeImageXP";
    _loaded = true;

    QJsonArray stack =    _data["Data"].toArray();
    for (int i = 0; i < stack.size(); ++i)
    {
        QJsonObject ob = stack.at(i).toObject();
        TimeImage im;
        im.loadFromJSON(ob);
        _xp.push_back(im);
    }
}

QString TimeImageXP::basePath(QJsonObject json)
{
    QJsonArray stack =    _data["Data"].toArray();
    QJsonObject ob = stack.first().toObject();
    TimeImage im;
    return im.basePath(ob);
}

void TimeImageXP::storeJson(QJsonObject json)
{
    _data = json;
    _count = _data["Data"].toArray().size();
}

TimeImage TimeImageXP::getImage(size_t i)
{
    if (_loaded) return (*this)[i];

    QJsonArray stack =    _data["Data"].toArray();
    QJsonObject ob = stack.at((int)i).toObject();
    TimeImage im;
    im.loadFromJSON(ob);

    return im;


}

void TimeImageXP::deallocate()
{
    //  foreach(TimeImage& mat, _xp)
    for(size_t i = 0; i < _xp.size(); ++i)
        _xp[i].deallocate();
}


StackedImageXP::StackedImageXP(): _loaded(false) {}

size_t StackedImageXP::count()
{
    return _count;
}

StackedImage &StackedImageXP::operator[](size_t i)
{
    return _xp[i];
}

void StackedImageXP::loadFromJSON(QJsonObject data)
{
    _loaded = true;

    QJsonArray stack =    data["Data"].toArray();
    for (int i = 0; i < stack.size(); ++i)
    {
        QJsonObject ob = stack.at(i).toObject();
        StackedImage im;
        im.loadFromJSON(ob);
        _xp.push_back(im);
    }
}

QString StackedImageXP::basePath(QJsonObject data)
{
    QJsonArray stack =    data["Data"].toArray();
    QJsonObject ob = stack.first().toObject();
    StackedImage im;
    return im.basePath(ob);

}


 size_t StackedImageXP::getChannelCount()
{
     QJsonArray stack = _data["Data"].toArray();
     QJsonObject ob = stack.at((int)0).toObject();
     StackedImage im;
     im.storeJson(ob);
     return im.getChannelCount();
}


void StackedImageXP::storeJson(QJsonObject json)
{
    _data = json;
    _count = _data["Data"].toArray().size();
}

StackedImage StackedImageXP::getImage(size_t i)
{
    if (_loaded) return (*this)  [i];
    QJsonArray stack =    _data["Data"].toArray();
    QJsonObject ob = stack.at((int)i).toObject();
    StackedImage im;
    im.storeJson(ob);
    return im;
}


void StackedImageXP::deallocate()
{
    for(size_t i = 0; i < _xp.size(); ++i)
        _xp[i].deallocate();
}


TimeStackedImageXP::TimeStackedImageXP() : _loaded(false) {}

size_t TimeStackedImageXP::count()
{
    return _xp.size();
}

TimeStackedImage &TimeStackedImageXP::operator[](size_t i)
{
    return _xp[i];
}

void TimeStackedImageXP::loadFromJSON(QJsonObject data)
{
    _loaded = true;

    QJsonArray stack =    data["Data"].toArray();
    for (int i = 0; i < stack.size(); ++i)
    {
        QJsonObject ob = stack.at(i).toObject();
        TimeStackedImage im;
        im.loadFromJSON(ob);
        _xp.push_back(im);
    }
}

QString TimeStackedImageXP::basePath(QJsonObject data)
{
    QJsonArray stack =    data["Data"].toArray();
    QJsonObject ob = stack.first().toObject();
    TimeStackedImage im;
    return im.basePath(ob);
}

void TimeStackedImageXP::storeJson(QJsonObject json)
{
    _data = json;
    _count = _data["Data"].toArray().size();
}

TimeStackedImage TimeStackedImageXP::getImage(size_t i)
{
    if (_loaded) return (*this)[i];

    QJsonArray stack =    _data["Data"].toArray();
    QJsonObject ob = stack.at((int)i).toObject();
    TimeStackedImage im;
    im.loadFromJSON(ob);
    return im;

}

void TimeStackedImageXP::deallocate()
{
    for(size_t i = 0; i < _xp.size(); ++i)
        _xp[i].deallocate();
}



size_t WellPlateXP::countX()
{
    return _plate.lastKey();
}

size_t WellPlateXP::countY()
{
    size_t r = 0;
    for (QMap<size_t, QMap<size_t, TimeStackedImageXP> >::iterator it = _plate.begin(), e  = _plate.end(); it != e; ++it)
     {
        r = std::max((size_t)(it.value().lastKey()), r);
    }
    return r;
}

bool WellPlateXP::exists(size_t i, size_t j)
{
    return (_plate.count(i) > 0) && (_plate[i].count(j) > 0);
}

TimeStackedImageXP &WellPlateXP::operator()(size_t i, size_t j)
{
    return _plate[i][j];
}

size_t WellPlateXP::getChannelCount()
{
    for (size_t x = 0; x < countX(); ++x)
        for (size_t y = 0; y < countY(); ++y)
            if (exists(x,y))
            {
                TimeStackedImageXP& well = _plate[x][y];
                for (size_t field = 0; field < well.count(); ++field)
                {
                    TimeStackedImage& timestack = well[field];
                    for (size_t time = 0; time < timestack.count(); ++time)
                    {
                        StackedImage& stack = timestack[time];
                        for (size_t z = 0; z < stack.count(); ++z)
                            return stack.getChannelCount();
                    }
                }
            }
    return 0;
}

void WellPlateXP::storeJson(QJsonObject json)
{
    this->_data = json;
}

void WellPlateXP::loadFromJSON(QJsonObject data)
{
    Q_UNUSED(data);

    auto pl = data["Data"].toObject();
    for (auto kv = pl.begin(), e = pl.end(); kv != e; ++kv)
    {
        int x = kv.key().toUInt();
        auto yy = kv.value().toObject();
        for (auto kkv = yy.begin(), ke = yy.end(); kkv != ke; ++kkv)
        {
            int y = kkv.key().toUInt();
            auto oo = kkv.value().toObject();
            TimeStackedImageXP xp;
            xp.loadFromJSON(oo);
            _plate[x][y] = xp;
        }
    }
}

QString WellPlateXP::basePath(QJsonObject json)
{
    Q_UNUSED(json);
    //    QDir dir(data.first().toString());
    //    return dir.dirName();


    return QString();
}

void WellPlateXP::deallocate()
{
    for (QMap<size_t, QMap<size_t, TimeStackedImageXP> >::iterator it = _plate.begin(), e  = _plate.end(); it != e; ++it)
    {
        for (QMap<size_t, TimeStackedImageXP>::iterator sit = it.value().begin(), se  = it.value().end(); sit != se; ++sit)
            sit.value().deallocate();
    }
}






size_t WellPlate::countX()
{
    return _plate.lastKey();
}

size_t WellPlate::countY()
{
    size_t r = 0;
    for (auto it = _plate.begin(), e  = _plate.end(); it != e; ++it)
     {
        r = std::max((size_t)(it.value().lastKey()), r);
    }
    return r;
}

bool WellPlate::exists(size_t i, size_t j)
{
    return (_plate.count(i) > 0) && (_plate[i].count(j) > 0);
}

TimeStackedImage &WellPlate::operator ()(size_t i, size_t j)
{
    return _plate[i][j];
}

size_t WellPlate::getChannelCount()
{
    for (size_t x = 0; x < countX(); ++x)
        for (size_t y = 0; y < countY(); ++y)
            if (exists(x,y))
            {
                TimeStackedImage& timestack = _plate[x][y];
                for (size_t time = 0; time < timestack.count(); ++time)
                {
                    StackedImage& stack = timestack[time];
                    for (size_t z = 0; z < stack.count(); ++z)
                        return stack.getChannelCount();
                }
            }
    return 0;
}

void WellPlate::storeJson(QJsonObject json)
{
    this->_data = json;
}

void WellPlate::loadFromJSON(QJsonObject data)
{
    Q_UNUSED(data);

    auto pl = data["Data"].toObject();
    for (auto kv = pl.begin(), e = pl.end(); kv != e; ++kv)
    {
        int x = kv.key().toUInt();
        auto yy = kv.value().toObject();
        for (auto kkv = yy.begin(), ke = yy.end(); kkv != ke; ++kkv)
        {
            int y = kkv.key().toUInt();
            auto oo = kkv.value().toObject();
            TimeStackedImage xp;
            xp.loadFromJSON(oo);
            _plate[x][y] = xp;
        }
    }
}

QString WellPlate::basePath(QJsonObject json)
{
    Q_UNUSED(json);
    //    QDir dir(data.first().toString());
    //    return dir.dirName();


    return QString();
}

void WellPlate::deallocate()
{
    for (QMap<size_t, QMap<size_t, TimeStackedImage> >::iterator it = _plate.begin(), e  = _plate.end(); it != e; ++it)
    {
        for (QMap<size_t, TimeStackedImage>::iterator sit = it.value().begin(), se  = it.value().end(); sit != se; ++sit)
            sit.value().deallocate();
    }
}

