#ifndef REGISTRABLEIMAGETYPE_H
#define REGISTRABLEIMAGETYPE_H

//TODO: Add VTK images
//TODO: Add ITK images

#include <QColor>

typedef QMap<int, QColor> Colormap;

#include <QJsonObject>
#include "RegistrableTypes.h"

class RegistrableImageParent : public RegistrableParent
{
    typedef RegistrableImageParent Self;

public:

    enum ContentType { Image, Roi_cv_stats, Roi_rect, Histogram };


    RegistrableImageParent() : _vectorImage(false), _splitted(false), _singleField(false),
        _withMeta(false), _autoload(true), _unbias(0), _tiles(0), _content_type(Image), _channel(-1)
    {
    }


    Self& channelsAsVector(bool splitted = false)
    {
        _vectorImage = true;
        _splitted = splitted;
        return *this;
    }

    Self& singleField()
    {
        _singleField = true;
        return *this;
    }

    void setProperties(const QJsonObject& json, QString prefix = QString())
    {
        if (json.contains("Properties") && json["Properties"].isObject())
        {
            auto ob = json["Properties"].toObject();
            for (auto ks : ob.keys())
            {
                if (!ob[ks].toString().isEmpty())
                {
                    //                    qDebug() << "Property" << prefix << ks << ob[ks].toString();
                    _metaData[prefix + ks] = ob[ks].toString();
                }
            }
        }
    }



    virtual void read(const QJsonObject& json)
    {
        RegistrableParent::read(json);
        setProperties(json);

        if (json.contains("splitted"))
            _splitted = json["splitted"].toBool();

        if (json.contains("singleField"))
            _singleField = json["singleField"].toBool();

        if (json.contains("unbias"))
            _unbias = json["unbias"].toBool();
        if (json.contains("tiled"))
            _tiles = json["tiled"].toInt();


        if (json.contains("ChannelNames"))
        {
            QJsonArray t = json["ChannelNames"].toArray();
            for (int i = 0; i < t.size(); ++i)
                _vectorNames << t[i].toString();
        }
        if (json.contains("Colormap"))
        {
            QJsonObject ob = json["Colormap"].toObject();
            for (auto it = ob.begin(); it != ob.end(); ++it)
            {
                QColor col;
                col.setNamedColor(it.value().toString());
                _colormap[it.key().toInt()] = col;
            }
        }

    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableParent::write(json);
        json["isImage"] = true;
        json["asVectorImage"] = _vectorImage;
        if (_channel > 0)
            json["Channel"] = _channel;

        if (_splitted) json["splitted"] = true;
        if (_singleField)
            json["singleField"] = true;

        json["unbias"] = _unbias; // Tells the client to send the bias field data
        json["tiled"] = (int)_tiles; // Tells the client to send the 8 sided images to be loaded as tiled data !

        if (_vectorNames.size() > 0)
            json["ChannelNames"] = QJsonArray::fromStringList(_vectorNames);
        if (_withMeta)
            json["Properties"] = QJsonArray::fromStringList(_meta);

        std::vector<QString> map = { "Image", "Roi_cv_stats", "Roi_rect", "Histogram" };
        json["ContentType"] = map[_content_type];

        if (_colormap.size() > 0)
        {
            QJsonObject ob;

            for (auto it = _colormap.begin(); it != _colormap.end(); ++it)
            {
                ob[QString("%1").arg(it.key())] = it.value().name();
            }

            json["Colormap"] = ob;
        }


    }

    Self& setChannel(int channel)
    {
        _channel = channel;
        return *this;
    }

    void setBiasFiles(QStringList bias)
    {
        _bias_files = bias;
    }

    QStringList getBiasFiles()
    {
        return _bias_files;
    }

    virtual QString toString() const
    {
        return QString("Image results");
    }

    virtual void loadImage(QJsonObject json) = 0;
    virtual QString basePath(QJsonObject json) = 0;

    virtual void freeImage() = 0;

    Self& withProperties(QStringList d)
    {
        _withMeta = true;
        _meta = d;
        return *this;
    }


    virtual void storeJson(QJsonObject json) = 0;
    virtual void applyBiasField(cv::Mat bias) = 0;

