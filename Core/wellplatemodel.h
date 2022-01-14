#ifndef WELLPLATEMODEL_H
#define WELLPLATEMODEL_H


#include <QFile>
#include <QTextStream>

#include <QDebug>

#include <QString>

#include <QHash>
#include <QMutex>

#include <QMap>

#include <QSize>
#include <QPoint>

#include <QAbstractTableModel>
#include <QFileInfo>

#include "Core/Dll.h"

#include <opencv2/opencv.hpp>

class MemoryHandler
{
public:

    static MemoryHandler& handler();

    void addData(QString vfs, void* data);


    void* getData(QString vfs);

    QMap<unsigned, QColor> getColor(QString vfs);
    void addColor(QString vfs, QMap<unsigned, QColor> color);

    void release(QString vfs);

protected:
    QMap<QString, void*> _data;
    QMap<QString, QMap<unsigned, QColor> > _cmap;


};


class DllCoreExport Dictionnary: public QMap<QString, QString>
{
public :
    Dictionnary()
    {}
};


class DllCoreExport DataProperty
{
public:
    DataProperty(Dictionnary dict ): _dict(dict)
    {
    }
    virtual ~DataProperty() {}

    virtual void setProperties(QString tag, QString value);
    virtual QString property(QString tag) const;
    virtual QString property(QRegExp& re) const;
    virtual QString properties();
    virtual bool hasProperty(QString tag) const;

protected:
    Dictionnary             _dict;
    QMap<QString, QVariant> _properties;
};

class ExperimentFileModel;

class DllCoreExport StructuredMetaData: public DataProperty
{ // Place holder for cv::Mat used for Histograms, Roi, Roi_CC_Stats (from opencv connectedCompo)

public:
    StructuredMetaData(Dictionnary dict = Dictionnary());
    ~StructuredMetaData();

    cv::Mat& content();
    void setContent(cv::Mat cont);
    void setTag(int id, QStringList d);
    QStringList getTag(int id);
    size_t length();
    void exportData();

   protected:
    cv::Mat _content;
    QVector<QStringList> _tags;

};

class DllCoreExport SequenceFileModel: public DataProperty
{
public:
    SequenceFileModel(Dictionnary dict = Dictionnary());

    // For imaging
    typedef QMap<int, QString> Channel;
    typedef QMap<int, Channel> TimeLapse;
    typedef QMap<int, TimeLapse> ImageStack;
    typedef QMap<int, ImageStack> FieldImaging;

    // For structured metadata content
    typedef QMap<int, QMap<QString, StructuredMetaData> > SChannel;
    typedef QMap<int, SChannel> STimeLapse;
    typedef QMap<int, STimeLapse> SImageStack;
    typedef QMap<int, SImageStack> SFieldImaging;




    unsigned getTimePointCount() const;
    unsigned getFieldCount() const;
    unsigned getZCount() const;
    unsigned getChannels() const;

    // Returns the Field at time position
    //  MeasurementRecordFileModel& getAcquisition(int Field);

    void addFile(int timePoint, int fieldIdx, int Zindex, int Channel, QString file);
    bool hasFile(int timePoint, int fieldIdx, int Zindex, int Channel);


    void addMeta(int timePoint, int fieldIdx, int Zindex, int Channel, QString name, StructuredMetaData meta);

    QStringList getAllFiles();


    // This model need to handle data set as well
    QString getFile(int timePoint, int fieldIdx, int Zindex, int channel);
    QString getFileChanId(int timePoint, int fieldIdx, int Zindex, int channel);

    Channel& getChannelsFiles(int timePoint, int fieldIdx, int Zindex);
    QSet<int> getChannelsIds();

    StructuredMetaData& getMeta(int timePoint, int fieldIdx, int Zindex, int channel, QString name);

    int getMetaChannels(int timePoint, int fieldIdx, int Zindex);
    QMap<QString, StructuredMetaData>& getMetas(int timePoint, int fieldIdx, int Zindex, int channel);

//    QStringList getMetaNames(int timePoint, int fieldIdx, int Zindex, int channel);
//    bool hasMeta(int timePoint, int fieldIdx, int Zindex, int channel, QString name = QString());

