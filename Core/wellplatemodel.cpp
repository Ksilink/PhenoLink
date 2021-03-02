
#include "wellplatemodel.h"

#include "Core/checkoutdataloaderplugininterface.h"
#include <Core/checkoutprocess.h>

#include <QtSql>
#include <QJsonObject>
#include <QJsonArray>
#include <opencv2/highgui/highgui.hpp>
#include <QFile>
#include <QtConcurrent>
#include <QTextStream>


void stringToPos(QString pos, int& row, int& col);

ExperimentFileModel::ExperimentFileModel(Dictionnary dict): DataProperty(dict),
    _cols(0), _rows(0), _hasTag(false), _owner(0), _computedData(0)
{
}

void ExperimentFileModel::setRowCount(unsigned r)
{
    _rows = r;
}

unsigned ExperimentFileModel::getRowCount()
{
    return _rows;
}

void ExperimentFileModel::setColCount(unsigned r)
{
    _cols = r;
}

unsigned ExperimentFileModel::getColCount()
{
    return _cols;
}


bool ExperimentFileModel::isCurrent(QPoint pos)
{
    return getState(pos, IsCurrent);
}

QStringList ExperimentFileModel::getTags(QPoint Pos)
{
    SequenceFileModel& res = _sequences[Pos.x()][Pos.y()];
    if (res.isValid())
        return res.getTags();

    return QStringList();
}

void ExperimentFileModel::setTag(QPoint Pos, QString list)
{
    SequenceFileModel& res = _sequences[Pos.x()][Pos.y()];
    if (res.isValid())
    {
        _hasTag = true;
        res.setTag(list);
    }
}

bool ExperimentFileModel::hasTag()
{
    return _hasTag;
}

QString ExperimentFileModel::getColor(QPoint Pos)
{
    SequenceFileModel& res = _sequences[Pos.x()][Pos.y()];
    if (res.isValid())
        return res.getColor();

    return QString();
}

void ExperimentFileModel::setColor(QPoint Pos, QString col)
{
    SequenceFileModel& res = _sequences[Pos.x()][Pos.y()];
    if (res.isValid())
    {
        res.setColor(col);
    }
}




void ExperimentFileModel::setCurrent(QPoint pos, bool active)
{
    clearState(IsCurrent);
    return setState(pos, IsCurrent, active);
}

bool ExperimentFileModel::isSelected(QPoint pos)
{
    return getState(pos, IsSelected);
}

void ExperimentFileModel::select(QPoint pos, bool active)
{
    setState(pos, IsSelected, active);
}

bool ExperimentFileModel::hasMeasurements(QPoint pos)
{
    return getState(pos, HasMeasurement);
}

void ExperimentFileModel::setMeasurements(QPoint pos, bool active)
{
    setState(pos, HasMeasurement, active);
}


QSize ExperimentFileModel::getSize()
{
    return QSize(_rows, _cols);
}

QPair<QList<double>, QList<double> > getWellPos(SequenceFileModel* seq, unsigned fieldc,  int z, int t, int c)
{
    Q_UNUSED(fieldc);

    QSet<double> x,y;

    for (unsigned field = 1; field < seq->getFieldCount(); ++ field)
    {
        QString k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("X");
        double v = seq->property(k).toDouble();
        x.insert(v);
        k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("Y");
        v = seq->property(k).toDouble();
        y.insert(v);

    }
    qDebug() << "Unpack well pos sorted" << x << y;

    QList<double> xl(x.begin(), x.end()), yl(y.begin(), y.end());
    std::sort(xl.begin(), xl.end());
    std::sort(yl.begin(), yl.end());
    qDebug() << "Unpack well pos sorted" <<  xl << yl;
    //    xl.indexOf(), yl.indexOf()
    return qMakePair(xl,yl);
}

QPointF getFieldPos(SequenceFileModel* seq, int field, int z, int t, int c)
{
    QString k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("X");
    double x = seq->property(k).toDouble();
    k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("Y");
    double y = seq->property(k).toDouble();

    qDebug() << "Extracting Field Pos :" << field << x << y;
    return QPointF(x,y);
}



void ExperimentFileModel::setFieldPosition()
{
    // get first Well
    auto mdl = _sequences.begin().value().begin().value();
    int first_chan = *mdl.getChannelsIds().begin();

    int z = 1, t = 1, c = first_chan;
    QSet<double> x,y;

    for (auto sit = _sequences.begin(), se = _sequences.end(); sit != se; ++sit)
    {
        for (auto it = sit.value().begin(), ei = sit.value().end(); it != ei; ++it)
        {
            //


            for (unsigned field = 1; field <= it.value().getFieldCount(); ++field)
            {
                QString k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("X");
                x.insert(it.value().property(k).toDouble());
                k = QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("Y");
                y.insert(it.value().property(k).toDouble());
            }
        }
    }


    QList<double> xl(x.begin(), x.end()), yl(y.begin(), y.end());
    std::sort(xl.begin(), xl.end());
    std::sort(yl.begin(), yl.end());
    qDebug() << "Unpack well pos sorted" <<  xl << yl;



    fields_pos = qMakePair(xl,yl);

    for (unsigned i = 0; i < mdl.getFieldCount(); ++i)
    {
        QPointF p = getFieldPos(&mdl, i+1, 1, 1, 1);

        int x = fields_pos.first.indexOf(p.x());
        int y = fields_pos.second.size() - fields_pos.second.indexOf(p.y()) - 1;

        toField[x][y] = i+1;
    }
}

QMap<int, QMap<int, int> > ExperimentFileModel::getFieldPosition()
{
    return toField;
}

QPair<QList<double>, QList<double> > ExperimentFileModel::getFieldSpatialPositions()
{
    return fields_pos;
}





SequenceFileModel &ExperimentFileModel::operator()(int row, int col)
{
    SequenceFileModel& res = _sequences[row][col];
    if (!res.getOwner())
        res.setOwner(this);
    res.setPos(row, col);
    return res;
}

SequenceFileModel &ExperimentFileModel::operator()(QPoint Pos)
{
    SequenceFileModel& res = _sequences[Pos.x()][Pos.y()];
    if (!res.getOwner())
        res.setOwner(this);

    res.setPos(Pos.x(), Pos.y());
    return res;
}

QList<SequenceFileModel *> ExperimentFileModel::getSelection()
{
    QList<SequenceFileModel *> r;

    //qDebug() << "Building list of exp to load";
    for(QMap<int, QMap<int, SequenceFileModel> >::iterator i = _sequences.begin(), e = _sequences.end(); i != e; ++i)
    {
        for (QMap<int, SequenceFileModel>::iterator i1 = i.value().begin(), e1 = i.value().end(); i1 != e1; ++i1)
            if (isSelected(QPoint(i.key(), i1.key())))
            {
                //           qDebug() << i.key() << i1.key();
                r << &i1.value();
            }
    }

    return r;
}




QList<SequenceFileModel *> ExperimentFileModel::getAllSequenceFiles()
{
    QList<SequenceFileModel *> r;


    for(QMap<int, QMap<int, SequenceFileModel> >::iterator i = _sequences.begin(), e = _sequences.end(); i != e; ++i)
    {
        for (QMap<int, SequenceFileModel>::iterator i1 = i.value().begin(), e1 = i.value().end(); i1 != e1; ++i1)
            r << &i1.value();
    }

    return r;
}

QList<SequenceFileModel *> ExperimentFileModel::getValidSequenceFiles()
{
    QList<SequenceFileModel *> r;


    for(QMap<int, QMap<int, SequenceFileModel> >::iterator i = _sequences.begin(), e = _sequences.end(); i != e; ++i)
    {
        for (QMap<int, SequenceFileModel>::iterator i1 = i.value().begin(), e1 = i.value().end(); i1 != e1; ++i1)
            if (this->hasMeasurements(QPoint(i.key(), i1.key())))
                r << &i1.value();
    }

    return r;
}

SequenceFileModel& ExperimentFileModel::getFirstValidSequenceFiles()
{
    return _sequences.begin().value().begin().value();
}