    Self& noImageAutoLoading(bool state = false) { _autoload = state; return *this; }
    bool imageAutoloading() { return _autoload; }


    QString getMeta(QString me, bool debug = false)
    {
        if (debug)
            for (auto i = _metaData.begin(), e = _metaData.end(); i != e; ++i)
                qDebug() << i.key() << i.value();

        return _metaData[me];
    }

    // Unbias on load
    Self& unbias()
    { // Apply the bias correction to the loaded
        _unbias = true;
        return *this;
    }

    bool shallUnbias()
    {
        return _unbias;
    }

    Self& loadTiled(unsigned size)
    {
        _tiles = size;
        return *this;
    }

    virtual bool hasData(void* data) = 0;


    Self& setColormap(Colormap color)
    {
        _colormap = color;
        return *this;
    }

    Self& setChannelNames(QStringList names)
    {
        _vectorNames = names;
        return *this;
    }

    QStringList getChannelNames()
    {
        return _vectorNames;
    }

    RegistrableImageParent& setContentType(ContentType t)
    {
        _content_type = t;
        return *this;
    }


    ContentType contentType()
    {
        return _content_type;
    }


protected:
    Colormap _colormap;

    bool        _vectorImage, _splitted, _singleField;
    bool        _withMeta;
    bool        _autoload;
    bool        _unbias;
    unsigned        _tiles;
    ContentType _content_type;
    int         _channel;

    QStringList _meta;
    QStringList _vectorNames;
    QMap<QString, QString> _metaData;
    QStringList _bias_files;
    QString base_path;
};

#include <QThread>


template <>
class Registrable<cv::Mat> : public RegistrableImageParent
{
public:
    typedef Registrable<cv::Mat> Self;
    typedef cv::Mat DataType;


    Registrable() : RegistrableImageParent(), _value(0)
    {
    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }
    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        //    if (json.contains("asVectorImage"))
        //      _vectorImage = json["asVectorImage"].toBool();
        setProperties(json);


        if (_vectorNames.size() == 0 && json.contains("ChannelNames"))
        {
            QJsonArray t = json["ChannelNames"].toArray();
            for (int i = 0; i < t.size(); ++i)
                _vectorNames << t[i].toString().simplified();
        }
    }

    void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("ImageContainer");

        if (!_value)
        { //qDebug() << "Empty image !!!!";
            return;
        }
        //      qDebug() << "Finishing Registered images" << _value->rows << _value->cols  << _value->channels();
        //        _value.
        if (this->_isProduct && this->_isFinished)
        {
//            if (this->_isOptional && this->_optionalDefault != true)
//                return;

            std::vector<cv::Mat> split;
            if (_value->channels() == 1) split.push_back(*_value);
            else       cv::split(*_value, split);


            if (split.size() == 0)
            {
                qDebug() << "OpenCV Split returned empty image... leaving..";
                return;
            }

            QJsonObject ob;

            if (_value->rows == 0 && _value->cols == 0)
            {
                ob["DataTypeSize"] = 0;
                ob["cvType"] = 0;
                ob["Rows"] = 0;
                ob["Cols"] = 0;
            }
            else
            {

                ob["DataTypeSize"] = (int)split[0].elemSize();
                ob["cvType"] = split[0].type();
                ob["Rows"] = split[0].rows;
                ob["Cols"] = split[0].cols;
                ob["DataHash"] = getHash();
                //          qDebug() << ob;

                unsigned long long len = split[0].elemSize() * split[0].rows * split[0].cols;

                std::vector<unsigned char> r(split.size() * len);
                auto iter = r.begin();
                QJsonArray d;
                for (int i = 0; i < split.size(); ++i)
                {
                    unsigned char* p = split[i].ptr<  unsigned char>(0);
                    for (int i = 0; i < len; ++i, ++p, ++iter)
                        *iter = *p;
                    d.append(QJsonValue((int)len));//a.size());
                }
                RegistrableParent::attachPayload(getHash(), r);

                ob["DataSizes"] = d;
                auto ar = QJsonArray();
                ar.push_back(ob);
                json["Payload"] = ar;
            }

            (*_value).release();

        }

    }

    virtual void storeJson(QJsonObject json)
    {
    }


    virtual void loadImage(QJsonObject json)
    {
        cocvMat::loadFromJSON(json, *_value);
    }


    virtual void applyBiasField(cv::Mat bias)
    {
        DataType& im = *_value;
        cv::Mat tmp;
        cv::divide(im, bias, tmp);
        cv::swap(tmp, *_value);
    }

    virtual QString basePath(QJsonObject json)
    {
        QDir dir(json["Data"].toArray().first().toString());
        return dir.dirName();
    }

    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        _value->copyTo(*data);

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }


    virtual bool hasData(void* data) override
    {
        return _value == data;
    }


