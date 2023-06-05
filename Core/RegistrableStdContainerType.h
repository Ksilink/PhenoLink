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


inline int toValI(QJsonValue v)
{
    if (v.isString())
        return v.toString().toInt();
    return v.toInt();
}

inline double toValF(QJsonValue v)
{
    if (v.isString())
        return v.toString().toDouble();
    return v.toDouble();
}


#define Str(arg) #arg

#define RegistrableCont(ContName, InnerType) \
    template <> \
    class Registrable< ContName < InnerType > >: public RegistrableParent\
    {\
    public:\
        typedef ContName< InnerType > DataType;\
        typedef Registrable<DataType> Self;\
    \
        Registrable():  _startChannel(0), _endChannel(-1), \
            _hasRange(false)\
        {\
    \
        }\
    \
        Self& setValuePointer(DataType *v)\
        {\
            _value = v;\
            return *this;\
    \
        }\
    \
    \
        Self& setDefault(InnerType v)\
        {\
            _def.clear();\
            _def.push_back(v);\
            return *this;\
        }\
    \
        Self& setDefault(DataType v)\
        {\
            _def = v;\
            return *this;\
        }\
    \
    \
        Self& setAggregationType(AggregationType agg)\
        {\
            _aggreg = agg;\
            return *this;\
        }\
    \
        AggregationType getAggregationType()\
        {\
            return _aggreg;\
        }\
    \
    \
        virtual void read(const QJsonObject &json)\
        {\
            RegistrableParent::read(json);\
            if (json.contains("Value"))\
            {\
    \
                if (json["Value"].isArray())\
                {\
                    QJsonArray ar = json["Value"].toArray();\
                    for (int i = 0; i < ar.size(); ++i)\
                        if (Str(InnerType) == "double" || Str(InnerType) == "float")\
                            (*_value).push_back((InnerType)toValF(ar.at(i)));\
                        else\
                            (*_value).push_back((InnerType)toValI(ar.at(i)));\
                }\
                else\
                {\
                    if (Str(InnerType) == "double" || Str(InnerType) == "float")\
                        (*_value).push_back((InnerType) toValF(json["Value"]));\
                    else\
                        (*_value).push_back((InnerType) toValI(json["Value"]));\
                }\
            }\
        }\
    \
        Self& setRange(ContName<InnerType> l, ContName<InnerType> h)\
        {\
            _hasRange = true;\
            _low = l;\
            _high = h;\
            return *this;\
        }\
    \
    \
        Self& setRange(InnerType l, InnerType h)\
        {\
            _hasRange = true;\
            _low.clear();\
            _low.push_back(l);\
            _high.clear();\
            _high.push_back(h);\
            return *this;\
        }\
    \
        virtual void write(QJsonObject &json) const\
        {\
            RegistrableParent::write(json);\
            json["Value"] = toString();\
            json["Type"] = QString("Container");\
            json["InnerType"]= QString( Str(InnerType) );\
            json["startChannel"] = _startChannel;\
            json["endChannel"] = _endChannel;\
    \
            json["Default"] = tojsonArr(_def);\
            json["Range/Set"] = _hasRange;\
            json["Range/Low"] = tojsonArr(_low);\
            json["Range/High"] = tojsonArr(_high);\
            json["Aggregation"] = QString(_aggreg == Sum ? "Sum" :\
                                                           ( _aggreg == Mean ? "Mean" :\
                                                                               ( _aggreg == Median ? "Median" :\
                                                                                                     ( _aggreg ==  Min ? "Min" : "Max" ))));\
        }\
    \
        virtual QString toString() const\
        {\
            QString data;\
    \
            for (auto it = _value->cbegin(), e = _value->cend(); it != e; ++it)\
                data += QString("%1;").arg(*it);\
    \
            return data;\
        }\
    \
        virtual void fromString(QString fr)\
        {\
            QStringList l = fr.split(";");\
            for (QStringList::const_iterator it = l.cbegin(), e = l.cend(); it != e; ++it)\
            {\
                 if (Str(InnerType) == "double" || Str(InnerType) == "float")   {\
                     _value->push_back((InnerType)it->toDouble());\
                 }\
                 else {\
                     _value->push_back((InnerType)it->toInt());\
                 }\
            }\
    \
        }\
    \
       Self& startChannel(int p)\
        {\
            _startChannel = p;\
            return *this;\
        }\
    \
        Self& endChannel(int p)\
        {\
            _endChannel = p;\
            return *this;\
        }\
    \
        virtual RegistrableParent* dup()\
        {\
            DataType* data = new DataType();\
            for (auto v: *_value)\
                (*data).push_back(v);\
    \
            Self* s = new Self();\
            s->setValuePointer(data);\
    \
            s->setTag(this->_tag);\
            s->setComment(this->_comment);\
            s->setHash(this->_hash);\
    \
            return s;\
        }\
\
    protected:\
        DataType* _value;\
    \
        int _startChannel, _endChannel;\
        DataType _def;\
        bool _hasRange;\
        DataType _low, _high;\
        AggregationType _aggreg;\
    };\




#endif // REGISTRABLESTDCONTAINERTYPE_H