    void removeMeta();

    void setOwner(ExperimentFileModel* ow);

    ExperimentFileModel* getOwner() const;

    QList<QJsonObject> toJSON(QString imageType, bool asVectorImage, QList<bool> selectedChanns, QStringList &metaData);

    void displayData();

    bool hasChannel(int timePoint, int fieldIdx, int Zindex, int channel);

    void setPos(unsigned r, unsigned c);
    QPoint pos();
    QString Pos() const;

    bool isAlreadyShowed();
    void setAsShowed(bool showed=true);

    bool toDisplay();
    void setDisplayStatus(bool disp);

    bool isProcessResult();
    void setProcessResult(bool val);

    void addSibling(SequenceFileModel* mdl);
    QList<SequenceFileModel*> getSiblings();

    virtual QString property(QString tag) const;
    virtual QString property(QRegExp& tag) const;

    void setInvalid();
    bool isValid();
    void checkValidity();
    bool hasMeasurements();
    void setSelectState(bool val);
    bool isSelected();

    void setTag(QString tag);
    void unsetTag(QString tag);
    void clearTags();
    QStringList getTags();

    void setChannelNames(QStringList names);
    QStringList getChannelNames();

    void setColor(QString col);
    QString getColor();

protected:



    bool _isValid;
    bool _isShowed;//, _isProcessResult;
    bool _toDisplay, _isProcessResult;
    unsigned _row, _col;
    FieldImaging _data;
    SFieldImaging _sdata;

    ExperimentFileModel* _owner;

    QList<SequenceFileModel*> _siblings;
    QSet<int> _channelsIds;
    QStringList _tags;
    QString _color;

private:

    struct MetaDataHandler
    { // Struct to pass current indexes for the metadata preparation
        QStringList metaData;
        int fieldIdx, zindex, timepoint, channel;

    };


    QMap<int, QList<QJsonObject> > toJSONnonVector(ImageStack stack, QString imageType, QList<bool> selectedChanns, MetaDataHandler& h);
    QMap<int, QList<QJsonObject> > toJSONnonVector(TimeLapse times, QString imageType, QList<bool> selectedChanns, MetaDataHandler& h);
    QMap<int, QList<QJsonObject> > toJSONnonVector(Channel channels, QString imageType, QList<bool> selectedChanns, MetaDataHandler& h);



    QList<QJsonObject> toJSONvector(ImageStack stack, QString imageType, QList<bool> selectedChanns, MetaDataHandler& h);
    QList<QJsonObject> toJSONvector(TimeLapse times, QString imageType, QList<bool> selectedChanns, MetaDataHandler& h);
    QList<QJsonObject> toJSONvector(Channel channels, QString imageType, QList<bool> selectedChanns, MetaDataHandler& h);


    QJsonObject getMeta(MetaDataHandler& h);
    QJsonObject getMeta(QStringList& keys, QString prefix);

};

class DllCoreExport SequenceFileModelOverlay
{

};


class DllCoreExport SequenceViewContainer
{
public:
    SequenceViewContainer();

    static SequenceViewContainer& getHandler();



    QList<SequenceFileModel *> &getSequences();

    SequenceFileModel* current();

    void setCurrent(SequenceFileModel* mdl);
    void addSequence(SequenceFileModel* mdl);
    void removeSequence(SequenceFileModel *mdl);


protected:
    QList<SequenceFileModel*> _curView;
    SequenceFileModel* _current;
};


class MatrixDataModel;
class QSqlQuery;

class DllCoreExport ExperimentDataTableModel: public QAbstractTableModel
{
    Q_OBJECT
    struct DataHolder
    {
        QPoint pos;
        int time, field, stackZ, chan;
        quint64 signature;
        QString tags;

        QMap<QString, QVector<double > > data;
    };

public:
    ExperimentDataTableModel(ExperimentFileModel* parent, int nX, int nY);
    static QPoint stringToPos(QString Spos);

    QString posToString(int x, int y) const;
    QString posToString(const QPoint p) const;

    QPoint intToPos(unsigned x) const;