protected:

    DataType* _value;

};

template <>
class Registrable<ImageContainer> : public RegistrableImageParent
{
public:
    typedef Registrable<ImageContainer> Self;
    typedef ImageContainer DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        setProperties(json);

        if (_vectorNames.size() == 0 && json.contains("ChannelNames"))
        {
            QJsonArray t = json["ChannelNames"].toArray();
            for (int i = 0; i < t.size(); ++i)
                _vectorNames << t[i].toString();
        }
    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("ImageContainer");

        if (!_value || _value->count() == 0)
        { //qDebug() << "Empty image !!!!";
            return;
        }
        //      qDebug() << "Finishing Registered images" << _value->rows << _value->cols  << _value->channels();
        //        _value.
        if (this->_isProduct && this->_isFinished)
        {
//            if (this->_isOptional && this->_optionalDefault != true)
//                return;
            auto ar = QJsonArray();

            for (size_t item = 0; item < _value->count(); ++item)
            {
                std::vector<cv::Mat> split;
                if ((*_value)(item).channels() == 1) split.push_back((*_value)(item));
                else       cv::split((*_value)(item), split);


                if (split.size() == 0)
                {
                    qDebug() << "OpenCV Split returned empty image... skipping..";
                    continue;
                }
                if (split[0].rows == 0 && split[0].cols == 0)
                    continue;

                QJsonObject ob;

                ob["DataTypeSize"] = (int)split[0].elemSize();
                ob["cvType"] = split[0].type();
                ob["Rows"] = split[0].rows;
                ob["Cols"] = split[0].cols;
                QString local_hash = getHash() + QString("%1").arg(item);
                if (item != 0)
                    ob["DataHash"] = local_hash;
                //          qDebug() << ob;

                unsigned long long len = split[0].elemSize() * split[0].rows * split[0].cols;

                //          QJsonArray arr;
                //                    qDebug() << "Writing image data to socket"
                //                             << split[0].elemSize()
                //                             << split[0].type()
                //                             << split[0].rows
                //                             << split[0].cols
                //                             << len
                //                             << split.size();
                //          QByteArray r;

                std::vector<unsigned char> r(split.size() * len);
                auto iter = r.begin();
                QJsonArray d;
                for (size_t i = 0; i < split.size(); ++i)
                {
                    unsigned char* p = split[i].ptr<  unsigned char>(0);
                    for (int i = 0; i < len; ++i, ++p, ++iter)
                        *iter = *p;
                    d.append(QJsonValue((int)len));//a.size());
                }

                RegistrableParent::attachPayload(local_hash, r, item);
                ob["DataSizes"] = d;
                //qDebug() << d;
                ar.push_back(ob);
            }

            json["Payload"] = ar;
            //          qDebug() << json;

        }

    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }


    virtual void applyBiasField(cv::Mat bias)
    {
        DataType& im = *_value;

        for (size_t i = 0; i < im.count(); ++i)
        {
            cv::Mat tmp;
            cv::divide(im[i], bias, tmp);
            cv::swap(tmp, im[i]);
        }
    }


    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }

    virtual void storeJson(QJsonObject json)
    {

        _value->storeJson(json);
    }


    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};



