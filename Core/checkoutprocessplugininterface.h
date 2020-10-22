#ifndef CHECKOUTPROCESSPLUGININTERFACE_H
#define CHECKOUTPROCESSPLUGININTERFACE_H


#include <QMap>
#include <QtCore>
#include "Dll.h"

#include <opencv2/highgui/highgui.hpp>

class ChannelSelectionType;

#include "ImageContainers.h"

struct InputImageMetaData
{
    QString hash;
    QString file_path;
    QString pos;
    QString commitName;

    int zPos;
    int fieldId;
    int TimePos;
    int channel;

    QJsonObject toJSON()
    {
        QJsonObject o;

        o["DataHash"]=hash;
        o["Pos"]=pos;
        o["zPos"]=zPos;
        o["FieldId"] = fieldId;
        o["TimePos"] = TimePos;
        o["channel"] = channel;
        o["CommitName"] = commitName;


        return o;
    }

};


class DllPluginManagerExport ChannelSelectionType // A placeholder for Explicit channel selection for algorithms
{
public:
    ChannelSelectionType(int v = 0) : _value(v)
    {

    }

    int operator()() const
    {
        return _value;
    }

    ChannelSelectionType& setValue(int v)
    {
        _value = v;
        return *this;
    }

protected:
    int _value;
};

DllPluginManagerExport QDebug operator<<(QDebug debug, const ChannelSelectionType &c);

#include "RegistrableTypes.h"


class DllPluginManagerExport CheckoutProcessPluginInterface
{
public:

    enum State { NotStarted, Running, Finished };

    typedef QMap<int, QColor> Colormap;


    CheckoutProcessPluginInterface();

    // This function is to be called in plugins constructors to declare to the handler what data is required as input
    // value is the plugin registered parameter
    // tag is a unique name for display and user query (enhence user displayed)
    // Comment is a comment that helps the user choose proper values
    // See usage exemples ()
    template <class Type>
    Registrable<Type>& use(Type* value, QString tag, QString comment)
    {
        if (_parameters.contains(tag)) _parameters.remove(tag);
        Registrable<Type>* r = new Registrable<Type>();


        r->setValuePointer(value);
        r->setTag(tag);
        r->setComment(comment);

        _parameters[tag] = r;

        return *r;
    }


    // Same function as above but dedicated to enumerated types
    template <typename Type>
    RegistrableEnum<Type>& use(Type* value, QStringList values, QString tag, QString comment)
    {
        if (_parameters.contains(tag)) _parameters.remove(tag);
        RegistrableEnum<Type>* r = new RegistrableEnum<Type>();

        r->setValuePointer(value);
        r->setTag(tag);
        r->setComment(comment);
        r->setEnum(values);

        _parameters[tag] = r;

        return *r;
    }

    template <class Type>
    RegistrablePair<Type>& use(Type* v1, Type* v2,
                               QString tag, QString comment1,
                               QString comment2)
    {

        if (_parameters.contains(tag)) _parameters.remove(tag);
        RegistrablePair<Type>* r = new RegistrablePair<Type>();


        r->setValuePointer(v1, v2);
        r->setTag(tag);
        r->setComments(comment1, comment2);

        _parameters[tag] = r;

        return *r;

    }

    // This function is to be called in plugins constructor to declare to the handler the output data produced by the plugin
    template <class Type>
    Registrable<Type>& produces(Type* value, QString tag, QString comment)
    {
        if (_parameters.contains(tag)) _parameters.remove(tag);
        Registrable<Type>* r = new Registrable<Type>();

        r->setAsProduct();
        r->setValuePointer(value);
        r->setTag(tag);
        r->setComment(comment);

        _results[tag] = r;
        return *r;
    }

    // This function is to be called in plugins constructor to declare to the handler the name of the processing
    // This name also has a purpose of presenting the processing in a tree like approach (called path, i.e.: a tree like representation separated by '/')
    // The authors allows the user to list the authors of the algorithm
    // Comments is a comment presenting the behaviour of the algorithm
    void description(QString path, QStringList authors,  QString comment );

    template <class Type>
    Registrable<Type>& getMetaInformation(Type* value)
    {

        foreach (RegistrableParent* val, _parameters.values())
        {
            Registrable<Type>* r = dynamic_cast<Registrable<Type>* > (val);
            if (r && (&r->value()) == value)
            {
//                qDebug() << "Found param meta for" << r->tag();
                return *r;
            }
        }

        foreach (RegistrableParent* val, _results.values())
        {
            Registrable<Type>* r = dynamic_cast<Registrable<Type>* > (val);
            if (r && (&r->value()) == value)
            {
//                 qDebug() << "Found result meta for" << r->tag();
                return *r;
            }
        }

        static Registrable<Type> par;
        return par;
    }