void ExperimentFileModel::addToDatabase()
{
    QDateTime d = QDateTime::currentDateTime();
    QString date = d.toString(Qt::ISODate);
    QString hash = this->hash();
    //          qDebug() << date;
    QSqlDatabase db = QSqlDatabase::database();
    QStringList tables = db.tables();

    QSqlQuery q;
    q.prepare("INSERT OR REPLACE INTO OpennedScreens (hash, ScreenDirectory, lastload) "
              "VALUES (:hash, :sd, :lastload);");


    q.bindValue(":hash", hash);
    q.bindValue(":sd", property("file"));
    q.bindValue(":lastload", date);
    if (!q.exec())
        qDebug() << q.lastError();

    bool table_exists = true;
    if (!tables.contains(QString("WellMeasurements_%1").arg(hash)))
    {
        if (!q.exec(QString("create table WellMeasurements_%1 (id text,"
                            "timePointCount integer, fieldCount integer,"
                            "sliceCount integer, channels integer);").arg(hash)))
            qDebug () << q.lastError();
        table_exists = false;
    }


    unsigned char key[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (!table_exists)
        for (unsigned r = 0; r < getRowCount(); ++r)
        {
            for (unsigned c = 0; c < getColCount(); ++c)
            {
                if (hasMeasurements(QPoint(r,c)))
                {
                    SequenceFileModel& fm = (*this)(r,c);

                    if (!q.exec(QString("insert or replace into WellMeasurements_%1 (id ,"
                                        "timePointCount , fieldCount ,"
                                        "sliceCount , channels ) values ('%2', '%3', '%4', '%5', '%6');")
                                .arg(hash)
                                .arg(QString("%1%2").arg(QChar(key[r])).arg(c+1, 2, 10, QLatin1Char('0')))
                                .arg(fm.getTimePointCount())
                                .arg(fm.getFieldCount())
                                .arg(fm.getZCount())
                                .arg(fm.getChannels())
                                ))
                        qDebug() << q.lastError();
                }
            }
        }


    if (!tables.contains(QString("WellParameters_%1").arg(hash)))
        if (!q.exec(QString("create table WellParameters_%1 (id text,"
                            "timepoint integer, fieldId integer,"
                            "sliceId integer, channel integer,"
                            "Imagehash text,"
                            "MinIntensity real, MaxIntensity real,"
                            "DispMinIntensity real, DispMaxIntensity real);").arg(hash)))
            qDebug() << q.lastError();

    if (!tables.contains(QString("WellMeasurement_details_%1").arg(hash)))
        if (!q.exec(QString("create table WellMeasurement_details_%1 (id text,"
                            "timepoint integer, fieldId integer,"
                            "sliceId integer, channel integer);").arg(hash)))
            qDebug() << q.lastError();


}



void ExperimentFileModel::setState(QPoint pos, ExperimentFileModel::WellState state, bool active)
{
    unsigned l = _state[pos.x()][pos.y()];

    //  qDebug() << "Setting state" << pos << state;
    l &= ~(1 << state);

    _state[pos.x()][pos.y()] = l | active << state;

}

void ExperimentFileModel::clearState(ExperimentFileModel::WellState state)
{
    for (QMap<int, QMap<int, unsigned> >::iterator it = _state.begin(), e = _state.end(); e != it; ++it)
    {
        for (QMap<int, unsigned>::iterator lit = it.value().begin(), le = it.value().end(); le != lit; ++lit)
        {
            lit.value() &=  ~(1 << state);
        }
    }
}

QList<ExperimentFileModel *> ExperimentFileModel::getSiblings()
{
    return _siblings.values();
}

ExperimentFileModel* ExperimentFileModel::getSibling(QString name)
{
    if (_siblings.contains(name))
        return _siblings[name];

    Dictionnary dic;
    ExperimentFileModel* mdl = new ExperimentFileModel(dic);
    _siblings[name] = mdl;
    mdl->_owner = this;
    return mdl;
}

ExperimentDataModel *ExperimentFileModel::computedDataModel()
{
    if (!_computedData)
        _computedData = new ExperimentDataModel(this, _cols, _rows);
    return _computedData;
}

bool ExperimentFileModel::hasComputedDataModel()
{
    return (_computedData != 0);
}

ExperimentDataModel *ExperimentFileModel::databaseDataModel(QString name)
{
    if (!_databaseData[name])
        _databaseData[name] = new ExperimentDataModel(this, _cols, _rows);
    return _databaseData[name];
}

bool ExperimentFileModel::hasdatabaseDataModel(QString name)
{
    return (_databaseData[name] != 0);
}

QStringList ExperimentFileModel::getDatabaseNames()
{
    return _databaseData.keys();
}

QList<QPoint>& ExperimentFileModel::measurementPositions()
{
    if (_positions.isEmpty())
    {
        for (QMap<int, QMap<int, unsigned> >::iterator it = _state.begin(), e = _state.end(); e != it; ++it)
            for (QMap<int, unsigned>::iterator lit = it.value().begin(), le = it.value().end(); le != lit; ++lit)
                _positions << QPoint(it.key(), lit.key());
    }
    return _positions;
}

bool ExperimentFileModel::displayed()
{
    return _displayed;
}

void ExperimentFileModel::setDisplayed(bool val)
{
    _displayed = val;
}

QString ExperimentFileModel::hash()
{
    return _hash;
}

void ExperimentFileModel::setChannelNames(QStringList names) { _channelNames = names;}

QStringList ExperimentFileModel::getChannelNames() {return _channelNames; }

void ExperimentFileModel::setMetadataPath(QString m) { metadataPath = m; }

QString ExperimentFileModel::getMetadataPath() { return metadataPath; }


void ExperimentFileModel::addSiblings(QString map, ExperimentFileModel *efm)
{
    _siblings[map] = efm;
}

bool ExperimentFileModel::getState(QPoint pos, ExperimentFileModel::WellState state)
{
    return _state[pos.x()][pos.y()] & (1 << state);
}
QString ExperimentFileModel::fileName() const
{
    return _fileName;
}

void ExperimentFileModel::setFileName(const QString &fileName)
{
    _fileName = fileName;
}


QString ExperimentFileModel::name() const
{
    if (_name.isEmpty() && _owner)
        return _owner->name();
    return _name;
}

void ExperimentFileModel::setName(const QString &name)
{
    _name = name;
}

QString ExperimentFileModel::groupName() const
{
    return _groupName;
}

void ExperimentFileModel::setGroupName(const QString &name)
{
    _groupName = name;
}

ExperimentFileModel *ExperimentFileModel::getOwner()
{
    return _owner;
}

void ExperimentFileModel::setOwner(ExperimentFileModel *own)
{
    _owner = own;
}



void ExperimentFileModel::reloadDatabaseData()
{

    QPair<QStringList, QStringList> dbs = databases();

    for (auto file: dbs.first)
    {
        QString t = file;
        QStringList spl = t.split("/");
        t=spl[spl.size() -2];


        reloadDatabaseData(file, t, false);
    }
    for (auto file: dbs.first)
    {
        QString t = file;
        QStringList spl = t.split("/");
        t=spl[spl.size() -2];
        reloadDatabaseData(file, t, true);
    }
}


void ExperimentFileModel::reloadDatabaseData(QString file, QString t, bool aggregat)
{
    ExperimentDataModel* data = databaseDataModel(t);


    QFile files(file);
    if (!files.open(QIODevice::ReadOnly | QIODevice::Text ))
        return;
    QTextStream in(&files);
    QString line = in.readLine();
    QStringList header = line.split(",");

    while (!in.atEnd()) {
        line = in.readLine();
        QStringList vals = line.split(",");

        QPoint pos = ExperimentDataTableModel::stringToPos(vals.at(1));

        if (aggregat)
        {
            QStringList tags = vals.at(2).split(";");

            for (int i = 3; i < vals.size(); ++i)
            {
                double d = vals.at(i).toDouble();
                data->addData(header.at(i), 0, 0, 0, 0, pos, d);
            }
        }
        else
        {
            int field  = vals.at(2).toInt();
            int stackZ = vals.at(3).toInt();
            int time   = vals.at(4).toInt();
            int chan   = vals.at(5).toInt();
            QStringList tags = vals.at(6).split(";");

            for (int i = 7; i < vals.size(); ++i)
            {

                double d = 0;
                if (!vals.at(i).isEmpty())
                    d = vals.at(i).toDouble();
                data->addData(header.at(i), field, stackZ, time, chan, pos, d);
            }
        }

    }
}


void getOLegacyDB(QString ddir, QString hash, QStringList& raw, QStringList& ag)
{
    QDir dir(ddir);
    QFileInfoList ff = dir.entryInfoList(QStringList() <<  hash +"_*.csv", QDir::Files);
    foreach (QFileInfo i, ff)
    {

        QString t =  i.absoluteFilePath();
        raw += t;

    }

    ff = dir.entryInfoList(QStringList() << "*/ag" << hash +".csv", QDir::Files);
    foreach (QFileInfo i, ff)
    {
        QString t =  i.absoluteFilePath();
        ag += t;

    }
}


void getDBs(QString ddir, QString hash, QStringList& raw, QStringList& ag)
{
    QDir dir(ddir);
    QFileInfoList dirs = dir.entryInfoList(QStringList() << "*", QDir::Dirs);
    foreach (QFileInfo d, dirs)
    {
        if (QFile::exists(d.filePath() + "/"+hash+".csv"))
        {
            QString t =  d.absoluteFilePath().remove(dir.absolutePath()+"/");
            if (t.isEmpty()) continue;
            raw += d.filePath() + "/"+hash+".csv";
        }
        if (QFile::exists(d.filePath() + "/ag"+hash+".csv"))
        {
            QString t =  d.absoluteFilePath().remove(dir.absolutePath()+"/");
            if (t.isEmpty()) continue;
            ag += d.filePath() + "/ag"+hash+".csv";
        }

    }
}

QPair<QStringList, QStringList> ExperimentFileModel::databases()
{
    QSettings set;
    QStringList raw, ag;


    { // older legacy ..
        getOLegacyDB(set.value("databaseDir").toString(), _hash, raw, ag);
    }

    { // Legacy 2nd version
        getDBs(set.value("databaseDir").toString(), _hash, raw, ag);
    }


    { // New settings
        QString writePath = QString("%1/%2/Checkout_Results/").arg(set.value("databaseDir").toString())
                .arg(property("project"));
        getDBs(writePath, name(), raw, ag);
    }

    return qMakePair(raw, ag);
}


void ExperimentFileModel::setProperties(QString ttag, QString value)
{
    // If a dictionnary is to be used to convert properties into a comprehensible inside format
    //  qDebug() <<ttag << value;
    QString tag = ttag;
    if (_dict.contains(tag))
        tag = _dict[tag];

    if (tag.endsWith("RowCount"))
        setRowCount(value.toUInt());
    if (tag.endsWith("ColumnCount"))
        setColCount(value.toUInt());

    if (ttag == "hash")
    {
        _hash = value;

        // Extract Tag selection from

        QSettings set;
        //        qDebug() << "Adding database:" << QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/databases/" + hash + ".db";
        QDir dir(set.value("databaseDir").toString());

        QFile file(dir.absolutePath() + "/"+ _hash + ".tagmap");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString line = file.readLine();
            QStringList l = line.split(';');

            QStringList tt = set.value("Tags", QStringList()).toStringList();

            foreach (QString s, l)
                if (!tt.contains(s.simplified())) // Check we already have the tag in tag list...
                    l << s.simplified();

            while (!file.atEnd()) {
                QString line = file.readLine();
                QStringList split = line.split(';');
                QString Pos = split.at(0);
                QPoint p;
                stringToPos(Pos, p.rx(), p.ry());

                split.pop_front();
                foreach(QString t, split)  setTag(p, t.simplified());

            }
        }

        // Load database data here !
        reloadDatabaseData();

    }

    DataProperty::setProperties(ttag, value);
}

QString ExperimentFileModel::property(QString tag)
{
    QString r = DataProperty::property(tag);
    if (r.isEmpty() && getOwner())
        return getOwner()->property(tag);
    return r;
}


void DataProperty::setProperties(QString ttag, QString value)
{
    QString tag = ttag;
    if (_dict.contains(tag))
        tag = _dict[tag];

    _properties[tag] = value;
}

QString DataProperty::property(QString tag)
{
    return _properties[tag].toString();
}