template <>
class Registrable<TimeImage> : public RegistrableImageParent
{
public:
    typedef Registrable<TimeImage> Self;
    typedef TimeImage DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {
    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        setProperties(json);


        QJsonArray data = json["Data"].toArray();
        for (int i = 0; i < data.size(); i++)
        {
            auto d = data.at(i).toObject();
            setProperties(d, QString("t%1").arg(i));

            if (_vectorNames.size() == 0 && d.contains("ChannelNames"))
            {
                QJsonArray t = d["ChannelNames"].toArray();
                for (int i = 0; i < t.size(); ++i)
                    _vectorNames << t[i].toString();
            }
        }

        if (_vectorNames.size() == 0 && json.contains("ChannelNames"))
        {
            QJsonArray t = json["ChannelNames"].toArray();
            for (int i = 0; i < t.size(); ++i)
                _vectorNames << t[i].toString();
        }


        //    if (json.contains("asVectorImage"))
        //      _vectorImage = json["asVectorImage"].toBool();
    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("TimeImage");

        //        qDebug() << "Should store TimeImage" << _value->count();

        if (!_value || _value->count() == 0)
        { //qDebug() << "Empty image !!!!";
            return;
        }
        //      qDebug() << "Finishing Registered images" << _value->rows << _value->cols  << _value->channels();
        //        _value.
        if (this->_isProduct && this->_isFinished)
        {
//            if (this->_isOptional && this->_optionalDefault != true)
//                return;
            auto ar = QJsonArray();

            for (size_t item = 0; item < _value->count(); ++item)
            {
                std::vector<cv::Mat> split;
                if ((*_value)(item).channels() == 1) split.push_back((*_value)(item));
                else       cv::split((*_value)(item), split);


                if (split.size() == 0)
                {
                    qDebug() << "OpenCV Split returned empty image... skipping..";
                    continue;
                }
                if (split[0].rows == 0 && split[0].cols == 0)
                    continue;

                QJsonObject ob;

                ob["DataTypeSize"] = (int)split[0].elemSize();
                ob["cvType"] = split[0].type();
                ob["Rows"] = split[0].rows;
                ob["Cols"] = split[0].cols;
                ob["DataHash"] = getHash() + QString("%1").arg(item);
                ob["Time"] = (int)item; // Outputs a time id, this is a time serie image output
                //          qDebug() << ob;

                unsigned long long len = split[0].elemSize() * split[0].rows * split[0].cols;

                //          QJsonArray arr;
                //                    qDebug() << "Writing image data to socket"
                //                             << split[0].elemSize()
                //                             << split[0].type()
                //                             << split[0].rows
                //                             << split[0].cols
                //                             << len
                //                             << split.size();
                //          QByteArray r;

                std::vector<unsigned char> r(split.size() * len);
                auto iter = r.begin();
                QJsonArray d;
                for (int i = 0; i < split.size(); ++i)
                {
                    unsigned char* p = split[i].ptr<  unsigned char>(0);
                    for (int i = 0; i < len; ++i, ++p, ++iter)
                        *iter = *p;
                    d.append(QJsonValue((int)len));//a.size());
                }
                //          qDebug() << "Writing image data to done" ;
                //          ob["Data"]=arr;
                RegistrableParent::attachPayload(r, item);
                ob["DataSizes"] = d;
                //qDebug() << d;
                ar.push_back(ob);
            }

            json["Payload"] = ar;
            qDebug() << json;

        }


    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }
    virtual void applyBiasField(cv::Mat bias)
    {
        DataType& im = *_value;

        for (size_t i = 0; i < im.count(); ++i)
        {
            cv::Mat tmp;
            cv::divide(im[i], bias, tmp);
            cv::swap(tmp, im[i]);
        }
    }
    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }


    virtual void storeJson(QJsonObject json)
    {

        _value->storeJson(json);
    }



    virtual void freeImage()
    {
        _value->deallocate();
    }
    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};


template <>
class Registrable<StackedImage> : public RegistrableImageParent
{
public:
    typedef Registrable<StackedImage> Self;
    typedef StackedImage DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        setProperties(json);

        _value->storeJson(json);

