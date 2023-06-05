#ifndef REGISTRABLETYPES_H
#define REGISTRABLETYPES_H

#include <QtCore>
#include <typeinfo>

class DllPluginManagerExport RegistrableParent
{

public:
    typedef RegistrableParent Self;

    enum Level
    {
        Basic,
        Advanced,
        VeryAdvanced,
        Debug
    };
    enum AggregationType
    {
        Sum,
        Mean,
        Median,
        Min,
        Max
    };

    enum DevectorizeType
    {
        None,
        Time,
        Field,
        Stack,
        Channel
    };

    RegistrableParent() : _position(-1), _wasSet(false), _level(Basic), _isProduct(false),
                          _isOptional(false), _isFinished(false), _isPerChannel(false), _isSync(false),
                          _keepInMem(false), _optionalDefault(false), _duped(false), _hidden(false), _isbool(false)
    {
    }

    // set Tag & set Comment shall not be exposed to the end user

    RegistrableParent &setTag(QString tag)
    {
        _tag = tag;
        return *this;
    }

    RegistrableParent &setComment(QString com)
    {
        _comment = com;
        return *this;
    }

    virtual QString toString() const = 0;

    void setAsProduct()
    {
        _isProduct = true;
    }

public:
    RegistrableParent &setPosition(int pos)
    {
        _position = pos;
        return *this;
    }

    RegistrableParent &setLevel(Level lvl)
    {
        _level = lvl;
        return *this;
    }


    RegistrableParent & setAsBool(bool state=true)
    {
        _isbool = state;
        return *this;
    }

    bool isSet() const
    {
        return _wasSet;
    }

    virtual void read(const QJsonObject &json)
    {
        //      qDebug() << json;
        if (json.contains("isOptional"))
        {
            //          qDebug() << "Setting optional state" << json;
            _isOptional = true;
            _optionalDefault = json["optionalState"].toBool();
        }
        else
            _isOptional = false;

        if (json.contains("isBool"))
            _isbool = json["isBool"].toBool();

        if (json.contains("DataHash"))
            _hash = json["DataHash"].toString();

        if (json.contains("PerChannelParameter"))
            _isPerChannel = json["PerChannelParameter"].toBool();

        if (json.contains("hidden"))
            _hidden = json["hidden"].toBool();

        //        _tag = json["Tag"].toString();
        //        _comment = json["Comment"].toString();
        //        _position = json["Position"].toInt();
        //        QString lvl = json["Level"].toString();
        //        if (lvl == "Basic") _level = Basic;
        //        if (lvl == "Advanced") _level = Advanced;
        //        if (lvl == "VeryAdvanced") _level = VeryAdvanced;
        //        if (lvl == "Debug") _level = Debug;
        //
    }

    Self &setIf(QString tag, int value)
    {
        _enableIf << qMakePair(tag, value);
        return *this;
    }

    virtual void write(QJsonObject &json) const
    {
        json["Tag"] = _tag;
        json["Comment"] = _comment;
        json["Position"] = _position;
        json["PerChannelParameter"] = _isPerChannel;
        json["IsSync"] = _isSync;
        json["DataHash"] = _hash;
        json["Level"] = QString(_level == Basic ? "Basic" : _level == Advanced   ? "Advanced"
                                                        : _level == VeryAdvanced ? "VeryAdvanced"
                                                                                 : "Debug");

        json["isSlider"] = false;
        if (_isbool)
            json["isBool"] = true;

        if (!_enableIf.isEmpty())
        {
            QJsonArray en;
            QPair<QString, int> p;
            foreach (p, _enableIf)
            {
                QJsonObject ob;
                ob[p.first] = p.second;
                en.push_back(ob);
            }
            json["enableIf"] = en;
        }

        if (_hidden)
        {
            json["hidden"] = true;
        }
        if (!_path.isEmpty())
            json["SavePath"] = _path;

        if (_isOptional)
        {
            json["isOptional"] = _isOptional;
            json["optionalState"] = _optionalDefault;
        }

        if (!_group.isEmpty())
        {
            json["group"] = QString(_group);
        }
    }