QString DataProperty::properties()
{
    QString r;
    //  qDebug() << "-*-*-*-*-*--*- Properties";
    for (QMap<QString, QVariant>::iterator it = _properties.begin(), e = _properties.end(); it != e; ++it)
    {
        //      qDebug() << it.key() << it.value().toString();
        r += QString("\t\"%1\" : \"%2\",\n").arg(it.key()).arg(it.value().toString());
    }
    //  qDebug() << "#*#*#*#*#*#*#*#*#*# Properties";
    return r;
}

bool DataProperty::hasProperty(QString tag)
{
    return _properties.contains(tag);
}


SequenceFileModel::SequenceFileModel(Dictionnary dict):
    DataProperty(dict), _isValid(true), _isShowed(false), _toDisplay(true), _owner(0)
{

}

unsigned SequenceFileModel::getTimePointCount() const
{
    return _data.cbegin()->cbegin()->size();
}

unsigned SequenceFileModel::getFieldCount() const
{
    return _data.size();
}

unsigned SequenceFileModel::getZCount() const
{
    return _data.cbegin()->size();
}

unsigned SequenceFileModel::getChannels() const
{

    //  qDebug() << "Check Validity" << Pos();
    FieldImaging::const_iterator fi = _data.cbegin();
    if (fi == _data.cend()) { return 0; }

    ImageStack::const_iterator si = fi->cbegin();
    if (si == fi->cend())  { return 0; }

    TimeLapse::const_iterator ti = si->cbegin();
    if (ti == si->cend())  { return 0; }

    Channel::const_iterator ci = ti->cbegin();
    if (ci == ti->cend())  { return 0; }

    return _data.cbegin()->cbegin()->cbegin()->size();
}

void SequenceFileModel::addFile(int timePoint, int fieldIdx, int Zindex, int channel, QString file)
{
    _channelsIds.insert(channel);
    //  qDebug() << "Adding: "<< timePoint << fieldIdx << Zindex << channel;
    _data[fieldIdx][Zindex][timePoint][channel] = file;
}

void SequenceFileModel::addMeta(int timePoint, int fieldIdx, int Zindex, int channel, QString name, StructuredMetaData meta)
{
    _sdata[fieldIdx][Zindex][timePoint][channel][name] = meta;
}

QStringList SequenceFileModel::getAllFiles()
{
    QStringList files;


    for (auto f: _data)
        for (auto z: f)
            for (auto t: z)
                for (auto c: t)
                    files << c;

    return files;
}

StructuredMetaData &SequenceFileModel::getMeta(int timePoint, int fieldIdx, int Zindex, int channel, QString name)
{
    static StructuredMetaData r;
    if (_sdata.contains(fieldIdx))
        if (_sdata[fieldIdx].contains(Zindex))
            if (_sdata[fieldIdx][Zindex].contains(timePoint))
                if (_sdata[fieldIdx][Zindex][timePoint].contains(channel))
                    if (_sdata[fieldIdx][Zindex][timePoint][channel].contains(name))
                        return _sdata[fieldIdx][Zindex][timePoint][channel][name];

    return r;
}

int SequenceFileModel::getMetaChannels(int timePoint, int fieldIdx, int Zindex)
{
    if (_sdata.contains(fieldIdx))
        if (_sdata[fieldIdx].contains(Zindex))
            if (_sdata[fieldIdx][Zindex].contains(timePoint))
                return _sdata[fieldIdx][Zindex][timePoint].size();
    return 0;
}

QMap<QString, StructuredMetaData> &SequenceFileModel::getMetas(int timePoint, int fieldIdx, int Zindex, int channel)
{


    static QMap<QString, StructuredMetaData> r;

    if (_sdata.contains(fieldIdx))
        if (_sdata[fieldIdx].contains(Zindex))
            if (_sdata[fieldIdx][Zindex].contains(timePoint))
                if (_sdata[fieldIdx][Zindex][timePoint].contains(channel))
                    return _sdata[fieldIdx][Zindex][timePoint][channel];


    return r;
}




//QStringList SequenceFileModel::getMetaNames(int timePoint, int fieldIdx, int Zindex, int channel)
//{
//    return QStringList();
//}

//bool SequenceFileModel::hasMeta(int timePoint, int fieldIdx, int Zindex, int channel, QString name )
//{
//    if (name.isEmpty())
//    {

//    }
//    else
//    {

//    }
//    return false;
//}

bool SequenceFileModel::hasChannel(int timePoint, int fieldIdx, int Zindex, int channel)
{
    return  _data[fieldIdx][Zindex][timePoint].contains(channel);
}

void SequenceFileModel::setPos(unsigned r, unsigned c)
{
    _row = r; _col = c;
}

QPoint SequenceFileModel::pos()
{
    return QPoint(_row, _col);
}