        if (_vectorNames.size() == 0 && json.contains("ChannelNames"))
        {
            QJsonArray t = json["ChannelNames"].toArray();
            for (int i = 0; i < t.size(); ++i)
                _vectorNames << t[i].toString();
        }


    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("StackedImage");
    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }
    virtual void applyBiasField(cv::Mat bias)
    {
        DataType& im = *_value;

        for (size_t i = 0; i < im.count(); ++i)
        {
            cv::Mat tmp;
            cv::divide(im[i], bias, tmp);
            cv::swap(tmp, im[i]);
        }
    }
    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }

    virtual void storeJson(QJsonObject json)
    {

        _value->storeJson(json);
    }

    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};

template <>
class Registrable<TimeStackedImage> : public RegistrableImageParent
{
public:
    typedef Registrable<TimeStackedImage> Self;
    typedef TimeStackedImage DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        setProperties(json);

        _value->storeJson(json);

        auto d = json["Data"].toArray();

        for (int i = 0; i < d.size(); ++i)
        {
            QJsonObject oo = d.at(i).toObject();
            StackedImage& xp = _value->addOne();
            Registrable<StackedImage> reg;
            reg.setValuePointer(&xp);
            reg.read(oo);
        }
        if (_vectorNames.size() == 0 && json.contains("ChannelNames"))
        {
            QJsonArray t = json["ChannelNames"].toArray();
            for (int i = 0; i < t.size(); ++i)
                _vectorNames << t[i].toString();
        }

    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("TimeStackedImage");
    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }

    virtual void applyBiasField(cv::Mat bias)
    {
        DataType& time = *_value;

        for (size_t t = 0; t < time.count(); ++t)
        {
            StackedImage& im = time[t];
            for (size_t i = 0; i < im.count(); ++i)
            {
                cv::Mat tmp;
                cv::divide(im[i], bias, tmp);
                cv::swap(tmp, im[i]);
            }
        }
    }
    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }



    virtual void storeJson(QJsonObject json)
    {

        _value->storeJson(json);
    }


    virtual void freeImage()
    {
        _value->deallocate();
    }


    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};

template <>
class Registrable<ImageXP> : public RegistrableImageParent
{
public:
    typedef Registrable<ImageXP> Self;
    typedef ImageXP DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        // Need to find out the metadata!!!
        QJsonArray data = json["Data"].toArray();


        for (int i = 0; i < data.size(); i++)
        {
            auto d = data.at(i).toObject();
            setProperties(d, QString("f%1").arg(i));

            if (_vectorNames.size() == 0 && d.contains("ChannelNames"))
            {
                QJsonArray t = d["ChannelNames"].toArray();
                for (int i = 0; i < t.size(); ++i)
                    _vectorNames << t[i].toString();
            }
        }




    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("ImageXP");
    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }

    virtual void applyBiasField(cv::Mat bias)
    {
        DataType& im = *_value;

        for (size_t i = 0; i < im.count(); ++i)
        {
            cv::Mat tmp;
            cv::divide(im[i], bias, tmp);
            cv::swap(tmp, im[i]);
        }
    }

    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }

    virtual void storeJson(QJsonObject json)
    {

        _value->storeJson(json);
    }


    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};

template <>
class Registrable<TimeImageXP> : public RegistrableImageParent
{
public:
    typedef Registrable<TimeImageXP> Self;
    typedef TimeImageXP DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        auto data = json["Data"].toArray();
        for (int i = 0; i < data.size(); i++)
        {
            auto d = data.at(i).toObject();
            setProperties(d, QString("f%1").arg(i));

            if (_vectorNames.size() == 0 && d.contains("ChannelNames"))
            {
                QJsonArray t = d["ChannelNames"].toArray();
                for (int i = 0; i < t.size(); ++i)
                    _vectorNames << t[i].toString();
            }
            if (data.contains("Data"))
            {
                auto sdata = d["Data"].toArray();
                for (int j = 0; j < sdata.size(); ++j)
                {
                    auto d = sdata.at(j).toObject();
                    setProperties(d, QString("f%1t%2").arg(i).arg(j));
                }
            }
        }
    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("TimeImageXP");
    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }

    virtual void applyBiasField(cv::Mat bias)
    {
        //        DataType& time = *_value;

        //        for (size_t t = 0; t < im.count(); ++t)
        //        {
        //            StackedImage& im = time[t];
        //            for (size_t i = 0; i < im.count(); ++i)
        //            {
        //                cv::Mat tmp;
        //                cv::divide(im[i], bias, tmp);
        //                cv::swap(tmp, *_value);
        //            }
        //        }

        qDebug() << "Not implemented yet !!!";

    }

    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }

    virtual void storeJson(QJsonObject json)
    {

        _value->storeJson(json);
    }


    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};

