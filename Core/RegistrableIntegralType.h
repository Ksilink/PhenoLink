
#include <QtCore>

template <>
class Registrable<THE_TYPE>: public RegistrableParent
{
public:
  typedef Registrable<THE_TYPE> Self;
  typedef THE_TYPE DataType;

  Registrable(): _hasDefault(false), _hasRange(false), _low(std::numeric_limits<DataType>::min()), _high(std::numeric_limits<DataType>::max()),
    _isSlider(false)
  {

  }


  Self& setRange(DataType l, DataType h)
  {
    _hasRange = true;
    _low = l;
    _high = h;
    return *this;
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


  Self& setSlider()
  {
    _isSlider = true;
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
        _wasSet = true;
        if (json["Value"].isArray())
            *_value = (DataType)json["Value"].toArray().at(0).toInt();
        else
            *_value = (DataType)json["Value"].toInt();
      }

  }

  virtual void write(QJsonObject &json) const
  {
    RegistrableParent::write(json);
    json["isIntegral"] = true;
    json["Range/Set"] = _hasRange;
    json["Range/Low"] = _low;
    json["Range/High"] = _high;

    if (_hasDefault)
      {
        json["Default"] = _default;
      }

    json["isSlider"] = _isSlider;
    json["Aggregation"] = QString(_aggreg == Sum ? "Sum" :
                          ( _aggreg == Mean ? "Mean" :
                          ( _aggreg == Median ? "Median" :
                          ( _aggreg ==  Min ? "Min" : "Max" ))));
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

    s->setAggregationType(this->_aggreg); // Fix : Aggregation need to be copied otherwise unknown state will be set..


    return s;
  }


protected:
  DataType* _value;
  bool _hasRange;
  DataType _low, _high;
  bool _hasDefault;
  DataType _default;
  bool _isSlider;
  AggregationType _aggreg;

};