    QString wellName()
    {
      return _callParams["Pos"].toString();
    }


    QString plateName()
    {
        //
        return _callParams["PlateName"].toString();
    }


    QStringList channelNames()
    { // FIXME: get channel Names from plugin
        return QStringList();
    }

    cv::Mat& getBiasField(int i)
    { // "datahash"
        static cv::Mat empty;

        if (_meta.isEmpty())
            return empty;

        QString hash =_meta.first().hash;

        if (i <= _hashtoBias[hash].size())
        {
            _hashtoBias[hash].resize(i+1);
        }
        if (_hashtoBias[hash][i] == 0)
        { // create image
            // auto file = QString("%2/DC_sCMOS #%1_CAM%1.tif").arg(i).arg(_meta.first().file_path);
            QString file;
            cv::Mat* mat = new cv::Mat;
            foreach (RegistrableParent* val, _parameters.values())
            {
                RegistrableImageParent* r = dynamic_cast<RegistrableImageParent* > (val);
                if (r)
                {
                    auto t = r->getBiasFiles();
                    if (i < t.size())
                    {
                        file = t[i];
                        break;
                    }
                }
            }


            if (QFileInfo::exists(file))
            {
                *mat = cv::imread(file.toStdString(), 2);
                cv::Mat m;
                mat->convertTo(m, CV_32F, 1. / 10000.);
                cv::swap(m, *mat);
                m.release();
            }
            else
            {
                qDebug() << file << "does not exists empty bias returned";
                *mat = cv::Mat(2160,2560, CV_32F, cv::Scalar(1));
                // FIXME: default value may not be 1, but could be more like max of U16 or .5 max of U16...
            }
            _hashtoBias[hash][i] = mat;

        }

        return *_hashtoBias[hash][i];
    }




    /// This function is to be called after executing the process,
    /// in order to fetch the processed data.
    QJsonObject gatherData(qint64 time);


    QString getPath() const
    {
        return path;
    }
    QStringList getAuthors() const
    {
        return authors;
    }

    QString getComments() const
    {
        return comments;
    }

    virtual void read(const QJsonObject &json);

    virtual void write(QJsonObject &json) const;



    virtual ~CheckoutProcessPluginInterface();
    virtual CheckoutProcessPluginInterface* clone() = 0;
    virtual void exec() = 0;

    // This function shall handle some data conversion / fetching, especially for the image(s) datatype (Wells, Screens, XP, Fields etc...)
    virtual void prepareData();

    // Use this to tell the plugin manager a status (Evolution of a process in ratio, or simple message).
    // This let the process manager to display information on the evolution of the current processing
    // Message is a user friendly message
    //  Tell an overall status & advance ratio (overall + message)
    //  Tell the current step advance: (step + stepMessage)

    virtual void statusMessage(float overall, QString message, float step, QString stepMessage = QString());
    virtual void printMessage(QString message);

    QString getPosition() const;
    void setPosition(const QString &value);

    QJsonObject createStatusMessage();
    void setStatusMessage(QJsonObject ob);

    void finished();
    void started(qint64 time);
    State processState();

    bool isFinished();

    virtual bool isPython()  { return false; }


    void setColormap(void* data, Colormap color);

protected:
    QString path;
    QStringList authors;
    QString comments;
    QJsonObject _callParams;

    QString position;

    QMap<QString, RegistrableParent*> _parameters;
    QMap<QString, RegistrableParent*> _results;


    QString _message, _stepMessage;
    float overallEvolution; // 0 to 1, tell the global process advances
    float stepEvolution;  // 0 to 1 tell a step advance (for multi step algorithm)
    QStringList _infos;
    State _state;
    bool _shallDisplay;
    int processStartId;
    QJsonObject _result;
    static QMutex mutex;

    // Keep track of loaded Bias images
    static QMap<QString, QVector<cv::Mat*> > _hashtoBias;
    // Keep track of number of usage of Bias field
    static QMap<QString, int> _hashtoBiasCount;

    QList<InputImageMetaData> _meta;

};

#define CheckoutProcessPluginInterface_iid "fr.wiest-daessle.Checkout.ProcessPluginInterface"

Q_DECLARE_INTERFACE(CheckoutProcessPluginInterface, CheckoutProcessPluginInterface_iid)

class SequenceFileModel;



#endif // CHECKOUTPROCESSPLUGININTERFACE_H