template <>
class Registrable<StackedImageXP> : public RegistrableImageParent
{
public:
    typedef Registrable<StackedImageXP> Self;
    typedef StackedImageXP DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }



    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        setProperties(json);
        _value->storeJson(json);

        auto data = json["Data"].toArray();
        for (int f = 0; f < data.size(); f++)
        {
            auto d = data.at(f).toObject();
            setProperties(d, QString("f%1").arg(f));

            auto sdata = d["Data"].toArray();
            {
                for (int z = 0; z < sdata.size(); z++)
                {

                    auto d = sdata.at(z).toObject();
                    if (z!=0)
                        setProperties(d, QString("f%1z%2").arg(f).arg(z));
                    else
                        setProperties(d, QString("f%1").arg(f));
                    if (_vectorNames.size() == 0 && d.contains("ChannelNames"))
                    {
                        QJsonArray t = d["ChannelNames"].toArray();
                        for (int i = 0; i < t.size(); ++i)
                            _vectorNames << t[i].toString();
                    }

                }
            }
        }
    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("StackedImageXP");
    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }

    virtual void applyBiasField(cv::Mat bias)
    {
        //        DataType& time = *_value;

        //        for (size_t t = 0; t < im.count(); ++t)
        //        {
        //            StackedImage& im = time[t];
        //            for (size_t i = 0; i < im.count(); ++i)
        //            {
        //                cv::Mat tmp;
        //                cv::divide(im[i], bias, tmp);
        //                cv::swap(tmp, *_value);
        //            }
        //        }

        qDebug() << "Not implemented yet !!!";

    }


    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }

    virtual void storeJson(QJsonObject json)
    {

        _value->storeJson(json);
    }


    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};

template <>
class Registrable<TimeStackedImageXP> : public RegistrableImageParent
{
public:
    typedef Registrable<TimeStackedImageXP> Self;
    typedef TimeStackedImageXP DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        setProperties(json);

        auto data = json["Data"].toArray();
        for (int i = 0; i < data.size(); i++)
        {
            auto d = data.at(i).toObject();
            setProperties(d, QString("f%1").arg(i));

            TimeStackedImage& tsi = _value->addOne();
            Registrable<TimeStackedImage> reg;
            reg.setValuePointer(&tsi);
            reg.read(d);


            if (_vectorNames.size() == 0 && d.contains("ChannelNames"))
            {
                QJsonArray t = d["ChannelNames"].toArray();
                for (int i = 0; i < t.size(); ++i)
                    _vectorNames << t[i].toString();
            }

        }
    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["ImageType"] = QString("TimeStackedImageXP");
    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }

    virtual void applyBiasField(cv::Mat bias)
    {
        //        DataType& time = *_value;

        //        for (size_t t = 0; t < im.count(); ++t)
        //        {
        //            StackedImage& im = time[t];
        //            for (size_t i = 0; i < im.count(); ++i)
        //            {
        //                cv::Mat tmp;
        //                cv::divide(im[i], bias, tmp);
        //                cv::swap(tmp, *_value);
        //            }
        //        }

        qDebug() << "Not implemented yet !!!";

    }


    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }

    virtual void storeJson(QJsonObject json)
    {

        _value->storeJson(json);
    }


    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};

template <>
class Registrable<WellPlate> : public RegistrableImageParent
{
public:
    typedef Registrable<WellPlate> Self;
    typedef WellPlate DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        setProperties(json);
        //        qDebug() << "DDD" << json;
        auto pl = json["Data"].toObject();

