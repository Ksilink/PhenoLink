#include "incellloader.h"

#include <QMessageBox>

#include <QFile>
#include <QDir>

#include <QDomDocument>
#include <QDomNode>

#include <QCryptographicHash>



QRgb WavelengthToRGB(float w)
{
    float R,G,B;

    if (w >= 380 && w < 440)
    {
        R = -(w - 440.) / (440. - 350.);
        G = 0.0;
        B = 1.0;
    }
    else if (w >= 440 && w < 490)
    {
        R = 0.0;
        G = (w - 440.) / (490. - 440.);
        B = 1.0;
    }
    else if (w >= 490 && w < 510)
    {
        R = 0.0;
        G = 1.0;
        B = -(w - 510.) / (510. - 490.);
    }
    else if ( w >= 510 && w < 580)
    {
        R = (w - 510.) / (580. - 510.);
        G = 1.0;
        B = 0.0;
    }
    else if ( w >= 580 && w < 645)
    {
        R = 1.0;
        G = -(w - 645.) / (645. - 580.);
        B = 0.0;
    }
    else if ( w >= 645 && w <= 780)
    {
        R = 1.0;
        G = 0.0;
        B = 0.0;
    }
    else
    {
        R = 0.0;
        G = 0.0;
        B = 0.0;
    }
    float SSS = 0;
    if (w >= 380 && w < 420)
        SSS = 0.3 + 0.7*(w - 350) / (420 - 350);
    else if ( w >= 420 && w <= 700)
        SSS = 1.0;
    else if ( w > 700 && w <= 780)
        SSS = 0.3 + 0.7*(780 - w) / (780 - 700);
    else
        SSS = 0.0;
    SSS *= 255;

    return qRgb(SSS*R, SSS*G, SSS*B);

}


ExperimentFileModel *InCellLoader::getExperimentModel(QString _file)
{
    _error = QString();
    ExperimentFileModel* r = new ExperimentFileModel();

    QStringList sfile = _file.split("/"); if (sfile.size() == 1) sfile = _file.split("\\");  sfile.pop_back();
    r->setGroupName(sfile.at(std::max(0, sfile.count()-2)));
    r->setName(sfile.back());


    QFile mlf(_file);
    if (!mlf.open(QFile::ReadOnly))
    {
        qDebug() << "Error opening file" << _file;
        _error += QString("Error opening file %1").arg(_file);
        return 0;
    }

    mlf.seek(0);
    QCryptographicHash hash(QCryptographicHash::Md5);

    hash.addData(&mlf);

    // NOTE: Is the hash computed on the file or the filename, or part of the filename / filepath
    r->setProperties("hash", hash.result().toHex());
    r->setProperties("file", _file);

    QDir dir(_file); dir.cdUp();

    mlf.seek(0);
    QDomDocument d("xdce");
    if (!d.setContent(&mlf))
    {
        qDebug() << "Not loading" << _file;
        _error += QString("Error opening file %1").arg(_file);
        mlf.close();
        return 0;
    }

    int rCount = d.firstChildElement("ImageStack").firstChildElement("AutoLeadAcquisitionProtocol")
            .firstChildElement("PlateMap").firstChildElement("NamingConvention").attribute("rows").toInt();
    int cCount = d.firstChildElement("ImageStack").firstChildElement("AutoLeadAcquisitionProtocol")
            .firstChildElement("PlateMap").firstChildElement("NamingConvention").attribute("columns").toInt();


    QDomElement n = d.firstChildElement("ImageStack").firstChildElement("AutoLeadAcquisitionProtocol").firstChildElement("Wavelengths").firstChildElement("Wavelength");
    int ch = 0;
    do {
        QRgb color = WavelengthToRGB(n.firstChildElement("ExcitationFilter").attribute("name").replace("_",".").toDouble());
        r->setProperties(QString("ChannelsColor%1").arg(ch), QColor(color).name(QColor::HexArgb));
        //qDebug()<< ch <<  QColor(color).name()<<n.firstChildElement("ExcitationFilter").attribute("name");
        ch++;
        n = n.nextSiblingElement();
    } while (!n.isNull());


   // qDebug() << r->properties();

    QDomElement m = d.firstChildElement("ImageStack").firstChildElement("Images").firstChildElement("Image");
    do
    {
        int row = -1, col = -1, isImage = true;
        int timepoint = 0, fieldidx = 0, zindex = 1, channel = 0;

        row = m.firstChildElement("Well").firstChildElement("Row").attribute("number").toInt()-1;
        col = m.firstChildElement("Well").firstChildElement("Column").attribute("number").toInt()-1;

        if (rCount < row) rCount = row;
        if (cCount < col) cCount = col;

        fieldidx = m.firstChildElement("Identifier").attribute("field_index").toInt() + 1;
        timepoint = m.firstChildElement("Identifier").attribute("time_index").toInt() + 1;
        channel =  m.firstChildElement("Identifier").attribute("wave_index").toInt() + 1;

        QString fname = m.attribute("filename");

        SequenceFileModel& seq = (*r)(row, col);
    //    qDebug() << row << col << seq.Pos() << fieldidx << timepoint << channel;
        seq.setOwner(r);

        r->setMeasurements(QPoint(row, col), true);
       {
//           qDebug() << timepoint << fieldidx << zindex << channel << fname;
            QString file = QString("%1/%2").arg(dir.absolutePath()).arg(fname);
            if (QFileInfo::exists(file))
                seq.addFile(timepoint, fieldidx, zindex, channel, file);
            else
            {
                _error += QString("Plugin %1, was not able to locate file: %2 at timepoint %3, field %4, zpos %5 for channel %6\n")
                        .arg(pluginName()).arg(file).arg(timepoint).arg(fieldidx).arg(zindex).arg(channel);
                seq.setInvalid();
            }
        }

        seq.checkValidity();
        //}
        m = m.nextSiblingElement();
    } while (!m.isNull());


    r->setProperties("RowCount", QString("%1").arg(rCount));
    r->setProperties("ColumnCount", QString("%1").arg(cCount));

    return r;
}

QString InCellLoader::errorString()
{
    return _error;
}


InCellLoader::InCellLoader(): CheckoutDataLoaderPluginInterface()
{

}

InCellLoader::~InCellLoader()
{

}

QString InCellLoader::pluginName()
{
    return "InCell Loader v0.a";
}

QStringList InCellLoader::handledFiles()
{
    return QStringList() << "*.xdce";
}

bool InCellLoader::isFileHandled(QString file)
{
    if (file.endsWith(".xdce"))
        return true;
    else
        return false;
}