// FIXME: Add Helper function for this and handle the pos/point to text in a single function (avoiding errors)
QString SequenceFileModel::Pos()
{
    unsigned char key[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    return (QString("%1%2").arg(QChar(key[_row])).arg(_col+1, 2, 10, QLatin1Char('0')));
}

bool SequenceFileModel::isAlreadyShowed()
{
    return _isShowed;
}

void SequenceFileModel::setAsShowed()
{
    _isShowed = true;
}

bool SequenceFileModel::toDisplay()
{
    return _toDisplay;
}

void SequenceFileModel::setDisplayStatus(bool disp)
{
    _toDisplay = disp;
}

void SequenceFileModel::setProcessResult(bool val)
{
    _isProcessResult = val;
}

void SequenceFileModel::addSibling(SequenceFileModel *mdl)
{
    if (!_siblings.contains(mdl))
        _siblings << mdl;
}

QList<SequenceFileModel *> SequenceFileModel::getSiblings()
{
    return _siblings;
}

QString SequenceFileModel::property(QString tag)
{
    QString r = DataProperty::property(tag);
    if (r.isEmpty() && getOwner())
        r = getOwner()->property(tag);
    return r;
}

void SequenceFileModel::setInvalid()
{
    _isValid = false;
}

bool SequenceFileModel::isValid()
{
    return _isValid;
}

void SequenceFileModel::checkValidity()
{
    //  qDebug() << "Check Validity" << Pos();
    FieldImaging::iterator fi = _data.begin();
    if (fi == _data.end()) { _isValid = false; return; }

    ImageStack::iterator si = fi->begin();
    if (si == fi->end())  { _isValid = false; return; }

    TimeLapse::iterator ti = si->begin();
    if (ti == si->end())  { _isValid = false; return; }

    Channel::iterator ci = ti->begin();
    if (ci == ti->end())  { _isValid = false; return; }
    //qDebug() << true;
    _isValid = true;
}

bool SequenceFileModel::hasMeasurements()
{
    FieldImaging::iterator fi = _data.begin();
    if (fi == _data.end()) { return false; }

    ImageStack::iterator si = fi->begin();
    if (si == fi->end())  { return false; }

    TimeLapse::iterator ti = si->begin();
    if (ti == si->end())  { return false; }

    Channel::iterator ci = ti->begin();
    if (ci == ti->end())  { return false; }
    //qDebug() << true;
    return  true;
}

void SequenceFileModel::setSelectState(bool val)
{
    _owner->select(this->pos(), val);
}

bool SequenceFileModel::isSelected()
{
    return _owner->isSelected(this->pos());
}

void SequenceFileModel::setTag(QString tag)
{
    tag.replace(",",";");
    if (!tag.simplified().isEmpty())
        if (!_tags.contains(tag))
            _tags << tag;
}

void SequenceFileModel::unsetTag(QString tag)
{
    if (_tags.contains(tag))
        _tags.removeAll(tag);
}

void SequenceFileModel::clearTags()
{
    _tags.clear();
}

QStringList SequenceFileModel::getTags()
{
    return _tags;
}

void SequenceFileModel::setChannelNames(QStringList names)
{
    _owner->setChannelNames(names);
}

QStringList SequenceFileModel::getChannelNames()
{
    return    _owner->getChannelNames();
}

void SequenceFileModel::setColor(QString col)
{
    _color = col;
}

QString SequenceFileModel::getColor()
{
    return _color;
}




bool SequenceFileModel::isProcessResult()
{
    return _isProcessResult;
}

QString SequenceFileModel::getFile(int timePoint, int fieldIdx, int Zindex, int channel)
{
    typedef QMap<int, QString> Channel;
    typedef QMap<int, Channel> TimeLapse;
    typedef QMap<int, TimeLapse> ImageStack;
    typedef QMap<int, ImageStack> FieldImaging;

    if (_data.size() < fieldIdx-1) return QString();
    FieldImaging::iterator fi = _data.begin(); std::advance(fi, fieldIdx-1);

    if (fi->size() < Zindex-1) return QString() ;
    ImageStack::iterator si = fi->begin(); std::advance(si, Zindex-1);
    if (si->size() < timePoint - 1) return QString();
    TimeLapse::iterator ti = si->begin(); std::advance(ti, timePoint-1);
    if (ti->size() < channel  ) return QString();
    Channel::iterator ci = ti->begin(); std::advance(ci, channel-1);
    return ci.value();
}


QString SequenceFileModel::getFileChanId(int timePoint, int fieldIdx, int Zindex, int channel)
{
    typedef QMap<int, QString> Channel;
    typedef QMap<int, Channel> TimeLapse;
    typedef QMap<int, TimeLapse> ImageStack;
    typedef QMap<int, ImageStack> FieldImaging;

    if (_data.size() < fieldIdx-1) return QString();
    FieldImaging::iterator fi = _data.begin(); std::advance(fi, fieldIdx-1);
    if (fi->size() < Zindex-1) return QString() ;
    ImageStack::iterator si = fi->begin(); std::advance(si, Zindex-1);
    if (si->size() < timePoint - 1) return QString();
    TimeLapse::iterator ti = si->begin(); std::advance(ti, timePoint-1);
    if (ti->size() < channel) return QString();
    return (*ti)[channel];
}


SequenceFileModel::Channel &SequenceFileModel::getChannelsFiles(int timePoint, int fieldIdx, int Zindex)
{
    FieldImaging::iterator fi = _data.begin(); std::advance(fi, fieldIdx-1);
    ImageStack::iterator si = fi->begin(); std::advance(si, Zindex-1);
    TimeLapse::iterator ti = si->begin(); std::advance(ti, timePoint-1);
    return ti.value();
}

QSet<int> SequenceFileModel::getChannelsIds()
{
    return _channelsIds;
}

void SequenceFileModel::setOwner(ExperimentFileModel *ow)
{
    _owner = ow;
}

ExperimentFileModel *SequenceFileModel::getOwner()
{
    return _owner;
}


QList<QJsonObject> SequenceFileModel::toJSONvector(Channel channels,
                                                   QString imageType, QList<bool> selectedChanns, MetaDataHandler &h)
{
    Q_UNUSED(imageType  );

    QList<QJsonObject> res;

    QJsonArray data;
    QJsonArray chans;
    QJsonObject parent;
    int chann = 0;

    if (selectedChanns.count() != 0)
    {
        for(Channel::iterator it = channels.begin(), e = channels.end(); it != e; ++it, ++chann)
            if (!it.value().startsWith(":/mem/") && chann < selectedChanns.size() && selectedChanns.at(chann))
            {
                h.channel = it.key();
                data.append(it.value());
                chans.append(it.key());
            }
    }
    else
    {
        for(Channel::iterator it = channels.begin(), e = channels.end(); it != e; ++it, ++chann)
        if (!it.value().startsWith(":/mem/"))
        {
            h.channel = it.key();
            data.append(it.value());
            chans.append(it.key());
        }
    }


    parent["Data"] = data;
    parent["Channels"] = chans;
    parent["ChannelNames"] = QJsonArray::fromStringList(this->getChannelNames());
    parent["Channel"] = -1;

    QJsonObject props = getMeta(h);
    if (!props.isEmpty())
        parent["Properties"] = props;

    if (!data.empty())
        res << parent;

    return res;
}

QJsonObject SequenceFileModel::getMeta(SequenceFileModel::MetaDataHandler &h)
{
    QJsonObject props;
    for (auto k: h.metaData)
    {

        QString mtag = QString("f%1s%2t%3c%4%5").arg(h.fieldIdx).arg(h.zindex)
                .arg(h.timepoint).arg(h.channel).arg(k);
        QString t = this->property(mtag);
        if (!t.isEmpty())
            props[k] = t;
        t = this->property(k);
        if (!t.isEmpty())
            props[k] = t;
    }

    if (_owner)
    {
        for (auto k: h.metaData)
        {
            QString t = _owner->property(k);
            if (!t.isEmpty())
                props[k] = t;
            for (unsigned c = 0; c < this->getChannels(); ++c)
            {
                QString ch =k+QString("_ch%1").arg(c+1);
                QString t = _owner->property(ch);
                if (!t.isEmpty())
                    props[ch] = t;
            }
        }
    }

    return props;
}

QList<QJsonObject> SequenceFileModel::toJSONvector(TimeLapse times,
                                                   QString imageType, QList<bool> selectedChanns, MetaDataHandler& h)
{
    QList<QJsonObject> res;

    if (imageType.contains("Time"))
    {
        QJsonArray data;
        for (TimeLapse::iterator it = times.begin(), e = times.end(); it != e; ++it)
        {
            h.timepoint = it.key();
            QList<QJsonObject> s = toJSONvector(it.value(), imageType,selectedChanns, h);
            foreach (QJsonObject o, s)
            {
                o["TimePos"] = it.key();
                data.append(o);
            }
        }
        QJsonObject obj;
        obj["Data"] = data;
        obj["TimePos"] = -1;
        if (!data.empty())
            res << obj;
        return res;
    }
    else
    {
        for (TimeLapse::iterator it = times.begin(), e = times.end(); it != e; ++it)
        {
            h.timepoint = it.key();
            QList<QJsonObject> s = toJSONvector(it.value(), imageType,selectedChanns, h);
            foreach (QJsonObject o, s)
            {
                o["TimePos"] = it.key();
                res << o;
            }
        }

        return res;
    }
}


QList<QJsonObject> SequenceFileModel::toJSONvector(ImageStack stack,
                                                   QString imageType, QList<bool> selectedChanns, MetaDataHandler &h)
{
    QList<QJsonObject> res;

    if (imageType.contains("Stack"))
    {
        QJsonArray data;
        for (ImageStack::iterator it = stack.begin(), e = stack.end(); it != e; ++it)
        {
            h.zindex = it.key();
            QList<QJsonObject> s = toJSONvector(it.value(), imageType, selectedChanns,h);
            foreach (QJsonObject o, s)
            {
                o["zPos"] = it.key();
                data.append(o);
            }
        }

        QJsonObject obj;
        obj["Data"] = data;
        obj["zPos"] = -1;
        if (!data.empty())
            res << obj;
        return res;
    }
    else
    {
        for (ImageStack::iterator it = stack.begin(), e = stack.end(); it != e; ++it)
        {
            h.zindex = it.key();
            QList<QJsonObject> s = toJSONvector(it.value(), imageType, selectedChanns, h);
            foreach (QJsonObject o, s)
            {
                o["zPos"] = it.key();
                res << o;
            }
        }

        return res;
    }
}



QMap<int, QList<QJsonObject> > SequenceFileModel::toJSONnonVector(Channel channels,
                                                                  QString imageType,
                                                                  QList<bool> selectedChanns
                                                                  , MetaDataHandler &h)
{
    QMap<int, QList<QJsonObject> > res;

    Q_UNUSED(imageType  );

    int channs = 0;
    for(Channel::iterator it = channels.begin(), e = channels.end(); it != e; ++it, ++channs)
        if (!it.value().startsWith(":/mem/") && channs < selectedChanns.size() && selectedChanns.at(channs))
        {
            QJsonObject parent;

            QJsonArray ch; ch.append(it.key());
            QJsonArray da; da.append(it.value());
            h.channel = it.key();
            parent["Channels"] = ch;
            parent["ChannelNames"] = QJsonArray::fromStringList(this->getChannelNames());
            parent["Data"] = da;

            QJsonObject props = getMeta(h);
            if (!props.isEmpty())
                parent["Properties"] = props;

            res[it.key()] << parent;
        }

    return res;
}


QMap<int, QList<QJsonObject> > SequenceFileModel::toJSONnonVector(TimeLapse times,
                                                                  QString imageType, QList<bool> selectedChanns, MetaDataHandler &h)
{
    QMap<int, QList<QJsonObject> > res;

    if (imageType.contains("Time"))
    {
        QMap<int, QJsonArray> data;
        for (TimeLapse::iterator it = times.begin(), e = times.end(); it != e; ++it)
        {
            h.timepoint = it.key();
            QMap<int, QList<QJsonObject> > v = toJSONnonVector(it.value(), imageType, selectedChanns, h);
            for( QMap<int, QList<QJsonObject> >::iterator cit = v.begin(), e = v.end(); cit != e; ++cit)
                foreach( QJsonObject o, cit.value())
                {
                    o["TimePos"] = it.key();
                    data[cit.key()].append(o);
                }
        }

        for( QMap<int, QJsonArray >::iterator cit = data.begin(), e = data.end(); cit != e; ++cit)
        {
            QJsonObject obj;
            obj["Data"] = cit.value();
            obj["TimePos"] = -1;
            res[cit.key()] << obj;
        }
        return res;
    }
    else
    {
        for (TimeLapse::iterator it = times.begin(), e = times.end(); it != e; ++it)
        {
            h.timepoint = it.key();
            QMap<int, QList<QJsonObject> > v = toJSONnonVector(it.value(), imageType, selectedChanns, h);
            for( QMap<int, QList<QJsonObject> >::iterator cit = v.begin(), e = v.end(); cit != e; ++cit)
            {
                foreach (QJsonObject o, cit.value())
                {
                    o["TimePos"] = it.key();
                    res[cit.key()] << o;
                }
            }
        }
        return res;
    }
}


QMap<int, QList<QJsonObject> > SequenceFileModel::toJSONnonVector(ImageStack stack,
                                                                  QString imageType, QList<bool> selectedChanns, MetaDataHandler &h)
{
    QMap<int, QList<QJsonObject> > res;

    if (imageType.contains("Stack"))
    {
        QMap<int, QJsonArray> data;
        for (ImageStack::iterator it = stack.begin(), e = stack.end(); it != e; ++it)
        {
            h.zindex = it.key();
            QMap<int, QList<QJsonObject> > v = toJSONnonVector(it.value(), imageType, selectedChanns, h);
            for( QMap<int, QList<QJsonObject> >::iterator cit = v.begin(), e = v.end(); cit != e; ++cit)
                foreach( QJsonObject o, cit.value())
                {
                    o["zPos"] = it.key();
                    data[cit.key()].append(o);
                }
        }

        for( QMap<int, QJsonArray >::iterator cit = data.begin(), e = data.end(); cit != e; ++cit)
        {
            QJsonObject obj;
            obj["Data"] = cit.value();
            obj["zPos"] = -1;
            res[cit.key()] << obj;
        }

        return res;
    }
    else
    {
        for (ImageStack::iterator it = stack.begin(), e = stack.end(); it != e; ++it)
        {
            h.zindex = it.key();
            QMap<int, QList<QJsonObject> > v = toJSONnonVector(it.value(), imageType, selectedChanns, h);
            for( QMap<int, QList<QJsonObject> >::iterator cit = v.begin(), e = v.end(); cit != e; ++cit)
            {
                foreach (QJsonObject o, cit.value())
                {
                    o["zPos"] = it.key();
                    res[cit.key()] << o;
                }
            }
        }
        return res;
    }
}

/*
typedef QMap<int, QString> Channel;
typedef QMap<int, Channel> TimeLapse;
typedef QMap<int, TimeLapse> ImageStack;
typedef QMap<int, ImageStack> FieldImaging;
*/


QList<QJsonObject> SequenceFileModel::toJSON(QString imageType, bool asVectorImage,
                                             QList<bool> selectedChanns, QStringList& metaData)
{
    QList<QJsonObject> res;
    MetaDataHandler handler;
    handler.metaData = metaData;


    if (asVectorImage)
    {
        if (imageType.contains("XP"))
        {
            QJsonArray data;
            for (FieldImaging::iterator it = _data.begin(), e = _data.end(); it != e; ++it)
            {
                handler.fieldIdx = it.key();
                QList<QJsonObject> s = toJSONvector(it.value(), imageType, selectedChanns, handler);
                foreach (QJsonObject o, s)
                {
                    o["FieldId"] = it.key();
                    data.append(o);
                }
            }
            QJsonObject obj;
            obj["Data"] = data;
            obj["ImageType"]=imageType;
            obj["Channel"]=-1;
            obj["FieldId"] = -1;
            obj["DataHash"] = getOwner()->hash();
            obj["Pos"] = Pos();
            obj["asVectorImage"]=true;
            obj["PlateName"] = getOwner()->name();

            if (!data.empty())
                res << obj;
            return res;
        }

        for (FieldImaging::iterator it = _data.begin(), e = _data.end(); it != e; ++it)
        {
            handler.fieldIdx = it.key();
            QList<QJsonObject> s = toJSONvector(it.value(), imageType, selectedChanns, handler);
            foreach (QJsonObject o, s)
            {
                o["FieldId"] = it.key();
                o["ImageType"]=imageType;
                o["Channel"]=-1;
                o["DataHash"] = getOwner()->hash();
                o["Pos"] = Pos();
                o["asVectorImage"]=true;
                o["PlateName"] = getOwner()->name();

                res << o;
            }
        }
        return res;
    }
    else
    {

        if (imageType.contains("XP"))
        {
            QMap<int, QJsonArray> data;
            for (FieldImaging::iterator it = _data.begin(), e = _data.end(); it != e; ++it)
            {
                handler.fieldIdx = it.key();
                QMap<int, QList<QJsonObject> > v = toJSONnonVector(it.value(), imageType, selectedChanns, handler);
                for( QMap<int, QList<QJsonObject> >::iterator cit = v.begin(), e = v.end(); cit != e; ++cit)
                    foreach( QJsonObject o, cit.value())
                    {
                        o["FieldId"] = it.key();
                        data[cit.key()].append(o);
                    }
            }

            for( QMap<int, QJsonArray>::iterator cit = data.begin(), e = data.end(); cit != e; ++cit)
            {
                QJsonObject obj;
                obj["Data"] = cit.value();
                obj["ImageType"]=imageType;
                obj["FieldId"] = -1;
                obj["DataHash"] = getOwner()->hash();
                obj["Pos"] = Pos();
                obj["asVectorImage"]=true;
                obj["PlateName"] = getOwner()->name();

                res << obj;
            }

            return res;
        }

        for (FieldImaging::iterator it = _data.begin(), e = _data.end(); it != e; ++it)
        {
            handler.fieldIdx = it.key();
            QMap<int, QList<QJsonObject> > v = toJSONnonVector(it.value(), imageType, selectedChanns, handler);
            for( QMap<int, QList<QJsonObject> >::iterator cit = v.begin(), e = v.end(); cit != e; ++cit)
            {
                foreach (QJsonObject o, cit.value())
                {
                    o["FieldId"] = it.key();
                    o["ImageType"]=imageType;
                    o["DataHash"] = getOwner()->hash();
                    o["Pos"] = Pos();
                    o["asVectorImage"]=true;
                    o["PlateName"] = getOwner()->name();

                    res << o;
                }
            }
        }
        return res;
    }

}

void SequenceFileModel::displayData()
{

    for (FieldImaging::iterator it = _data.begin(), e = _data.end(); it != e; ++it)
    {
        for (ImageStack::iterator ist = it->begin(), est = it->end(); ist != est; ++ist)
        {
            for (TimeLapse::iterator tst = ist->begin(), etst = ist->end(); tst != etst; ++tst)
            {
                for (Channel::iterator cst = tst->begin(), cest = tst->end(); cst != cest; ++cst)
                {
                    //                  qDebug() << _data.size() << it.value().size() << ist.value().size() << tst.value().size();
                    qDebug() << it.key() << ist.key() << tst.key() << cst.key() << *cst;
                }
            }

        }
    }

}


ScreensHandler &ScreensHandler::getHandler()
{
    static ScreensHandler* handler = 0;

    if (!handler) handler = new ScreensHandler();

    return *handler;

}

ExperimentFileModel* ScreensHandler::loadScreen(QString scr)
{
    ExperimentFileModel* xp = 0;
    CheckoutDataLoader& loader = CheckoutDataLoader::handler();
    //    qDebug() << "Searching: " << scr;
    if (loader.isFileHandled(scr))
    {
        xp = loader.getExperimentModel(scr);
        if (xp)
        {
            //            qDebug() << "Started";
            xp->setFileName(scr);
            _screens << xp;
            _mscreens[xp->hash()] = xp;

        }
        if (!loader.errorMessage().isEmpty())
            _error += loader.errorMessage();
    }
    return xp;
}

void ScreensHandler::addScreen(ExperimentFileModel* xp)
{
    if (!xp) return;
    _screens << xp;
    _mscreens[xp->hash()] = xp;
}
ExperimentFileModel* loadScreenFunct(QString it)
{
    Q_UNUSED(it  );

    return nullptr;
}

ExperimentFileModel* loadJson(QString fileName, ExperimentFileModel* mdl)
{
    if (!mdl)
        return mdl;
    QDir dir(fileName); dir.cdUp();
    QString tfile = dir.absolutePath() + "/tags.json";
    QStringList tags;

    QFile file;
    file.setFileName(tfile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        mdl->setMetadataPath(tfile);
        QString val = file.readAll();
        file.close();
        QStringList gtags;
        QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
        if (d.isObject())
        {

            QSettings set;
            tags = set.value("Tags", QStringList()).toStringList();
            auto json = d.object();
            if (json.contains("meta"))
            {
                auto meta = json["meta"].toObject();

                if (meta.contains("project"))
                {
                    mdl->setProperties("project", meta["project"].toString());
                }
                if (meta.contains("global_tags"))
                {
                    auto ar = meta["global_tags"].toArray();

                    for (auto i: ar) gtags << i.toString();

                    mdl->setProperties("global_tags", gtags.join(";"));
                }
                if (meta.contains("cell_lines"))
                {
                    auto ar = meta["cell_lines"].toArray();
                    QStringList ll;
                    for (auto i: ar) ll << i.toString();
                    mdl->setProperties("cell_lines", ll.join(";"));
                }
            }
            if (json.contains("map"))
            {
                auto map = json["map"].toObject();
                for (auto it = map.begin(), e = map.end(); it != e; ++it)
                {
                    QString ctag = it.key();
                    ctag = ctag.replace(';', ' ');
                    tags << ctag;
                    auto wells = it.value().toObject();
                    for (auto k = wells.begin(), ee = wells.end(); k != ee; ++k)
                    {
                        auto arr = k.value().toArray();
                        int r =  k.key().toUtf8().at(0) - 'A';
                        for (int i = 0; i < arr.size(); ++i)
                        {
                            int c = arr[i].toInt();
                            QPoint p(r, c);

                            mdl->setTag(p, ctag);

                        }
                    }
                }
            }
            if (json.contains("color_map"))
            {
                auto map = json["color_map"].toObject();
                for (auto it = map.begin(), e = map.end(); it != e; ++it)
                {
                    // tags << it.key();
                    auto wells = it.value().toObject();
                    for (auto k = wells.begin(), ee = wells.end(); k != ee; ++k)
                    {
                        auto arr = k.value().toArray();
                        int r =  k.key().toUtf8().at(0) - 'A';
                        for (int i = 0; i < arr.size(); ++i)
                        {
                            int c = arr[i].toInt();
                            QPoint p(r, c);
                            mdl->setColor(p, it.key());
                        }
                    }
                }
            }
            set.setValue("Tags", tags);
        }
    }

    return mdl;
}


// To use with blockingMapped function to load in //
ExperimentFileModel* loadScreenFunc(QString it, bool allow_loaded, Screens& _screens)
{

    bool loaded = false; // Check if file is alredy loaded
    foreach(ExperimentFileModel* mdl, _screens)
    {
        loaded = (mdl->fileName() == it);
        //          qDebug() << mdl->fileName()<< *it << loaded;
        if (loaded && allow_loaded && !mdl->displayed())
            loaded = false;
        if (loaded && allow_loaded)
            return mdl;
        if (loaded) return 0;
    }
    //      qDebug() << "is already loaded" << *it << loaded << _screens.size();
    if (loaded) return 0;
    ExperimentFileModel* res = ScreensHandler::getHandler().loadScreen(it);
    if (res)
        res->setFieldPosition();

    return loadJson(it, res);
}

Screens ScreensHandler::loadScreens(QStringList list, bool allow_loaded)
{
    _error = QString();
    // This class shall use the plugin interface to expose multiple instances of the CheckoutDataLoaderPluginInterface
#if 1

    auto func = std::bind(loadScreenFunc, std::placeholders::_1 , allow_loaded, _screens);
    Screens tmp = QtConcurrent::blockingMapped(list, func);

    Screens res;

    foreach(ExperimentFileModel * mdl, tmp)
        if (mdl)
            res << mdl;


#else

    for (QStringList::iterator it = list.begin(), e = list.end(); it != e; ++it)
    {
        bool loaded = false; // Check if file is alredy loaded
        foreach(ExperimentFileModel* mdl, _screens)
        {
            loaded = (mdl->fileName() == *it);
            //          qDebug() << mdl->fileName()<< *it << loaded;
            if (loaded && allow_loaded && !mdl->displayed())
                loaded = false;
            if (loaded && allow_loaded)
                res << mdl;
            if (loaded) break;
        }
        //      qDebug() << "is already loaded" << *it << loaded << _screens.size();
        if (loaded) continue;


        ExperimentFileModel* xp = loadScreen(*it);
        //      qDebug() << *it << xp ;
        if (xp)
            res <<    xp;
    }
#endif
    return res;
}

Screens &ScreensHandler::getScreens()
{
    return _screens;
}

ExperimentFileModel *ScreensHandler::getScreenFromHash(QString hash)
{

    foreach (ExperimentFileModel* mdl, _screens)
        if (mdl->hash() == hash)
            return mdl;

    return 0;
}


void stringToPos(QString pos, int& row, int& col)
{

    unsigned char key[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int j = 0; j < 27; ++j)
        if (pos.at(0) == key[j])
            row=j;

    col =  pos.remove(0,1).toInt()-1;
}


SequenceFileModel* ScreensHandler::addProcessResultSingleImage(QJsonObject &ob)
{

    SequenceFileModel* rmdl = 0;


    if (!ob.contains("DataHash"))
    {
        return rmdl;
    }


    qDebug() << "Handling response object:" << ob;

    if (ob["isImage"].toBool())
    {
        // Find whats is the content type & store it accordingly
        //        qDebug() << "Add result object:" << ob;
        //      if (ob.contains("DataHash") && !ob["DataHash"].toString().isEmpty())
        if (ob.contains("isOptional") && ob["optionalState"].toBool())
        {
            // Store the current object for further usage
            // Query the server for the data

            // Query the server for the data
            QJsonObject meta = ob["Meta"].toArray().at(0).toObject();

            QString processHash = ob["ProcessHash"].toString();
            int procId = ob["ProcessStartId"].toInt();


            QString hash = meta["DataHash"].toString();
            QString tag = ob["Tag"].toString();
            //    qDebug() << tag << hash <<  ob;

            if (!_mscreens.contains(hash))
            {
                qDebug() << "Cannot find original XP for hash" << hash;
                qDebug() << "#### NOT ADDING IMAGE ##########";
                return rmdl;
            }

            ExperimentFileModel* mdl =  _mscreens[hash]->getSibling( tag + procId);
            mdl->setProperties("hash", hash);
            mdl->setName(_mscreens[hash]->name());

            if (mdl->fileName().isEmpty())
            {
                mdl->setFileName(QString("%1_%3/%3_%2/dummyfile.mrf").arg(hash).arg(tag).arg(processHash));
                mdl->setProperties("hash", hash);
            }

            int col, row;
            stringToPos(meta["Pos"].toString(), row, col);
            // qDebug() << ob;


            if (ob["ContentType"].toString() == "Image")
            {

                SequenceFileModel& seq = (*mdl)(row, col);
                seq.setOwner(mdl);
                seq.setProcessResult(true);
                seq.setDisplayStatus(ob["shallDisplay"].toBool());
                (*_mscreens[hash])(row, col).addSibling(&seq);

                ExperimentFileModel* sr = (*_mscreens[hash])(row, col).getOwner();
                if (!sr) return 0;
                for (int i = 1; i < 10; ++i)
                {
                    QString prop = QString("ChannelsColors%1").arg(i);
                    if (sr->hasProperty(prop))        {
                        qDebug() << "Property : " << sr->property(prop);
                        mdl->setProperties(prop, sr->property(prop));
                    }
                }

                QJsonArray res = ob["Payload"].toArray();

                for (int item = 0; item < res.count(); ++item)
                {
                    QString hash = QString("%1%2").arg( ob["DataHash"].toString(), item == 0 ? "" : QString("%1").arg(item));
                    std::vector<unsigned char> data = CheckoutProcess::handler().detachPayload(hash);
                    if (data.size() == 0) {
                        qDebug() << "Data" << hash << " not fetched yet, awaiting";
                        continue;
                    }

                    QJsonObject payload = res.at(item).toObject();

                    QJsonArray datasizes =  payload["DataSizes"].toArray();
                    unsigned size = 0;
                    for (int i = 0; i < datasizes.size(); ++i)
                        size += datasizes.at(i).toInt();

                    if (size == 0)
                    {
                        if (data.size() != size)
                        {
                            qDebug() << "Error while gathering data (expected" << size << "received" << data.size() << ")";
                            return rmdl;
                        }
                        else
                            return rmdl;
                    }

                    unsigned long long chans = 0;
                    for (int i = 0; i < datasizes.size(); ++i)
                    {

                        int r = payload["Rows"].toInt(), c = payload["Cols"].toInt();
                        int cvtype = payload["cvType"].toInt();

                        unsigned long long len = r * c * payload["DataTypeSize"].toInt() ;
                        cv::Mat im(r, c, cvtype, &(data.data()[chans]));


                        //   cv::imwrite(QString("c:/temp/im%1.jpg").arg(hash).toStdString(), im*255);

                        cv::Mat* m = new cv::Mat();

                        (*m) = im.clone();

                        chans += len;
                        if (!m)
                        {
                            qDebug() << "Memory error, Image not created !";
                            return rmdl;
                        }
                        int t = payload.contains("Time") ? payload["Time"].toInt() : meta["TimePos"].toInt(),
                                f = meta["FieldId"].toInt(),
                                z = meta["zPos"].toInt(),
                                ch = meta["channel"].toInt();

                        int cc = datasizes.size() > 1  ? i+1 : ch;

                        QString fname =  QString(":/mem/%1_%2_%3_%4_T%5F%6Z%7C%8.png")
                                .arg(hash)
                                .arg(ob["Tag"].toString())
                                .arg(processHash)
                                .arg(meta["Pos"].toString())
                                .arg(t)
                                .arg(f)
                                .arg(z)
                                .arg(cc)
                                ;

                        MemoryHandler::handler().addData(fname, m);
                        seq.addFile(t, f, z, cc, fname);

                        rmdl = &seq;
                        _result_images << fname;

                        if (ob.contains("Colormap"))
                        {
                            QMap<unsigned, QColor> color;

                            QJsonObject obj = ob["Colormap"].toObject();
                            for (auto it = obj.begin(); it != obj.end(); ++it)
                            {
                                QColor col;
                                col.setNamedColor(it.value().toString());
                                color[it.key().toInt()] = col;
                            }
                            MemoryHandler::handler().addColor(fname, color);
                        }
                        if (ob.contains("ChannelNames"))
                        {
                            QStringList names;
                            QJsonArray ar = ob["ChannelNames"].toArray();
                            for (int n = 0; n < ar.size(); ++n)
                                names << ar[n].toString();
                            mdl->setChannelNames(names);
                        }
                    }
                }
            }
            else
            { // Handle other types of results, points + features & Box + features expected here
                qDebug() << "Need to handle overlay content Type";
                SequenceFileModel& seq = (*_mscreens[hash])(row, col);

                QJsonArray res = ob["Payload"].toArray();

                for (int item = 0; item < res.count(); ++item)
                {
                    QString hash = QString("%1%2").arg( ob["DataHash"].toString(), item == 0 ? "" : QString("%1").arg(item));
                    std::vector<unsigned char> data = CheckoutProcess::handler().detachPayload(hash);
                    if (data.size() == 0) {
                        qDebug() << "Data" << hash << " not fetched yet, awaiting";
                        continue;
                    }

                    QJsonObject payload = res.at(item).toObject();

                    QJsonArray datasizes =  payload["DataSizes"].toArray();
                    unsigned size = 0;
                    for (int i = 0; i < datasizes.size(); ++i)
                        size += datasizes.at(i).toInt();

                    if (size == 0)
                    {
                        if (data.size() != size)
                        {
                            qDebug() << "Error while gathering data (expected" << size << "received" << data.size() << ")";
                            return rmdl;
                        }
                        else
                            return rmdl;
                    }

                    unsigned long long chans = 0;
                    for (int i = 0; i < datasizes.size(); ++i)
                    {

                        int r = payload["Rows"].toInt(), c = payload["Cols"].toInt();
                        int cvtype = payload["cvType"].toInt();

                        unsigned long long len = r * c * payload["DataTypeSize"].toInt() ;
                        cv::Mat im(r, c, cvtype, &(data.data()[chans]));

                        chans += len;

                        int t = payload.contains("Time") ? payload["Time"].toInt() : meta["TimePos"].toInt(),
                                f = meta["FieldId"].toInt(),
                                z = meta["zPos"].toInt(),
                                ch = meta["channel"].toInt();

                        int cc = datasizes.size() > 1  ? i+1 : ch;

                        QString fname =  QString(":/mem/%1_%2_%3_%4_T%5F%6Z%7C%8.png")
                                .arg(hash)
                                .arg(ob["Tag"].toString())
                                .arg(processHash)
                                .arg(meta["Pos"].toString())
                                .arg(t)
                                .arg(f)
                                .arg(z)
                                .arg(cc)
                                ;


                        StructuredMetaData data;
                        data.setContent(im.clone());
                        if (ob.contains("ChannelNames"))
                        {
                            QStringList names;
                            QJsonArray ar = ob["ChannelNames"].toArray();
                            for (int n = 0; n < ar.size(); ++n)
                                names << ar[n].toString();
                            QString tt = names.join(";");
                            data.setProperties("ChannelNames", tt);
                        }

                        seq.addMeta(t < 1 ? 1 : t, f < 1 ? 1 : f, z < 1 ? 1 : z, cc < 0 ? 1 : cc, ob["Tag"].toString(), data);
        
                    }

                }


                rmdl = &seq;
            }

        }
    }


    return rmdl;
}

//QMutex ScreensHandler::_mutex;


//SequenceFileModel* ScreensHandler::addProcessResultSingleImage(QJsonObject& ob,  QString processHash, int procId, bool displayed)
//{
//    SequenceFileModel* rmdl = 0;

//    return rmdl;

//}


QList<SequenceFileModel*> ScreensHandler::addProcessResultImage(QJsonObject& data)
{
    QList<SequenceFileModel*> mdls;

    //  qDebug() << data["CommitName"];

    QJsonArray ar = data["Data"].toArray();
    //    int procId = data["ProcessStartId"].toInt();
    QString processHash = data["Hash"].toString();
    //    bool displayed = data["shallDisplay"].toBool();
    for (int i = 0; i < ar.size(); ++i)
    {
        QJsonObject ob = ar.at(i).toObject();

        SequenceFileModel* mdl = addProcessResultSingleImage(ob);// , processHash, procId, displayed);

        if (mdl && !mdls.contains(mdl))
            mdls << mdl;

    }
    return mdls;
}

QList<QString>& ScreensHandler::getTemporaryFiles()
{
    return _result_images;
}

QString &ScreensHandler::errorMessage()
{
    return _error;
}



SequenceViewContainer::SequenceViewContainer():  _current(0)
{
}

SequenceViewContainer &SequenceViewContainer::getHandler()
{
    static SequenceViewContainer* cont = 0;
    if (!cont) cont = new SequenceViewContainer;

    return *cont;
}

void SequenceViewContainer::addSequence(SequenceFileModel *mdl)
{
    _curView << mdl;
}

void SequenceViewContainer::removeSequence(SequenceFileModel *mdl)
{ // Fix: Bug #31
    mdl->setSelectState(false);
    _curView.removeOne(mdl);
}


QList<SequenceFileModel *>& SequenceViewContainer::getSequences()
{
    return _curView;
}

SequenceFileModel *SequenceViewContainer::current()
{
    return _current;
}



void SequenceViewContainer::setCurrent(SequenceFileModel *mdl)
{
    if (!_curView.contains(mdl))
        _curView << mdl;
    _current = mdl;
}

QMutex ExperimentDataModel::_lock(QMutex::NonRecursive);

ExperimentDataTableModel::ExperimentDataTableModel(ExperimentFileModel *parent, int nX, int nY): _owner(parent), nbX(nX), nbY(nY)
{

}

QPoint ExperimentDataTableModel::stringToPos(QString Spos)
{
    QPoint r;

    r.setX((int)Spos.at(0).toLatin1()-'A');
    r.setY(Spos.remove(0,1).toUInt()-1);

    return r;
}
// Warning do not mix row / col order...
QString ExperimentDataTableModel::posToString(int x, int y) const
{
    unsigned char key[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    return QString("%1%2").arg(QChar(key[x])).arg(y+1, 2, 10, QLatin1Char('0'));
}

QString ExperimentDataTableModel::posToString(const QPoint p) const
{
    return posToString(p.x(), p.y());
}


unsigned ExperimentDataTableModel::posToInt(int x, int y) const
{
    return x + y * (unsigned)('Z' - 'A');
}

unsigned ExperimentDataTableModel::posToInt(const QPoint p) const
{
    return posToInt(p.x(), p.y());
}


QPoint ExperimentDataTableModel::intToPos(unsigned x) const
{
    unsigned dd = (unsigned)('Z' - 'A');
    QPoint p;
    p.setX(x % dd);
    p.setY(x / dd);
    return p;
}



void ExperimentDataTableModel::addData(QString XP, int field, int stackZ, int time, int chan, int x, int y, double data)
{
    QVector<double> d; d.push_back(data);
    addData(XP, field, stackZ, time, chan, x, y, d);
}

void ExperimentDataTableModel::addData(QString XP, int field, int stackZ, int time, int chan, QString pos, double data)
{
    addData(XP, field, stackZ, time, chan, stringToPos(pos), data);
}

void ExperimentDataTableModel::addData(QString XP, int field, int stackZ, int time, int chan, QPoint pos, double data)
{
    addData(XP, field, stackZ, time, chan, pos.x(), pos.y(), data);
}


void ExperimentDataTableModel::addData(QString XP, int field, int stackZ, int time, int chan, QString pos, QVector<double> data)
{
    addData(XP, field, stackZ, time, chan, stringToPos(pos), data);
}

void ExperimentDataTableModel::addData(QString XP, int field, int stackZ, int time, int chan, QPoint pos, QVector<double> data)
{
    addData(XP, field, stackZ, time, chan, pos.x(), pos.y(), data);
}


/*
-- Try to update any existing row
UPDATE players
SET user_name='steven', age=32
WHERE user_name='steven';

-- If no update happened (i.e. the row didn't exist) then insert one
INSERT INTO players (user_name, age)
SELECT 'steven', 32
WHERE (Select Changes() = 0);*/


void ExperimentDataTableModel::addData(QString XP, int field, int stackZ, int time, int chan, int x, int y, QVector<double> data)
{
    //  qDebug() << "add Data" << XP << field << stackZ << time << chan << x << y;
    static quint64 MaxY = 50, MaxChan = 30, MaxTime = 1000, MaxField = 100, MaxZ = 1000;
    DataHolder h;
    bool newCol = false;
    QModelIndex idx;

    if (!_datanames.contains(XP))
    {

        QSet<QString>::const_iterator pos = _datanames.insert(XP);
        int col = std::distance(_datanames.cbegin(), pos);
        beginInsertColumns(idx, 5 + (int)_owner->hasTag() + col, 5 + (int)_owner->hasTag() + col);

        _datanames.insert(XP);
        headerDataChanged(Qt::Horizontal, col, col);
        newCol = true;

    }

    // Compute signature:
    h.signature = x + y * MaxY
            + chan *   MaxChan * MaxY
            + time *   MaxChan * MaxY * MaxTime
            + stackZ * MaxChan * MaxY * MaxTime * MaxZ
            + field  * MaxChan * MaxY * MaxTime * MaxZ * MaxField;
    //  qDebug() << h.signature;

    int pos = 0;
    for (QList<DataHolder>::iterator it = _dataset.begin(), e = _dataset.end(); it != e; ++it, ++pos)
    {
        DataHolder& l =  *it;
        if (l.signature == h.signature)
        {
            l.data[XP] = data;
            QString xp = XP;
            QSet<QString>::iterator cpos = _datanames.insert(xp);
            int col = std::distance(_datanames.begin(), cpos);

            if (newCol)      endInsertColumns();

            dataChanged(index(pos, 5+_owner->hasTag()+col), index(pos, 5+_owner->hasTag()+col));

            return;
        }
    }

    beginInsertRows(idx, _dataset.size(), _dataset.size());

    h.data[XP] = data;

    h.field = field;
    h.stackZ = stackZ;
    h.time = time;
    h.chan = chan;


    fields.insert(field);
    stacks.insert(stackZ);
    times.insert(time);
    chans.insert(chan);


    h.pos.setX(x);
    h.pos.setY(y);

    h.tags = _owner->getTags(h.pos).join(";");

    _dataset << h;

    endInsertRows();
    if (newCol)  endInsertColumns();
}


QVector<double> ExperimentDataTableModel::getData(QString XP, int field, int stackZ, int time, int chan, int x, int y)
{
    static unsigned MaxY = 50, MaxChan = 30, MaxTime = 1000, MaxField = 100, MaxZ = 1000;
    DataHolder h;

    // Compute signature:
    h.signature = x + y * MaxY
            + chan *   MaxChan * MaxY
            + time *   MaxChan * MaxY * MaxTime
            + stackZ * MaxChan * MaxY * MaxTime * MaxZ
            + field  * MaxChan * MaxY * MaxTime * MaxZ * MaxField;

    for (QList<DataHolder>::iterator it = _dataset.begin(), e = _dataset.end(); it !=e ; ++it)
        if (it->signature == h.signature)
        {
            return it->data[XP];
        }

    return QVector<double>();
}

QVector<double> ExperimentDataTableModel::getData(QString XP, int field, int stackZ, int time, int chan, QPoint pos)
{
    return  getData(XP, field, stackZ, time, chan, pos.x(), pos.y());
}

QVector<double> ExperimentDataTableModel::getData(QString XP, int field, int stackZ, int time, int chan, QString pos)
{
    return getData(XP,field, stackZ, time, chan, stringToPos(pos));
}



QStringList ExperimentDataTableModel::getExperiments() const
{
    return _datanames.values();
}

int ExperimentDataTableModel::getExperimentCount() const
{
    //  qDebug() << _datanames << _datanames.size();
    return _datanames.size();
}

int ExperimentDataTableModel::getDataSize() const
{
    return _dataset.size();
}

void ExperimentDataTableModel::setCommitName(QString str) { _commitName = str; }

QString ExperimentDataTableModel::getCommitName() { return _commitName; }

void ExperimentDataTableModel::setAggregationMethod(QString Xp, QString method)
{
    _aggregation[Xp] = method;
}


// returns true if the table was added
bool checkOrAlterTable(QSqlQuery& q, QString table, QString col);

void ExperimentDataTableModel::bindValue(QSqlQuery& select, DataHolder& h, QString key )
{
    QString pos =  posToString(h.pos);

    select.bindValue(":fieldId", h.field);
    select.bindValue(":slice", h.stackZ);
    select.bindValue(":time", h.time);
    select.bindValue(":channel", h.chan);
    select.bindValue(":id",pos);
    if (!key.isEmpty())
        select.bindValue(":data", h.data[key].first()); // FIXME: Only single dimension data is stored...

}


double AggregateSum(QList<double>& f)
{

    if (f.size() == 1) return f.at(0);

    double r = 0;
    foreach(double ff, f)
        r+= ff;

    return r;
}
double AggregateMean(QList<double>& f)
{
    if (f.size() == 1) return f.at(0);
    double r = 0;
    foreach(double ff, f)
        r+= ff;
    r /= (double)f.size();
    return r;
}




double AggregateMedian(QList<double>& f)
{
    if (f.size() == 1) return f.at(0);
    std::sort(f.begin(), f.end());
    return f.at(f.size()/2);
}

double AggregateMin(QList<double>& f)
{
    if (f.size() == 1) return f.at(0);
    double r = std::numeric_limits<double>::max();
    foreach(double ff, f)
        if (ff < r) r = ff;
    return r;
}

double AggregateMax(QList<double>& f)
{
    if (f.size() == 1) return f.at(0);
    double r = std::numeric_limits<double>::min();
    foreach(double ff, f)
        if (ff > r) r = ff;
    return r;
}


double Aggregate(QList<double>& f, QString& ag)
{
    if (ag == "Sum") return AggregateSum(f);
    if (ag == "Mean") return AggregateMean(f);
    if (ag == "Median") return AggregateMedian(f);
    if (ag == "Min") return AggregateMin(f);
    if (ag == "Max") return AggregateMax(f);

    qDebug() << "Aggregation error!" << ag << "Not found";
    return -1.;
}

int ExperimentDataTableModel::commitToDatabase(QString hash, QString prefix)
{
    if (prefix.isEmpty()) return 0;
    int linecounter = 0;

    QSettings set;
    QString dataname;
    QMap<unsigned, QMap<QString, QList<double> >    >  factor;

    foreach (QString key, _datanames)
    {
        dataname += QString(",%1").arg(key);
    }

    {
        QDir dir(set.value("databaseDir").toString());
        // 20201210: Large behaviour change
        // Now: Assuming the following reordering:
        // dir + {tag.project} + Checkout_Results/ + prefix + / PlateName + .csv
        // If file exists move previous file with a post_fix info
        QString writePath = QString("%1/%2/Checkout_Results/%3/").arg(dir.absolutePath()).arg(_owner->property("project")).arg(prefix)
                ;
        QString fname =  _owner->name() +".csv";

        dir.mkpath(writePath);
        QFile file(writePath + fname);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return -1;

        QTextStream resFile(&file);

        resFile << "Plate,Well,timepoint,fieldId,sliceId,channel,tags" << dataname << Qt::endl;

        for (QList<DataHolder>::iterator it = _dataset.begin(), e = _dataset.end(); it !=e ; ++it)
        {
            DataHolder& h = *it;
            QString pos =  posToString(h.pos);
            resFile << _owner->name() << "," << pos << "," << h.field << ","
                    << h.stackZ << "," << h.time << ","
                    << h.chan << "," << (_owner->getTags(h.pos).join(";"));


            foreach (QString key, _datanames)
            {
                double v = h.data[key].first();
                resFile << "," << v ;

                factor[posToInt(h.pos)][key] << v;
            }
            //        qDebug() << prep.lastQuery() << prep.boundValues();
            resFile << Qt::endl;
            linecounter++;
        }
        resFile.flush();
        file.close();
        qDebug() << "Writing " << linecounter << "Data from" << _owner->name() << "to" << fname;
    }


    {
        QDir dir(set.value("databaseDir").toString());

        QString writePath = QString("%1/%2/Checkout_Results/%3/").arg(dir.absolutePath()).arg(_owner->property("project")).arg(prefix);
        QString fname ="ag" +  _owner->name() +".csv";
        QFile file(writePath+fname);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return -1;

        QTextStream resFile(&file);

        resFile << "Plate,Well,tags" << dataname << Qt::endl;

        for (QMap<unsigned, QMap<QString, QList<double> >    >::iterator it = factor.begin(), e = factor.end();
             it != e; ++it)
        {
            QPoint npos = intToPos(it.key());
            QString pos = posToString(npos);
            resFile << _owner->name() << "," << pos << "," << (_owner->getTags(npos).join(";"));

            foreach (QString key, _datanames)
            {
                double v = Aggregate(it.value()[key], getAggregationMethod(key));
                resFile << "," << v;
            }
            resFile << Qt::endl;
        }

        resFile.flush();
        file.close();

    }

    if (!_owner->getMetadataPath().isEmpty())
    {
        QFile meta(_owner->getMetadataPath());
        QDir dir(set.value("databaseDir").toString());


        QString writePath = QString("%1/%2/Checkout_Results/%3/").arg(dir.absolutePath()).arg(_owner->property("project")).arg(prefix);

        meta.copy(writePath + "/"+ _owner->name() + "_tags.json");
    }

    return linecounter;
}


QVariant ExperimentDataTableModel::getData(int row, QString XP) const
{
    const QList<DataHolder>& dh = _dataset;

    if (row < dh.size())
    {
        const DataHolder& d = dh[row];
        if (d.data[XP].size())
            return d.data[XP].first();
    }
    return QString("-");
}

QString ExperimentDataTableModel::getMeta(int row, int col) const
{
    //  qDebug() << "Get Meta" << row << col;

    if (row > _dataset.size()) return QString();

    const DataHolder& dh = _dataset[row];
    if (col == 0)    return posToString(dh.pos);
    if (col == 1)    return QString("%1").arg(dh.time);
    if (col == 2)    return QString("%1").arg(dh.field);
    if (col == 3)    return QString("%1").arg(dh.stackZ);
    if (col == 4)    return QString("%1").arg(dh.chan);
    if (_owner->hasTag() && col == 5) return dh.tags;
    return QString();
}

QString& ExperimentDataTableModel::getAggregationMethod(QString XP)
{
    return _aggregation[XP];
}

int ExperimentDataTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent  );

    // Let's count this...
    int res = this->getDataSize();

    return res;
}

