#ifndef REGISTRABLETYPES_H
#define REGISTRABLETYPES_H


#include <QtCore>
#include <typeinfo>


class DllPluginManagerExport RegistrableParent
{

public:

    typedef RegistrableParent Self;

    enum Level { Basic, Advanced, VeryAdvanced, Debug };
    enum AggregationType { Sum, Mean, Median, Min, Max };


    RegistrableParent():  _position(-1), _wasSet(false), _level(Basic), _isProduct(false),
        _isOptional(false), _isFinished(false), _isPerChannel(false), _keepInMem(false), _optionalDefault(false)
    {
    }

    // set Tag & set Comment shall not be exposed to the end user

    RegistrableParent& setTag(QString tag)
    {
        _tag = tag;
        return *this;
    }

    RegistrableParent& setComment(QString com)
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
    RegistrableParent& setPosition(int pos)
    {
        _position = pos;
        return *this;
    }

    RegistrableParent& setLevel(Level lvl)
    {
        _level = lvl;
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

        if (json.contains("DataHash"))
            _hash = json["DataHash"].toString();


        if (json.contains("PerChannelParameter"))
            _isPerChannel  = json["PerChannelParameter"].toBool();

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


    Self& setIf(QString tag, int value)
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
        json["DataHash"] = _hash;
        json["Level"] = QString(_level == Basic ? "Basic" :
                                                  _level == Advanced ? "Advanced" :
                                                                       _level == VeryAdvanced ? "VeryAdvanced" : "Debug");


        if (!_enableIf.isEmpty())
        {
            QJsonArray en;
            QPair<QString, int> p;
            foreach (p, _enableIf)
            {
                QJsonObject ob; ob[p.first] = p.second;
                en.push_back(ob);
            }
            json["enableIf"] = en;
        }


        json["isOptional"] = _isOptional;
        json["optionalState"] = _optionalDefault;

        if (!_group.isEmpty())
        {
            json["group"] = QString(_group);
        }
    }

    QString tag() const
    {
        return _tag;
    }

    Self& setGroup(QString g)
    {
        _group = g;
        return *this;
    }

    QString getGroup()
    {
        return _group;
    }



    Self& setAsOptional(bool val)
    {
        _isOptional = true;
        _optionalDefault = val;
        return *this;

    }

    Self& setFinished()
    {
        _isFinished = true;
        return *this;
    }

    // Use this function to tell the plugin that this is a "per channel" option
    Self& setPerChannel(bool val = true)
    {
        _isPerChannel = val;
        return *this;
    }

    //    virtual void clone()  = 0;
    virtual RegistrableParent* dup() = 0;
    void setHash(QString s) {_hash = s; /*qDebug() << "Data Hash" << _tag << _hash; */}
    QString getHash() const {return _hash; }

    void keepInMem(bool mem) { _keepInMem = mem; }

    //    void Q_DECL_DEPRECATED attachPayload(QByteArray arr) const;
    //    void Q_DECL_DEPRECATED attachPayload(QString hash, QByteArray arr) const;

    void attachPayload(std::vector<unsigned char> arr, size_t pos = 0) const;
    void attachPayload(QString hash, std::vector<unsigned char> arr, size_t pos = 0) const;



protected:

    QString _tag, _comment, _hash;
    int _position;
    bool _wasSet;
    Level _level;
    bool _isProduct, _isFinished;
    bool _isOptional, _optionalDefault;
    bool _isPerChannel;
    QList<QPair<QString, int> > _enableIf;
    QString _group;
    bool _keepInMem;

};

template <class Type>
class Registrable: public RegistrableParent
{
public:  
    typedef Registrable< Type> Self;
    typedef  Type DataType;


    // Value & setValuePointer shall not be exposed to the user
    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;
    }

    virtual QString toString() const
    {
        // if the following code does not compile,
        // it means that your are trying to Register an unsuported type
        // Please ask for datatype support to : wiest.daessle@gmail.com
        return QString("%1").arg(*_value);
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


protected:
    DataType* _value;
};

template <class T> inline QString tostr(T& val)
{
    return QString("%1").arg(val);
}
template <>
inline QString tostr(std::vector<int>& val)
{
    QString r;
    for (auto i : val)
        r = QString("%1,%2").arg(r).arg(i);
    return r;
}

template <class Type>
class RegistrablePair: public RegistrableParent
{
public:
    typedef RegistrablePair< Type> Self;
    typedef  Type DataType;


    RegistrablePair(): _hasDefault(false)
    {

    }

    // Value & setValuePointer shall not be exposed to the user
    DataType& value()
    {
        return *_value1;
    }

    void setValue(DataType t, DataType t2)
    {
        _wasSet = true;
        *_value1 = t;
        *_value2 = t;
    }

    Self& setValuePointer(DataType *v1, DataType* v2)
    {
        _value1 = v1;
        _value2 = v2;
        return *this;
    }

    virtual QString toString() const
    {
        // if the following code does not compile,
        // it means that your are trying to Register an unsuported type
        // Please ask for datatype support to : wiest.daessle@gmail.com
        return QString("%1;%2").arg(tostr(*_value1)).arg(tostr(*_value2));
    }