        this->_hash=json["DataHash"].toString();;
        bool ok;
        //      qDebug() << pl ;
        for (auto kv = pl.begin(), e = pl.end(); kv != e; ++kv)
        {
            int x = kv.key().toUInt(&ok); if (!ok) continue;

            auto yy = kv.value().toObject();
            for (auto kkv = yy.begin(), ke = yy.end(); kkv != ke; ++kkv)
            {
                int y = kkv.key().toUInt(&ok); if (!ok) continue;

                auto oo = kkv.value().toObject();
                oo["BasePath"] = json["BasePath"].toString();
                qDebug() << oo;
                TimeStackedImage& xp = _value->addOne(x,y);
                Registrable<TimeStackedImage> reg;
                reg.setValuePointer(&xp);
                //                 qDebug() << x << y << oo["Data"];
                reg.read(oo);
                //               _plate[x][y] = xp;
            }

        }
    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["DataHash"]=this->_hash;
        json["ImageType"] = QString("WellPlate");



    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }
    virtual void applyBiasField(cv::Mat bias)
    {
        //        DataType& time = *_value;

        //        for (size_t t = 0; t < im.count(); ++t)
        //        {
        //            StackedImage& im = time[t];
        //            for (size_t i = 0; i < im.count(); ++i)
        //            {
        //                cv::Mat tmp;
        //                cv::divide(im[i], bias, tmp);
        //                cv::swap(tmp, *_value);
        //            }
        //        }

        qDebug() << "Not implemented yet !!!";

    }

    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }

    virtual void storeJson(QJsonObject json)
    {

        return _value->storeJson(json);
    }


    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};


template <>
class Registrable<WellPlateXP> : public RegistrableImageParent
{
public:
    typedef Registrable<WellPlateXP> Self;
    typedef WellPlateXP DataType;


    Registrable() : RegistrableImageParent(), _value(nullptr)
    {

    }


    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType* v)
    {
        _value = v;
        return *this;

    }

    virtual void read(const QJsonObject& json)
    {
        RegistrableImageParent::read(json);
        setProperties(json);
        //        qDebug() << "DDD" << json;
        auto pl = json["Data"].toObject();

        this->_hash=json["DataHash"].toString();;
        bool ok;
        //      qDebug() << pl ;
        for (auto kv = pl.begin(), e = pl.end(); kv != e; ++kv)
        {
            int x = kv.key().toUInt(&ok); if (!ok) continue;

            auto yy = kv.value().toObject();
            for (auto kkv = yy.begin(), ke = yy.end(); kkv != ke; ++kkv)
            {
                int y = kkv.key().toUInt(&ok); if (!ok) continue;

                auto oo = kkv.value().toObject();
                oo["BasePath"] = json["BasePath"];

                TimeStackedImageXP& xp = _value->addOne(x,y);
                Registrable<TimeStackedImageXP> reg;
                reg.setValuePointer(&xp);
                //                 qDebug() << x << y << oo["Data"];
                reg.read(oo);
                //               _plate[x][y] = xp;
            }

        }
    }

    virtual void write(QJsonObject& json) const
    {
        RegistrableImageParent::write(json);
        json["DataHash"]=this->_hash;
        json["ImageType"] = QString("WellPlateXP");



    }

    virtual void loadImage(QJsonObject json)
    {
        _value->loadFromJSON(json);
    }
    virtual void applyBiasField(cv::Mat bias)
    {
        //        DataType& time = *_value;

        //        for (size_t t = 0; t < im.count(); ++t)
        //        {
        //            StackedImage& im = time[t];
        //            for (size_t i = 0; i < im.count(); ++i)
        //            {
        //                cv::Mat tmp;
        //                cv::divide(im[i], bias, tmp);
        //                cv::swap(tmp, *_value);
        //            }
        //        }

        qDebug() << "Not implemented yet !!!";

    }

    virtual QString basePath(QJsonObject json)
    {
        return _value->basePath(json);
    }

    virtual void storeJson(QJsonObject json)
    {

        return _value->storeJson(json);
    }


    virtual void freeImage()
    {
        _value->deallocate();
    }

    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        *data = *_value;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    virtual bool hasData(void* data) override
    {
        return _value == data;
    }

protected:

    DataType* _value;
};



#endif // REGISTRABLEIMAGETYPE_H