int ExperimentDataTableModel::columnCount(const QModelIndex &/* parent*/) const
{
    int res = 5+_owner->hasTag();
    res += this->getExperimentCount();

    return res;
}


QVariant ExperimentDataTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() )
        return QVariant();


    if (role == Qt::DisplayRole)
    {
        if (index.column() < 5 + _owner->hasTag())
            return QVariant(this->getMeta(index.row(), index.column()));
        QString hdata = headerData(index.column(), Qt::Horizontal).toString();
        //       qDebug() << index.row() << hdata;
        return QVariant(this->getData(index.row(), hdata));
    }
    else
        return QVariant();
}


QVariant ExperimentDataTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QString data[] = {"Pos", "Time", "Field", "Z", "Channel", "#Tags"};
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0)
    {
        if (section < 5 +_owner->hasTag() )
        {
            return data[section];
        }
        else
            if (this->getExperiments().size() > (section - (5+_owner->hasTag())))
            {
                return this->getExperiments().at(section-(5+_owner->hasTag()));
            }
    }

    return QVariant();
}

void ExperimentDataTableModel::clearAll()
{

    QModelIndex idx;
    beginRemoveColumns(idx, 5+_owner->hasTag(), this->columnCount());
    beginRemoveRows(idx, 0, _dataset.size());

    _datanames.clear();
    _dataset.clear();

    endRemoveRows();
    endRemoveColumns();
}