    QString tag() const
    {
        return _tag;
    }

    Self &setGroup(QString g)
    {
        _group = g;
        return *this;
    }

    QString getGroup()
    {
        return _group;
    }

    Self &setAsOptional(bool val)
    {
        _isOptional = true;
        _optionalDefault = val;
        return *this;
    }

    Self &setFinished()
    {
        _isFinished = true;
        return *this;
    }

    // Use this function to tell the plugin that this is a "per channel" option
    Self &setPerChannel(bool val = true)
    {
        _isPerChannel = val;
        return *this;
    }

    Self &perChannels()
    {
        _isPerChannel = true;
        return *this;
    }

    Self &setSync(bool val = true)
    {
        _isSync = val;
        return *this;
    }

    Self &isHidden(bool val = true)
    {
        _hidden = val;
        return *this;
    }

    Self &setPath(QString p)
    {
        _path = p;
        return *this;
    }

    //    virtual void clone()  = 0;
    virtual RegistrableParent *dup() = 0;
    void setHash(QString s) { _hash = s; /*qDebug() << "Data Hash" << _tag << _hash; */ }
    QString getHash() const { return _hash; }

    void keepInMem(bool mem) { _keepInMem = mem; }

    //    void Q_DECL_DEPRECATED attachPayload(QByteArray arr) const;
    //    void Q_DECL_DEPRECATED attachPayload(QString hash, QByteArray arr) const;

    void attachPayload(std::vector<unsigned char> arr, size_t pos = 0) const;
    void attachPayload(QString hash, std::vector<unsigned char> arr, size_t pos = 0) const;

    QString getPath()
    {
        return _path;
    }

protected:
    QString _tag, _comment, _hash;
    int _position;
    bool _wasSet;
    Level _level;
    bool _isProduct, _isFinished;
    bool _isOptional, _optionalDefault;
    bool _isPerChannel, _isSync;
    QList<QPair<QString, int>> _enableIf;
    QString _group;
    bool _keepInMem;
    bool _duped;
    bool _hidden;
    bool _isbool;
    QString _path;
};

template <class Type>
class Registrable : public RegistrableParent
{
public:
    typedef Registrable<Type> Self;
    typedef Type DataType;

    Registrable() : _value(0)
    {
    }

    ~Registrable()
    {
        if (_duped && _value)
            delete _value;
    }

    // Value & setValuePointer shall not be exposed to the user
    DataType &value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self &setValuePointer(DataType *v)
    {
        if (_value && _duped)
        {
            delete _value;
            _duped = false;
        }

        _value = v;
        if (this->_hasDefault)
            (*_value) = this->_default;

        return *this;
    }

    Self &setDefault(DataType d1)
    {
        this->_hasDefault = true;
        this->_default = d1;

        (*_value) = d1;

        return *this;
    }

    Self &setRange(DataType mi, DataType ma)
    {
        this->_hasRange = true;

        this->_minRange = mi;
        this->_maxRange = ma;

        return *this;
    }




    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);

        if (this->_hasDefault)
        {
            json["Default"] = tojson(this->_default);
        }

        json["Range/Set"] = this->_hasRange;
        json["Range/Low"] = tojson(this->_minRange);
        json["Range/High"] = tojson(this->_maxRange);
    }





    virtual QString toString() const
    {
        // if the following code does not compile,
        // it means that your are trying to Register an unsuported type
        // Please ask for datatype support to : wiest.daessle@gmail.com
        return QString("%1").arg(*_value);
    }

    virtual RegistrableParent *dup()
    {
        _duped = true;
        DataType *data = new DataType();
        *data = *_value;

        Self *s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

protected:
    DataType *_value;
};

template <class T>
inline QString tostr(T &val)
{
    return QString("%1").arg(val);
}
template <>
inline QString tostr(std::vector<int> &val)
{
    QString r;
    for (auto i : val)
        r = QString("%1,%2").arg(r).arg(i);
    return r;
}

namespace RegPair_details
{

