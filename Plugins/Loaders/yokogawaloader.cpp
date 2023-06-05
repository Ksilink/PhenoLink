#include "yokogawaloader.h"

#include <QMessageBox>

#include <QFile>
#include <QDir>

#include <QDomDocument>
#include <QDomNode>

#include <QCryptographicHash>

ExperimentFileModel *YokogawaLoader::getExperimentModel(QString _file)
{
    _error = QString();
    ExperimentFileModel* r = new ExperimentFileModel();

    QStringList sfile = _file.split("/"); if (sfile.size() == 1) sfile = _file.split("\\");  sfile.pop_back();
    r->setGroupName(sfile.at(std::max(0, sfile.count()-2)));
    r->setName(sfile.back());


    QFile mrf(_file);
    if (!mrf.open(QFile::ReadOnly))
    {
        qDebug() << "Error opening file" << _file;
        _error += QString("Error opening file %1").arg(_file);
        return 0;
    }

    QDomDocument doc("mrf");
    if (!doc.setContent(&mrf))
    {
        mrf.close();
        return 0;
    }

    QDomElement el = doc.firstChildElement("bts:MeasurementDetail");

    if (el.isNull()) return 0;

    for (int i = 0; i < el.attributes().count(); ++i)
    {
        QDomNode a = el.attributes().item(i);

        QString tag = a.nodeName().split(":").back();
        r->setProperties(tag, a.nodeValue());
    }


    for( int ir = 0; ir < el.childNodes().count(); ++ir)
    {
        QDomNode n = el.childNodes().at(ir);

        if (n.attributes().contains("bts:Ch"))
        {
            QString ch = QString("_ch%1").arg(n.attributes().namedItem("bts:Ch").nodeValue());

            for (int i = 0; i < n.attributes().count(); ++i)
            {
                QDomNode a = n.attributes().item(i);
                if (a.nodeName() == "bts:Ch") continue;
                QString tag = a.nodeName().split(":").back() + ch;
                r->setProperties(tag, a.nodeValue());
            }
        }
    }

    mrf.seek(0);
    QCryptographicHash hash(QCryptographicHash::Md5);

    hash.addData(&mrf);

    r->setProperties("hash", hash.result().toHex());
    r->setProperties("file", _file);

    mrf.close();

    QDir dir(_file); dir.cdUp();
    QString _mlf = dir.absolutePath() + "/MeasurementData.mlf";

    QFile mlf(_mlf);

    QDomDocument d("mlf");
    if (!d.setContent(&mlf))
    {
        qDebug() << "Not loading" << _mlf;
        _error += QString("Error opening file %1").arg(_mlf);
        mlf.close();
        return 0;
    }


    int error_count = 0;

    QDomElement m = d.firstChildElement("bts:MeasurementData").firstChildElement("bts:MeasurementRecord");
    do
    {
        int row = -1, col = -1, isImage = false;
        int timepoint = 0, fieldidx = 0, zindex = 0, channel = 0;
        for (int i = 0; i < m.attributes().count(); ++i)
        {
            QDomNode a = m.attributes().item(i);

            QString tag = a.nodeName().split(":").back();

            if (tag == "Row") row = a.nodeValue().toInt() - 1;      // The code is 0 indexed , the xml is 1 indexed
            if (tag == "Column") col = a.nodeValue().toInt() - 1;   // The code is 0 indexed, the xml is 1 indexed

            if (tag == "TimePoint") timepoint = a.nodeValue().toInt();
            if (tag == "FieldIndex") fieldidx = a.nodeValue().toInt();
            if (tag == "ZIndex") zindex = a.nodeValue().toInt();
            if (tag == "Ch") channel = a.nodeValue().toInt();

            if (tag == "Type" && a.nodeValue() == "IMG")   isImage = true;
            //            qDebug() << tag << a.nodeValue() << isImage;
            if (tag == "Type" && a.nodeValue() == "ERR")
            {
                //                QMessageBox::StandardButton reply;
                //				_error += tr("Trying to load data with error\nPlease abort the loading otherwise some inconsitencies in the display and processing will appear");

                _error += QString("Error on file descriptor %1: R%2 C%3 T%4 F%5 Z%6 C%7\r\n")
                        .arg(_file).arg(row).arg(col).arg(timepoint).arg(fieldidx).arg(zindex).arg(channel);
                error_count++;
                //	reply = QMessageBox::critical(0x0, tr("Erroneous Data"),
                //                                tr("Trying to load data with error\nPlease abort the loading otherwise some inconsitencies in the display and processing will appear"),
                //                              QMessageBox::Abort | QMessageBox::Ignore);
                //  if (reply == QMessageBox::Abort)
                //    return 0;
            }
        }

        if (!((row == -1 && col == -1) || !isImage) )
        {
            SequenceFileModel& seq = (*r)(row, col);
            //            qDebug() << row << col << seq.Pos();
            seq.setOwner(r);

            for (int i = 0; i < m.attributes().count(); ++i)
            {
                QDomNode a = m.attributes().item(i);
                QString tag =a.nodeName().split(":").back();
                if (tag == "Row" || tag == "Column"||tag == "TimePoint"|| tag == "FieldIndex"||tag == "ZIndex"|| tag == "Ch"|| (tag == "Type" && a.nodeValue() == "IMG"))
                    continue; // skip already stored data
                QString mtag = QString("f%1s%2t%3c%4%5").arg(fieldidx).arg(zindex).arg(timepoint).arg(channel).arg(tag);

                //                qDebug() << tag << a.nodeValue();
                seq.setProperties(mtag, a.nodeValue());
            }
            r->setMeasurements(QPoint(row, col), true);
            if (!m.childNodes().at(0).nodeValue().isEmpty())
            {
                //                qDebug() << timepoint << fieldidx << zindex << channel;
                QString file = QString("%1/%2").arg(dir.absolutePath()).arg(m.childNodes().at(0).nodeValue());
                if (QFileInfo::exists(file))
                    seq.addFile(timepoint, fieldidx, zindex, channel, file);
                else
                {
                    error_count ++;
                    _error += QString("Plugin %1, was not able to locate file: %2 at timepoint %3, field %4, zpos %5 for channel %6\r\n")
                            .arg(pluginName()).arg(file).arg(timepoint).arg(fieldidx).arg(zindex).arg(channel);
                    seq.setInvalid();
                }
            }

            seq.checkValidity();
        }
        m = m.nextSiblingElement();
    } while (!m.isNull());


    if (error_count > 0)
    {
        qDebug() << _error;
        qDebug() << "Errors (" << error_count << ") with loading screen: " << _file;
    }

    QString MesSettings = r->property("MeasurementSettingFileName");


    dir = QDir(_file); dir.cdUp();
    QString _mes = dir.absolutePath() + "/" +MesSettings;

    QFile mes(_mes);

    d = QDomDocument("mes");
    if (!d.setContent(&mes))
    {
        qDebug() << "Not loading" << _mes;
        _error += QString("Error oppenning file %1\r\n").arg(_mes);
        mes.close();
        return r;
    }


    // Search for "ChannelList" && for each Channel description: Color field

    m = d.firstChildElement("bts:MeasurementSetting").firstChildElement("bts:ChannelList").firstChildElement("bts:Channel");
    //    qDebug() << d.firstChildElement("bts:MeasurementSetting").tagName();
    do
    {
        QString ch = m.attribute("bts:Ch");
        QString color = m.attribute("bts:Color");
        //        qDebug() << "channel" << ch << "Color" << color;
        r->setProperties("ChannelsColor"+ch, color);
        m = m.nextSiblingElement();
    } while (!m.isNull());



    return r;
}

QString YokogawaLoader::errorString()
{
    return _error;
}


YokogawaLoader::YokogawaLoader(): CheckoutDataLoaderPluginInterface()
{

}

YokogawaLoader::~YokogawaLoader()
{

}

QString YokogawaLoader::pluginName()
{
    return "Yokogawa Loader v0.a";
}

QStringList YokogawaLoader::handledFiles()
{
    return QStringList() << "MeasurementDetail.mrf";
}

bool YokogawaLoader::isFileHandled(QString file)
{
    if (file.endsWith("MeasurementDetail.mrf"))
        return true;
    else
        return false;
}