    unsigned posToInt(int x, int y) const;
    unsigned posToInt(const QPoint p) const;



    void addData(QString XP, int field, int stackZ, int time, int chan, int x, int y, double data);
    void addData(QString XP, int field, int stackZ, int time, int chan, QPoint pos, double data);
    void addData(QString XP, int field, int stackZ, int time, int chan, QString pos, double data);

    void addData(QString XP, int field, int stackZ, int time, int chan, int x, int y, QVector<double> data);
    void addData(QString XP, int field, int stackZ, int time, int chan, QPoint pos, QVector<double> data);
    void addData(QString XP, int field, int stackZ, int time, int chan, QString pos, QVector<double> data);

    void resyncmodel();


    QVector<double> getData(QString XP, int field, int stackZ, int time, int chan, int x, int y);
    QVector<double> getData(QString XP, int field, int stackZ, int time, int chan, QPoint pos);
    QVector<double> getData(QString XP, int field, int stackZ, int time, int chan, QString pos);

    QStringList getExperiments() const;
    int getExperimentCount() const;
    int getDataSize() const;

    void setCommitName(QString str);
    QString getCommitName();

    QString &getAggregationMethod(QString XP);

    void setAggregationMethod(QString Xp, QString method);

    int commitToDatabase(QString hash, QString prefix);


    QVariant getData(int row, QString) const;
    QString getMeta(int row, int col) const;

    void timerEvent(QTimerEvent *event);

protected:
    ExperimentFileModel* _owner;
    QString _commitName;
    int nbX, nbY;

    QList<DataHolder> _dataset;
    QSet<QString> _datanames;
    QSet<int> fields, stacks, times, chans;

    QMap<QString, QString> _aggregation;
    static QMutex _lock;

    void bindValue(QSqlQuery& select, DataHolder& h, QString key = QString());

    bool modified;

    int saveTimer;

public:

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void clearAll();

    int exposeDataColumnCount(QStringList memlist, QStringList dblist);

    MatrixDataModel* exposeData(QStringList memlist, QStringList dblist);
    MatrixDataModel* exposeData(QStringList memlist, QStringList dblist, QVector<QList<int> >& subdata);

    MatrixDataModel* exposeMemoryData(QStringList xps, MatrixDataModel* datamodel);
    MatrixDataModel* exposeDatabaseData(QStringList xps,  MatrixDataModel* datamodel);
    MatrixDataModel* exposeMemoryData(QStringList xps, MatrixDataModel* datamodel, QVector<QList<int> > &subdata );
    MatrixDataModel* exposeDatabaseData(QStringList xps,  MatrixDataModel* datamodel, QVector<QList<int> >& subdata );

};

typedef ExperimentDataTableModel ExperimentDataModel;

#include "matrixdatamodel.h"


class DllCoreExport ExperimentFileModel: public DataProperty
{

public:
    enum WellState { HasMeasurement = 0, IsSelected = 1, IsCurrent = 2 };
public:
    ExperimentFileModel(Dictionnary dict = Dictionnary());
    virtual ~ExperimentFileModel();

    void setRowCount(unsigned r);
    unsigned getRowCount();

    void setColCount(unsigned r);
    unsigned getColCount();



    bool isSelected(QPoint pos);
    void select(QPoint pos, bool active = true);

    bool isCurrent(QPoint pos);
    void setCurrent(QPoint pos, bool active = true);

    bool hasMeasurements(QPoint pos);
    void setMeasurements(QPoint pos, bool active = true);

    void reloadDatabaseData(bool load=false, QString db=QString());
    void reloadDatabaseData(QString file, QString t, bool aggregat, bool load=false);
    void reloadDatabaseDataCSV(QString file, QString t, bool aggregat, bool load=false);
    void reloadDatabaseDataFeather(QString file, QString t, bool aggregat, bool load=false);

    QPair<QStringList, QStringList> databases();

    void setProperties(QString tag, QString value);
    virtual QString property(QString tag) const;
    virtual QString property(QRegExp tag) const;

    QSize getSize();

    void setFieldPosition();

    QMap<int, QMap<int, int> > getFieldPosition();
    QPair<QList<double>, QList<double> > getFieldSpatialPositions();