int ExperimentDataTableModel::exposeDataColumnCount(QStringList memlist, QStringList dblist)
{
    Q_UNUSED(dblist  );


    int memcolumnCount =  fields.size() * stacks.size() * times.size() * chans.size() * memlist.size();
    int dbcolumnCount = 0;

    QStringList l = _owner->getDatabaseNames();
    for (int i = 0; i < l.size(); ++i)
    {
        ExperimentDataModel* edm =  _owner->databaseDataModel(l.at(i));
        dbcolumnCount += edm->columnCount();
    }

    return dbcolumnCount + memcolumnCount;

}

MatrixDataModel *ExperimentDataTableModel::exposeData(QStringList memlist, QStringList dblist)
{
    //  qDebug() << "Exposing Data";
    MatrixDataModel* datamodel = 0;
    int columnCount = exposeDataColumnCount(memlist, dblist);
    if (columnCount)
        datamodel = new MatrixDataModel(columnCount);

    if (memlist.size())
        exposeMemoryData(memlist, datamodel);
    if (dblist.size())
        exposeDatabaseData(dblist, datamodel);

    return datamodel;
}

MatrixDataModel *ExperimentDataTableModel::exposeData(QStringList memlist, QStringList dblist, QVector<QList<int> > &subdata)
{
    // qDebug() << "Exposing Data";
    MatrixDataModel* datamodel = 0;
    int columnCount = exposeDataColumnCount(memlist, dblist);
    // qDebug() << columnCount;
    if (columnCount)
        datamodel = new MatrixDataModel(columnCount);

    if (memlist.size())
        exposeMemoryData(memlist, datamodel,subdata);
    if (dblist.size())
        exposeDatabaseData(dblist, datamodel,subdata);

    return datamodel;
}