    template <class T>
    inline void setValue(T *val, QString v, const QJsonObject &json)
    {
        if (json[v].isArray())
            *val = json[v].toArray().at(0).toInt();
        else
            *val = json[v].toInt();
    }

    template <>
    inline void setValue(std::vector<int> *val, QString v, const QJsonObject &json)
    {
        (*val).clear();
        if (!json.contains(v))
            return;
        if (json[v].isArray())
        { // Might need magick c++ tricks for type dispatch :(
            QJsonArray ar = json[v].toArray();
            for (int i = 0; i < ar.size(); ++i)
                (*val).push_back(ar.at(i).toInt());
        }
        else
            (*val).push_back(json[v].toInt());
    }

    template <typename T>
    inline QJsonValue tojson(const T &def)
    {
        static_assert(sizeof(T) == -1, "You have to have a specialization for to json");
        return QJsonValue();
    }
    template <>
    inline QJsonValue tojson(const int &def) { return QJsonValue((int)def); }
    template <>
    inline QJsonValue tojson(const double &def) { return QJsonValue((double)def); }
    template <>
    inline QJsonValue tojson(const float &def) { return QJsonValue((double)def); }

    template <>
    inline QJsonValue tojson(const std::vector<int> &vals)
    {
        QJsonArray res;
        for (int i = 0; i < vals.size(); ++i)
            res.push_back(vals[i]);
        return res;
    }

}

template <class Type>
class RegistrablePair : public RegistrableParent
{
public:
    typedef RegistrablePair<Type> Self;
    typedef Type DataType;

    RegistrablePair() : _hasDefault(false), _value1(0), _value2(0)
    {
    }

    ~RegistrablePair()
    {
        if (_duped && _value1)
            delete _value1;
        if (_duped && _value2)
            delete _value2;
    }
    // Value & setValuePointer shall not be exposed to the user
    DataType &value()
    {
        return *_value1;
    }

    void setValue(DataType t, DataType t2)
    {
        _wasSet = true;
        *_value1 = t;
        *_value2 = t;
    }

    Self &setValuePointer(DataType *v1, DataType *v2)
    {

        if (_value1 && _duped)
        {
            delete _value1;
            _duped = false;
        }
        if (_value2 && _duped)
        {
            delete _value2;
            _duped = false;
        }

        _value1 = v1;
        if (_hasDefault)
            (*_value1) = _default1;

        _value2 = v2;
        if (_hasDefault)
            (*_value1) = _default2;

        return *this;
    }

    virtual QString toString() const
    {
        // if the following code does not compile,
        // it means that your are trying to Register an unsuported type
        // Please ask for datatype support to : wiest.daessle@gmail.com
        return QString("%1;%2").arg(tostr(*_value1)).arg(tostr(*_value2));
    }

    Self &setComments(QString s1, QString s2)
    {
        _comment = s1;
        _comment2 = s2;

        return *this;
    }

    template <class T>
    void setValue(T *val, QString v, const QJsonObject &json)
    {
        RegPair_details::setValue(val, v, json);
    }

    virtual void read(const QJsonObject &json)
    {
        //        qDebug() << "Reading value: " << json["Value"]
        //                 << json["Value2"];
        RegistrableParent::read(json);

        if (_hasDefault)
        {
            (*_value1) = _default1;
            (*_value2) = _default2;
        }

        setValue(_value1, "Value", json);
        setValue(_value2, "Value2", json);
    }