    SequenceFileModel& operator()(int row, int col);
    SequenceFileModel& operator()(QPoint Pos);

    QStringList getTags(QPoint pos);
    void setTag(QPoint pos, QString list);
    bool hasTag();

    QString getColor(QPoint pos);
    void setColor(QPoint pos, QString col);


    QList<SequenceFileModel*> getSelection();
    QList<SequenceFileModel*> getAllSequenceFiles();

    QList<SequenceFileModel*> getValidSequenceFiles();
    SequenceFileModel &getFirstValidSequenceFiles();

    void Q_DECL_DEPRECATED addToDatabase();

    QString name() const;
    void setName(const QString &name);

    QString groupName() const;
    void setGroupName(const QString& name);

    ExperimentFileModel* getOwner() const;
    void setOwner(ExperimentFileModel* own);


    QString fileName() const;
    void setFileName(const QString &fileName);

    QString getProjectName() const;

    void clearState(WellState state);

    QList<ExperimentFileModel *> getSiblings();
    void addSiblings(QString map, ExperimentFileModel* efm);

    ExperimentFileModel *getSibling(QString name);

    ExperimentDataModel *computedDataModel();
    bool hasComputedDataModel();

    ExperimentDataModel *databaseDataModel(QString name);
    bool hasdatabaseDataModel(QString name);

    QStringList getDatabaseNames();

    QList<QPoint> &measurementPositions();

    bool displayed();
    void setDisplayed(bool val=true);

    QString hash();

    void setChannelNames(QStringList names);
    QStringList getChannelNames();

    void setMetadataPath(QString m);
    QString getMetadataPath();


protected:

    void setState(QPoint pos, WellState state, bool active);
    bool getState(QPoint pos, WellState state);

protected:
    unsigned _cols, _rows;
    QString _fileName;
    QString _name, _groupName;
    QString _hash;
    bool _hasTag;

    bool _displayed;

    QString metadataPath;


    QMap<QString, ExperimentFileModel *> _siblings;
    ExperimentFileModel* _owner;

    ExperimentDataModel *_computedData;
    QMap<QString, ExperimentDataModel *> _databaseData;

    QMap<int, QMap<int, unsigned> > _state;
    QMap<int, QMap<int, SequenceFileModel> > _sequences;
    QList<QPoint> _positions;
    QMap<int, QMap<int, int> > toField;
    QPair<QList<double>, QList<double> > fields_pos;
    QStringList _channelNames;
};

typedef QList<ExperimentFileModel*> Screens;




class DllCoreExport ScreensHandler
{
public:
    static ScreensHandler& getHandler();

    Screens loadScreens(QStringList list, bool allow_loaded=false);
    Screens& getScreens();
    ExperimentFileModel* getScreenFromHash(QString hash);

    QString findPlate(QString plate, QStringList project, QString drives = QString());


    QList<SequenceFileModel *> addProcessResultImage(QJsonObject &data);
    SequenceFileModel *addProcessResultSingleImage(QJsonObject &data);
//    SequenceFileModel *addProcessResultSingleImage(QJsonObject &data, QString processHash, int procId, bool displayed);

    QList<QString> &getTemporaryFiles();

    QString& errorMessage();

    ExperimentFileModel *loadScreen(QString scr);
    void addScreen(ExperimentFileModel* xp);
    void removeScreen(ExperimentFileModel* xp);
    QList<SequenceFileModel *> addProcessResultImage(QCborValue &data);
    ExperimentFileModel* addDataToDb(QString hash, QString commit, QJsonObject &data, bool finished);
    void commitAll();
protected:
    Screens _screens;
    QMap<QString, ExperimentFileModel*> _mscreens;
    QList<QString> _result_images;
    QString _error;
    //  static QMutex _mutex;
};




// other helper func

QPair<QList<double>, QList<double> > getWellPos(SequenceFileModel* seq, unsigned fieldc,  int z, int t, int c);
QPointF getFieldPos(SequenceFileModel* seq, int field, int z, int t, int c);

#endif // WELLPLATEMODEL_H