MatrixDataModel *ExperimentDataTableModel::exposeMemoryData(QStringList xps,
                                                            MatrixDataModel* datamodel,
                                                            QVector<QList<int> >& subdata )
{
    for (int i = 0; i < _dataset.size(); ++i)
    {
        DataHolder& d = _dataset[i];
        if (!subdata[MatrixDataModel::Field].empty() && !subdata[MatrixDataModel::Field].contains(d.field)) continue;
        if (!subdata[MatrixDataModel::Stack].empty() && !subdata[MatrixDataModel::Stack].contains(d.stackZ)) continue;
        if (!subdata[MatrixDataModel::Time].empty() && !subdata[MatrixDataModel::Time].contains(d.time)) continue;
        if (!subdata[MatrixDataModel::Channel].empty() && !subdata[MatrixDataModel::Channel].contains(d.chan)) continue;

        foreach (QString xp, xps)
        {
            datamodel->addData(xp, d.field, d.stackZ, d.time, d.chan, d.pos.x(), d.pos.y(), d.data[xp].first());
        }
    }


    return datamodel;
}

MatrixDataModel *ExperimentDataTableModel::exposeMemoryData(QStringList xps,
                                                            MatrixDataModel* datamodel )
{
    //    qDebug() << "Expose Memory Data!!!" << xps << subdata[0] << subdata[1] << subdata[2] <<subdata[3];
    for (int i = 0; i < _dataset.size(); ++i)
    {
        DataHolder& d = _dataset[i];


        foreach (QString xp, xps)
        {
            //          qDebug() << xp << d.field << d.stackZ << d.time << d.chan << d.pos << d.data[xp];
            datamodel->addData(xp, d.field, d.stackZ, d.time, d.chan, d.pos.x(), d.pos.y(), d.data[xp].first());
        }
    }


    return datamodel;
}