    Self& setComments(QString s1, QString s2)
    {
        _comment = s1;
        _comment2 = s2;

        return *this;
    }

    template <class T>
    void setValue(T* val, QString v, const QJsonObject& json)
    {
        if (json[v].isArray())
            *val = json[v].toArray().at(0).toInt();
        else
            *val = json[v].toInt();
    }

    template <>
    void setValue(std::vector<int>* val, QString v,const QJsonObject& json)
    {
        (*val).clear();
        if (!json.contains(v)) return;
        if (json[v].isArray())
        { // Might need magick c++ tricks for type dispatch :(
            QJsonArray ar = json[v].toArray();
            for (int i = 0; i < ar.size(); ++i)
                (*val).push_back(ar.at(i).toInt());
        }
        else
            (*val).push_back(json[v].toInt());
    }

    virtual void read(const QJsonObject &json)
    {
        //        qDebug() << "Reading value: " << json["Value"]
        //                 << json["Value2"];
        RegistrableParent::read(json);
        setValue(_value1, "Value", json);
        setValue(_value2, "Value2", json);
    }

    virtual RegistrableParent* dup()
    {
        DataType* data1 = new DataType();
        DataType* data2 = new DataType();
        *data1 = *_value1;
        *data2 = *_value2;

        Self* s = new Self();
        s->setValuePointer(data1, data2);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }

    template <typename T> inline QJsonValue tojson(const T& def) const {  static_assert(sizeof(T) == -1, "You have to have a specialization for to json");  }
    template <> inline QJsonValue tojson(const int& def) const { return  QJsonValue((int)def); }
    template <> inline QJsonValue tojson(const double& def) const { return  QJsonValue((double)def); }
    template <> inline QJsonValue tojson(const float& def) const { return  QJsonValue((double)def); }

    template <>
    inline QJsonValue tojson(const std::vector<int>& vals) const
    {
        QJsonArray res;
        for (int i = 0; i < vals.size(); ++i)
            res.push_back(vals[i]);
        return res;
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



    Self& setDefault(DataType  d1, DataType d2)
    {
        _hasDefault = true;
        _default1 = d1;
        _default2 = d2;

        return *this;
    }


    Self& setRange(DataType mi, DataType ma)
    {
        _hasRange = true;

        _minRange = mi;
        _maxRange = ma;

        return *this;
    }

protected:
    DataType* _value1;
    DataType* _value2;

    QString _comment2;

    bool _hasDefault;
    DataType _default1, _default2;

    bool _hasRange;
    DataType _minRange, _maxRange;

};



template <typename Type>
class RegistrableEnum: public RegistrableParent
{

public:
    typedef RegistrableEnum< Type> Self;
    typedef  Type DataType;

    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;
    }

    Self& setEnum(QStringList val)
    {
        _enum = val;
        return *this;
    }

    Self& setDefault(DataType v)
    {
        _hasDefault = true;
        _default = v;
        return *this;
    }


    virtual QString toString() const
    {
        return _enum.at(*_value);
    }


    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);
        if (json.contains("Value"))
        {
            int val = 0;
            for (int i = 0; i < _enum.size(); ++i)
                if (json["Value"].toString() == _enum[i])
                    val = i;
            *_value = (DataType)val;
        }
    }

    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);
        json["Enum"] = QJsonArray::fromStringList(_enum);
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
protected:
    DataType* _value;
    QStringList _enum;

    bool _hasDefault;
    DataType _default;
};








template <>
class Registrable<ChannelSelectionType>: public RegistrableParent
{

public:
    typedef Registrable<ChannelSelectionType> Self;
    typedef  ChannelSelectionType DataType;



    DataType& value()
    {
        return *_value;
    }

    void setValue(DataType t)
    {
        _wasSet = true;
        *_value = t;
    }

    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;
    }

    Self& setDefault(int v)
    {
        _hasDefault = true;
        _default = v;
        return *this;
    }

    Self& setRegex(QString match)
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
            QJsonArray ar= json["Enum"].toArray();
            int val = 0;
            for (int i = 0; i < ar.size(); ++i)
                if (cur == ar.at(i).toString())
                    val = i;

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
protected:
    DataType* _value;
    QStringList _enum;
    QString _regex;
    bool _hasDefault;
    int _default;
};


template <>
class Registrable<QString>: public RegistrableParent
{
public:
    typedef Registrable<QString> Self;
    typedef QString DataType;

    Registrable(): _hasDefault(false), _isPath(false)
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
    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;

    }

    Self& setDefault(DataType v)
    {
        _hasDefault = true;
        _default = v;
        return *this;
    }

    virtual Self& isPath(bool p = true)
    {
        _isPath = p;
        return *this;
    }

    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);

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

    }

    virtual QString toString() const
    {
        return QString("%1").arg(*_value);
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
        s->isPath(_isPath);

        return s;
    }


protected:
    DataType* _value;
    bool _isPath;
    bool _hasDefault;
    DataType _default;
};


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

#include "RegistrableStdContainerType.h"


#endif // REGISTRABLETYPES_H
