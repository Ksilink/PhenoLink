#ifndef REGISTRABLESTDCONTAINERTYPE_H
#define REGISTRABLESTDCONTAINERTYPE_H



// Allows the registration of std::list, std::vector, std::set or std::map
// (and Qt equivalent)
// In list data will be constrained to:
// int, short, char, long long int (and all unsigned eqv.), float, double
// with addition of QString, QPoint, QPointF, QRect, QRectF and dedicated Circle types
//
// Implicit conversion can be added for plugin dev. ease of use
//


template <>
class Registrable<std::list<QPoint> >: public RegistrableParent
{
public:
    typedef std::list<QPoint> DataType;
    typedef Registrable<DataType> Self;

    Registrable()
    {

    }

    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;

    }

    //  Self& setDefault(DataType v)
    //  {
    //    _hasDefault = true;
    //    _default = v;
    //    return *this;
    //  }


    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);
        if (json.contains("Value"))
        {
            //        _wasSet = true;
            fromString(json["Value"].toString());
        }
    }
    Self& setAggregationType(AggregationType agg)
    {
        _aggreg = agg;
        return *this;
    }

    AggregationType getAggregationType()
    {
        return _aggreg;
    }


    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);
        json["Value"] = toString();
        json["Type"] = QString("Container");
        json["InnerType"]= QString("Point");
        json["Aggregation"] = QString(_aggreg == Sum ? "Sum" :
                                                       ( _aggreg == Mean ? "Mean" :
                                                                           ( _aggreg == Median ? "Median" :
                                                                                                 ( _aggreg ==  Min ? "Min" : "Max" ))));
        //        json["Enum"] = QJsonArray::fromStringList(_enum);

    }

    virtual QString toString() const
    {
        QString data;

        for (std::list<QPoint>::iterator it = _value->begin(), e = _value->end(); it != e; ++it)
            data += QString("%1, %2;").arg(it->x()).arg(it->y());

        return data;//QString("%1").arg(*_value);
    }

    virtual void fromString(QString fr)
    {
        QStringList l = fr.split(";");
        for (QStringList::iterator it = l.begin(), e = l.end(); it != e; ++it)
        {
            QStringList p = it->split(",");
            if (p.size() >= 2)
                _value->push_back(QPoint(p.at(0).toInt(), p.at(1).toInt()));
        }

    }


protected:
    DataType* _value;
    AggregationType _aggreg;

};


template <>
class Registrable<QList<unsigned> >: public RegistrableParent
{
public:
    typedef QList<unsigned> DataType;
    typedef Registrable<DataType> Self;

    Registrable():  _startChannel(0), _endChannel(-1), _def(0),
        _hasRange(false), _low(std::numeric_limits<int>::min()),
        _high(std::numeric_limits<int>::max())
    {

    }

    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;

    }


    Self& setDefault(unsigned v)
    {
        _def = v;
        return *this;
    }
    Self& setAggregationType(AggregationType agg)
    {
        _aggreg = agg;
        return *this;
    }

    AggregationType getAggregationType()
    {
        return _aggreg;
    }


    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);
        if (json.contains("Value"))
        {

            if (json["Value"].isArray())
            {
                QJsonArray ar = json["Value"].toArray();
                for (int i = 0; i < ar.size(); ++i)
                    (*_value) << ar.at(i).toInt();
            }
            else
                (*_value) << json["Value"].toInt();

            //        fromString(json["Value"].toString());
        }
    }

    Self& setRange(int l, int h)
    {
        _hasRange = true;
        _low = l;
        _high = h;
        return *this;
    }

    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);
        json["Value"] = toString();
        json["Type"] = QString("Container");
        json["InnerType"]= QString("unsigned");
        json["startChannel"] = _startChannel;
        json["endChannel"] = _endChannel;

        json["Default"] = _def;
        json["Range/Set"] = _hasRange;
        json["Range/Low"] = _low;
        json["Range/High"] = _high;
        json["Aggregation"] = QString(_aggreg == Sum ? "Sum" :
                                                       ( _aggreg == Mean ? "Mean" :
                                                                           ( _aggreg == Median ? "Median" :
                                                                                                 ( _aggreg ==  Min ? "Min" : "Max" ))));
        //        json["Enum"] = QJsonArray::fromStringList(_enum);

    }

    virtual QString toString() const
    {
        QString data;

        for (QList<unsigned>::const_iterator it = _value->cbegin(), e = _value->cend(); it != e; ++it)
            data += QString("%1;").arg(*it);

        return data;//QString("%1").arg(*_value);
    }

    virtual void fromString(QString fr)
    {
        QStringList l = fr.split(";");
        for (QStringList::const_iterator it = l.cbegin(), e = l.cend(); it != e; ++it)
        {
            _value->push_back(it->toInt());
        }

    }


    Self& startChannel(int p)
    {
        _startChannel = p;
        return *this;
    }

    Self& endChannel(int p)
    {
        _endChannel = p;
        return *this;
    }


    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        foreach (unsigned v, *_value)
            (*data) << v;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }




protected:
    DataType* _value;

    int _startChannel, _endChannel;
    int _def;
    bool _hasRange;
    int _low, _high;
    AggregationType _aggreg;
};


// DOUBLE

template <>
class Registrable<QList<double> >: public RegistrableParent
{
public:
    typedef QList<double> DataType;
    typedef Registrable<DataType> Self;

    Registrable(): _startChannel(0), _endChannel(-1), _def(0),
        _hasRange(false), _low(-std::numeric_limits<double>::max()),
        _high(std::numeric_limits<double>::max())
    {

    }

    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;

    }


    Self& setDefault(double v)
    {
        _def = v;
        return *this;
    }
    Self& setAggregationType(AggregationType agg)
    {
        _aggreg = agg;
        return *this;
    }

    AggregationType getAggregationType()
    {
        return _aggreg;
    }


    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);
        if (json.contains("Value"))
        {

            if (json["Value"].isArray())
            {
                QJsonArray ar = json["Value"].toArray();
                for (int i = 0; i < ar.size(); ++i)
                    (*_value) << ar.at(i).toDouble();
            }
            else
                (*_value) << json["Value"].toDouble();

            //        fromString(json["Value"].toString());
        }
    }

    Self& setRange(double l, double h)
    {
        _hasRange = true;
        _low = l;
        _high = h;
        return *this;
    }

    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);
        json["Value"] = toString();
        json["Type"] = QString("Container");
        json["InnerType"]= QString("double");
        json["startChannel"] = _startChannel;
        json["endChannel"] = _endChannel;

        json["Default"] = _def;
        json["Range/Set"] = _hasRange;
        json["Range/Low"] = _low;
        json["Range/High"] = _high;
        json["Aggregation"] = QString(_aggreg == Sum ? "Sum" :
                                                       ( _aggreg == Mean ? "Mean" :
                                                                           ( _aggreg == Median ? "Median" :
                                                                                                 ( _aggreg ==  Min ? "Min" : "Max" ))));
        //        json["Enum"] = QJsonArray::fromStringList(_enum);

    }

    virtual QString toString() const
    {
        QString data;

        for (QList<double>::const_iterator it = _value->cbegin(), e = _value->cend(); it != e; ++it)
            data += QString("%1;").arg(*it);

        return data;//QString("%1").arg(*_value);
    }

    virtual void fromString(QString fr)
    {
        QStringList l = fr.split(";");
        for (QStringList::const_iterator it = l.cbegin(), e = l.cend(); it != e; ++it)
        {
            _value->push_back(it->toDouble());
        }

    }

    Self& startChannel(int p)
    {
        _startChannel = p;
        return *this;
    }

    Self& endChannel(int p)
    {
        _endChannel = p;
        return *this;
    }


    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        foreach (double v, *_value)
            (*data) << v;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }




protected:
    DataType* _value;

    int _startChannel, _endChannel;
    double _def;
    bool _hasRange;
    double _low, _high;
    AggregationType _aggreg;
};



// SELECTION
template <>
class Registrable<QList<ChannelSelectionType> >: public RegistrableParent
{
public:
    typedef QList<ChannelSelectionType> DataType;
    typedef Registrable<QList<ChannelSelectionType > > Self;

    Registrable():  _startChannel(0), _endChannel(-1), _def(0)
    {

    }
    Self& setAggregationType(AggregationType agg)
    {
        _aggreg = agg;
        return *this;
    }

    AggregationType getAggregationType()
    {
        return _aggreg;
    }

    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;
    }


    Self& setDefault(unsigned v)
    {
        _def = v;
        return *this;
    }


    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);
        if (json.contains("Value"))
        {
            QJsonArray er= json["Enum"].toArray();

            if (json["Value"].isArray())
            {
                QJsonArray ar = json["Value"].toArray();
                for (int i = 0; i < ar.size(); ++i)
                {
                    QString cur = ar.at(i).toString();

                    int val = 0;
                    for (int i = 0; i < er.size(); ++i)
                        if (cur == er.at(i).toString())
                            val = i;

                    (*_value) << ChannelSelectionType(val);
                    //               (*_value) << ar.at(i).toInt();

                }
            }
            else
            {
                QString cur = json["Value"].toString();
                int val = 0;
                for (int i = 0; i < er.size(); ++i)
                    if (cur == er.at(i).toString())
                        val = i;

                (*_value) << ChannelSelectionType(val);
            }
        }
    }

    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);
        json["Value"] = toString();
        json["Type"] = QString("Container");
        json["InnerType"]= QString("ChannelSelector");
        json["startChannel"] = _startChannel;
        json["endChannel"] = _endChannel;
        json["DefaultValue"] = _def;
        json["Aggregation"] = QString(_aggreg == Sum ? "Sum" :
                                                       ( _aggreg == Mean ? "Mean" :
                                                                           ( _aggreg == Median ? "Median" :
                                                                                                 ( _aggreg ==  Min ? "Min" : "Max" ))));
    }

    virtual QString toString() const
    {
        QString data;

        for (QList<ChannelSelectionType>::const_iterator it = _value->cbegin(), e = _value->cend(); it != e; ++it)
            data += QString("%1;").arg((*it)());

        return data;//QString("%1").arg(*_value);
    }

    virtual void fromString(QString fr)
    {
        QStringList l = fr.split(";");
        for (QStringList::const_iterator it = l.cbegin(), e = l.cend(); it != e; ++it)
        {
            _value->push_back(ChannelSelectionType(it->toInt()));
        }

    }

    Self& startChannel(int p)
    {
        _startChannel = p;
        return *this;
    }

    Self& endChannel(int p)
    {
        _endChannel = p;
        return *this;
    }


    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        foreach (ChannelSelectionType v, *_value)
            (*data) << v;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }




protected:
    DataType* _value;
    ;
    int _startChannel, _endChannel;
    int _def;
    AggregationType _aggreg;
};


template <>
class RegistrableEnum<QList<unsigned> >: public RegistrableParent
{
public:
    typedef QList<unsigned> DataType;
    typedef RegistrableEnum<QList<unsigned > > Self;

    RegistrableEnum(): _startChannel(0), _endChannel(-1), _def(0)
    {

    }

    Self& setValuePointer(DataType *v)
    {
        _value = v;
        return *this;
    }


    Self& setDefault(unsigned v)
    {
        _def = v;
        return *this;
    }

    Self& setAggregationType(AggregationType agg)
    {
        _aggreg = agg;
        return *this;
    }

    AggregationType getAggregationType()
    {
        return _aggreg;
    }

    virtual void read(const QJsonObject &json)
    {
        RegistrableParent::read(json);
        if (json.contains("Value"))
        {
            QJsonArray er= json["Enum"].toArray();

            if (json["Value"].isArray())
            {
                QJsonArray ar = json["Value"].toArray();
                for (int i = 0; i < ar.size(); ++i)
                {
                    QString cur = ar.at(i).toString();

                    int val = 0;
                    for (int i = 0; i < er.size(); ++i)
                        if (cur == er.at(i).toString())
                            val = i;

                    (*_value) << (val);
                    //               (*_value) << ar.at(i).toInt();

                }
            }
            else
            {
                QString cur = json["Value"].toString();
                int val = 0;
                for (int i = 0; i < er.size(); ++i)
                    if (cur == er.at(i).toString())
                        val = i;

                (*_value) << (val);
            }
        }
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

    virtual void write(QJsonObject &json) const
    {
        RegistrableParent::write(json);
        json["Value"] = toString();
        json["Type"] = QString("Container");
        json["startChannel"] = _startChannel;
        json["endChannel"] = _endChannel;
        json["DefaultValue"] = _def;
        json["Enum"] = QJsonArray::fromStringList(_enum);
        json["Aggregation"] = QString(_aggreg == Sum ? "Sum" :
                                                       ( _aggreg == Mean ? "Mean" :
                                                                           ( _aggreg == Median ? "Median" :
                                                                                                 ( _aggreg ==  Min ? "Min" : "Max" ))));

    }

    virtual QString toString() const
    {
        QString data;

        for (QList<unsigned>::const_iterator it = _value->cbegin(), e = _value->cend(); it != e; ++it)
            data += QString("%1;").arg((*it));

        return data;//QString("%1").arg(*_value);
    }

    virtual void fromString(QString fr)
    {
        QStringList l = fr.split(";");
        for (QStringList::const_iterator it = l.cbegin(), e = l.cend(); it != e; ++it)
        {
            _value->push_back((it->toInt()));
        }

    }

    Self& perChannels()
    {
        return *this;
    }

    Self& startChannel(int p)
    {
        _startChannel = p;
        return *this;
    }

    Self& endChannel(int p)
    {
        _endChannel = p;
        return *this;
    }


    virtual RegistrableParent* dup()
    {
        DataType* data = new DataType();
        foreach (unsigned v, *_value)
            (*data) << v;

        Self* s = new Self();
        s->setValuePointer(data);

        s->setTag(this->_tag);
        s->setComment(this->_comment);
        s->setHash(this->_hash);

        return s;
    }




protected:
    DataType* _value;

    int _startChannel, _endChannel;
    int _def;
    QStringList _enum;

    bool _hasDefault;
    DataType _default;
    AggregationType _aggreg;
};








#endif // REGISTRABLESTDCONTAINERTYPE_H