MatrixDataModel *ExperimentDataTableModel::exposeDatabaseData(QStringList xps,
                                                              MatrixDataModel *datamodel,
                                                              QVector<QList<int> > &subdata)
{

    //qDebug() << "filtered" << xps;

    foreach (QString n, xps)
    {
        QStringList d = n.split("/");
        if (d.size() == 1)
        {
            ExperimentDataModel* mdl = _owner->databaseDataModel(n);
            mdl->exposeMemoryData(QStringList() << n, datamodel, subdata);
        }
        else
        {
            ExperimentDataModel* mdl = _owner->databaseDataModel(d.at(0));
            mdl->exposeMemoryData(QStringList() << d.at(1), datamodel, subdata);
        }

    }

    return datamodel;
}

MatrixDataModel *ExperimentDataTableModel::exposeDatabaseData(QStringList xps,
                                                              MatrixDataModel *datamodel)
{
    // qDebug() << "Unfiltered" << xps;
    foreach (QString n, xps)
    {
        ExperimentDataModel* mdl = _owner->databaseDataModel(n);
        mdl->exposeMemoryData(xps, datamodel);
    }


    return datamodel;
}



MemoryHandler &MemoryHandler::handler()
{
    static MemoryHandler* hdl = 0;
    if (!hdl) hdl = new MemoryHandler;
    return *hdl;
}

void MemoryHandler::addData(QString vfs, void *data)
{
    _data[vfs] = data;
}

void *MemoryHandler::getData(QString vfs)
{

    return _data[vfs];
}

QMap<unsigned, QColor> MemoryHandler::getColor(QString vfs)
{
    if (_cmap.contains(vfs))
        return _cmap[vfs];
    return QMap<unsigned,QColor>();
}

void MemoryHandler::addColor(QString vfs, QMap<unsigned, QColor> color)
{
    _cmap[vfs] = color;
}

void MemoryHandler::release(QString vfs)
{
    _data.remove(vfs);
    if (_cmap.contains(vfs))
        _cmap.remove(vfs);
}

StructuredMetaData::StructuredMetaData(Dictionnary dict): DataProperty(dict)
{
}

cv::Mat &StructuredMetaData::content() { return _content; }

void StructuredMetaData::setContent(cv::Mat cont)
{
    // Ensure datatype is CV_32F as later on we use it forced to float
    if (cont.type() != CV_32F)
        cont.convertTo(_content, CV_32F);
    else
        _content = cont;
}