    virtual RegistrableParent *dup()
    {
        _duped = true;
        DataType *data1 = new DataType();
        DataType *data2 = new DataType();
        *data1 = *_value1;
        *data2 = *_value2;

        Self *s = new Self();
        s->setValuePointer(data1, data2);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    template <typename T>
    inline QJsonValue tojson(const T &def) const
    {
        return RegPair_details::tojson(def);
    }

    virtual void write(QJsonObject &json) const
    {

        RegistrableParent::write(json);

        json["Tag"] = _tag;
        json["Comment"] = _comment;
        json["Comment2"] = _comment2;

        if (std::numeric_limits<DataType>::has_infinity)
            json["isIntegral"] = false;
        else
            json["isIntegral"] = true;

        if (_isOptional)
        {
            json["isOptional"] = _isOptional;
            json["optionalState"] = _optionalDefault;
        }

        if (_hasDefault)
        {
            json["Default"] = tojson(_default1);
            json["Default2"] = tojson(_default2);
        }

        json["Range/Set"] = _hasRange;
        json["Range/Low"] = tojson(_minRange);
        json["Range/High"] = tojson(_maxRange);
    }

    Self &setDefault(DataType d1, DataType d2)
    {
        _hasDefault = true;
        _default1 = d1;
        _default2 = d2;

        (*_value1) = d1;
        (*_value2) = d2;

        return *this;
    }

    Self &setRange(DataType mi, DataType ma)
    {
        _hasRange = true;

        _minRange = mi;
        _maxRange = ma;

        return *this;
    }

protected:
    DataType *_value1;
    DataType *_value2;

    QString _comment2;

    bool _hasDefault;
    DataType _default1, _default2;

    bool _hasRange;
    DataType _minRange, _maxRange;
};

template <typename Type>
class RegistrableEnum : public RegistrableParent
{

public:
    typedef RegistrableEnum<Type> Self;
    typedef Type DataType;

    RegistrableEnum() : _value(0)
    {
    }

    ~RegistrableEnum()
    {
        if (_duped && _value)
            delete _value;
    }

    DataType &value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self &setValuePointer(DataType *v)
    {
        if (_value && _duped)
        {
            delete _value;
            _duped = false;
        }

        _value = v;
        if (_hasDefault)
            (*_value) = _default;

        return *this;
    }

    Self &setEnum(QStringList val)
    {
        _enum = val;
        return *this;
    }

    Self &setDefault(DataType v)
    {
        _hasDefault = true;
        _default = v;
        (*_value) = v;
        return *this;
    }

    virtual QString toString() const
    {
        return _enum.at(*_value);
    }

    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);
        if (_hasDefault)
            (*_value) = _default;

        if (json.contains("Value"))
        {
            int val = 0;
            QString vals = json["Value"].isArray() ? json["Value"].toArray().at(0).toString() : json["Value"].toString();
            for (int i = 0; i < _enum.size(); ++i)
                if (vals == _enum[i])
                    val = i;
            *_value = (DataType)val;
        }
    }

    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);
        json["Enum"] = QJsonArray::fromStringList(_enum);
    }

    virtual RegistrableParent *dup()
    {
        _duped = true;
        DataType *data = new DataType();
        *data = *_value;

        Self *s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

protected:
    DataType *_value;
    QStringList _enum;

    bool _hasDefault;
    DataType _default;
};

template <>
class Registrable<ChannelSelectionType> : public RegistrableParent
{

public:
    typedef Registrable<ChannelSelectionType> Self;
    typedef ChannelSelectionType DataType;

    Registrable() : _value(0)
    {
    }

    ~Registrable()
    {
        if (_value && _duped)
        {
            delete _value;
        }
    }

    DataType &value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self &setValuePointer(DataType *v)
    {
        if (_value && _duped)
        {
            delete _value;
            _duped = false;
        }
        _value = v;
        if (_hasDefault)
            (*_value) = _default;

        return *this;
    }

    Self &setDefault(int v)
    {
        _hasDefault = true;
        _default = v;
        (*_value) = v;
        return *this;
    }

    Self &setRegex(QString match)
    {
        // Suggested Nuclei  regex like : "i(nuclei)|(hoechst).*"
        // TODO: 'i' starting  regex shall be case insensitive...
        _regex = match;
        return *this;
    }

    virtual QString toString() const
    {
        return QString("%1").arg((*_value)());
    }

    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);
        if (json.contains("Value"))
        {

            QString cur = json["Value"].toString();
            QJsonArray ar = json["Enum"].toArray();
            int val = 0;
            for (int i = 0; i < ar.size(); ++i)
                if (cur == ar.at(i).toString())
                {
                    val = i;
                    break; // Stop on first value if multiple of same are given
                }
            _value->setValue(val);
        }
    }

    virtual void write(QJsonObject &json) const
    {
        json["Type"] = QString("ChannelSelector");
        json["Default"] = _default;
        json["DefaultValue"] = _default;
        json["regex"] = _regex;
        RegistrableParent::write(json);
    }

    virtual RegistrableParent *dup()
    {
        _duped = true;
        DataType *data = new DataType();
        *data = *_value;

        Self *s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

protected:
    DataType *_value;
    QStringList _enum;
    QString _regex;
    bool _hasDefault;
    int _default;
};

template <>
class Registrable<QString> : public RegistrableParent
{
public:
    typedef Registrable<QString> Self;
    typedef QString DataType;

    Registrable() : _hasDefault(false), _isPath(false), _isDbPath(false), _value(0)
    {
    }

    ~Registrable()
    {
        if (_value && _duped)
        {
            delete _value;
            _duped = false;
        }
    }

    DataType &value()
    {
        return *_value;
    }
    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }
    Self &setValuePointer(DataType *v)
    {
        if (_value && _duped)
        {
            delete _value;
            _duped = false;
        }
        _value = v;
        if (_hasDefault)
            (*_value) = _default;

        return *this;
    }

    Self &setDefault(DataType v)
    {
        _hasDefault = true;
        _default = v;
        (*_value) = v;
        return *this;
    }

    virtual Self &isPath(bool p = true)
    {
        _isPath = p;
        return *this;
    }

    virtual Self &isDBPath(bool p = true)
    {
        _isPath = p;
        _isDbPath = p;
        return *this;
    }

    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);

        if (_hasDefault)
            (*_value) = _default;

        if (json.contains("Value"))
        {
            _wasSet = true;
            *_value = (DataType)json["Value"].toString();

        }
    }

    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);
        json["isString"] = true;
        json["isPath"] = _isPath;
        if (_hasDefault)
        {
            json["Default"] = _default;
        }
        json["isDbPath"] = _isDbPath;
    }

    virtual QString toString() const
    {
        return QString("%1").arg(*_value);
    }

    virtual RegistrableParent *dup()
    {
        _duped = true;
        DataType *data = new DataType();
        *data = *_value;

        Self *s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);
        s->isPath(_isPath);

        return s;
    }

protected:
    DataType *_value;
    bool _isPath, _isDbPath;
    bool _hasDefault;
    DataType _default;
};

#define THE_TYPE bool
#include "RegistrableIntegralType.h"
#undef THE_TYPE


#define THE_TYPE int
#include "RegistrableIntegralType.h"
#undef THE_TYPE

#define THE_TYPE short
#include "RegistrableIntegralType.h"
#undef THE_TYPE

#define THE_TYPE char
#include "RegistrableIntegralType.h"
#undef THE_TYPE

#define THE_TYPE long long int
#include "RegistrableIntegralType.h"
#undef THE_TYPE

#define THE_TYPE float
#include "RegistrableFloatType.h"
#undef THE_TYPE

#define THE_TYPE double
#include "RegistrableFloatType.h"
#undef THE_TYPE

#include "RegistrableImageType.h"

#include "RegistrableSecondOrderType.h"


template <class inner>
QJsonArray tojsonArr(inner& img)
{
    QJsonArray res;

    for (const auto& r: img)
        res.append(QString("%1").arg(r));
    return res;
}



#include "RegistrableStdContainerType.h"

RegistrableCont(std::vector, float)
RegistrableCont(std::vector, double)
RegistrableCont(std::vector, int)
RegistrableCont(std::vector, unsigned)

RegistrableCont(std::list, float)
RegistrableCont(std::list, double)
RegistrableCont(std::list, int)
RegistrableCont(std::list, unsigned)

RegistrableCont(QList, float)
RegistrableCont(QList, double)
RegistrableCont(QList, int)
RegistrableCont(QList, unsigned)


//RegistrableCont(QVector, float)
//RegistrableCont(QVector, double)
//RegistrableCont(QVector, int)
//RegistrableCont(QVector, unsigned)


#include "RegistrableStdContainerTypeSpecial.h"


#endif // REGISTRABLETYPES_H
