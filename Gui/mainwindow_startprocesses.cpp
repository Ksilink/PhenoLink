#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "screensmodel.h"


#include <algorithm>

#include <QFileSystemModel>
#include <QDebug>

#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QSettings>

#include <QProgressBar>
#include <QScrollBar>

#include <ScreensDisplay/graphicsscreensitem.h>

#include <QGraphicsRectItem>

#include <QScrollArea>
#include <QSlider>

#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QClipboard>
#include <QtConcurrent/QtConcurrent>

#include <QFuture>
#include <QFutureWatcher>

#include <ctkWidgets/ctkDoubleRangeSlider.h>
#include <ctkWidgets/ctkCollapsibleGroupBox.h>
#include <ctkWidgets/ctkRangeSlider.h>
#include <ctkWidgets/ctkPopupWidget.h>
#include <ctkWidgets/ctkDoubleSlider.h>
#include <ctkWidgets/ctkCheckableHeaderView.h>
#include <ctkWidgets/ctkColorPickerButton.h>
#include <ctkWidgets/ctkPathLineEdit.h>

#include <QCheckBox>

#include <QTableView>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>

#include <QSortFilterProxyModel>

#include <QtConcurrent>

#include <Core/checkoutprocess.h>
#include <Core/imageinfos.h>

#include <ImageDisplay/imageform.h>

#include <ImageDisplay/scrollzone.h>

#include "ScreensDisplay/screensgraphicsview.h"
#include <QInputDialog>

#include <networkprocesshandler.h>



template <class Ob>
void setData(QJsonObject& obj, QString tag, bool list, Ob* s)
{
    if (s)
    {
        if (list)
        {
            QJsonArray ar = obj[tag].toArray();
            ar.push_back(s->value());
            obj[tag] = ar;
        }
        else
        {
            obj[tag] = s->value();
        }
    }
}

template <>
void setData(QJsonObject& obj, QString tag, bool list, QComboBox* s)
{
    if (s)
    {
        QStringList el;
        for (int i = 0; i < s->count(); ++i)
            el << s->itemText(i);
        obj["Enum"] = QJsonArray::fromStringList(el);
        if (list)
        {
            QJsonArray ar = obj[tag].toArray();
            ar.push_back(s->currentText());
            obj[tag] = ar;
        }
        else
        {
            if (s->objectName() == "Channels") // To return index of channel names
                obj[tag] = QString("%1").arg(s->currentIndex()+1);
            else
                obj[tag] = s->currentText();
        }
    }
}

template <>
void setData(QJsonObject& obj, QString tag, bool list, QLineEdit* s)
{
    if (s)
    {
        if (list)
        {
            QJsonArray ar = obj[tag].toArray();
            ar.push_back(s->text());
            obj[tag] = ar;
        }
        else
        {
            obj[tag] = s->text();
        }
    }
}

template <>
void setData(QJsonObject& obj, QString tag, bool list, ctkPathLineEdit* s)
{
    if (s)
    {
        if (list)
        {
            QJsonArray ar = obj[tag].toArray();
            ar.push_back(s->currentPath());
            obj[tag] = ar;
        }
        else
        {
            //  qDebug() << s->currentPath();
            obj[tag] = s->currentPath();
        }
    }
}

template <>
void setData(QJsonObject& obj, QString tag, bool list, QCheckBox* s)
{
    if (s)
    {
        if (list)
        {
            QJsonArray ar = obj[tag].toArray();
            ar.push_back(s->checkState()==Qt::Unchecked?0:1);
            obj[tag] = ar;
        }
        else
        {
            //  qDebug() << s->currentPath();
            obj[tag] = s->checkState()==Qt::Unchecked?0:1;
        }
    }
}



void MainWindow::getValue(QWidget* wid, QJsonObject& obj, QString tag, bool list)
{
    // to handle the case where a single input is dependent on the context :)
    // For instance unit of measurement, where you can use the same input with different meaning, or different default value
    // Say a surface can be expressed in px² or µm² but in the plugin code it is just a matter of adapting the value with respect to single parameter

    if (list && !obj.contains(tag))
        obj[tag] = QJsonArray();

    setData(obj, tag, list, dynamic_cast<QSlider*>(wid));
    setData(obj, tag, list, dynamic_cast<QSpinBox*>(wid));
    setData(obj, tag, list, dynamic_cast<ctkDoubleSlider*>(wid));
    setData(obj, tag, list, dynamic_cast<QDoubleSpinBox*>(wid));
    setData(obj, tag, list, dynamic_cast<QComboBox*>(wid));
    setData(obj, tag, list, dynamic_cast<QLineEdit*>(wid));
    setData(obj, tag, list, dynamic_cast<ctkPathLineEdit*>(wid));
    setData(obj, tag, list, dynamic_cast<QCheckBox*>(wid));

}


bool sortWidgets(QWidget* a, QWidget* b)
{
    int sa = a->parentWidget()->objectName().remove("Channel ").toInt();
    int sb = b->parentWidget()->objectName().remove("Channel ").toInt();
    //qDebug() << a->parentWidget()->objectName() << b->parentWidget()->objectName()<< sa << sb;
    return sa < sb;
}


QJsonArray MainWindow::startProcess(SequenceFileModel* sfm, QJsonObject obj,
                                    QList<bool> selectedChanns, QRegularExpression siteMatcher )
{

    static size_t crypto_offset = 0;

    if (!ui || !ui->processingArea)
    {
        //        QMessageBox::warning(this, tr("Starting Process"),
        qDebug() << tr("Unable to set data parameter");
        return QJsonArray();
    }
    bool started = false;
    if (!sfm)
    {
        //        QMessageBox::warning(this,
        //                             tr("Starting Process"),
        //                             tr("Currently only the image in the current display is to processed, please drag & drop a well in the workspace and retry to process"));

        qDebug() << "Currently only the image in the current display is to processed, please drag & drop a well in the workspace and retry to process";

        return QJsonArray();
    }


    QString imgType;
    QStringList metaData;

    bool asVectorImage = isVectorImageAndImageType(obj, imgType, metaData);

    QList<QJsonObject>  images =  sfm->toJSON(imgType, asVectorImage, selectedChanns, metaData, siteMatcher);
    SequenceInteractor* inter = _sinteractor.current();

    if (!images.size())
    {
        //        QMessageBox::warning(this,
        //                             tr("Starting Process"),
        //                             QString(tr("Channels filtering leads to an empty set %1")).arg(sfm->Pos()));
        return QJsonArray();
    }

    QJsonArray procArray;
    //        int i = 0;
    foreach (QJsonObject im, images)
    {
        //        QCoreApplication::processEvents();
        if (_typeOfprocessing->currentText() == "Current Image")
        {
            if (im["zPos"].toInt() !=  (int)inter->getZ() ||
                im["FieldId"].toInt() != (int)inter->getField() ||
                im["TimePos"].toInt() != (int)inter->getTimePoint())
                continue;
        }

        if (_typeOfprocessing->currentText() == "All Loaded Screens"
            ||
            _typeOfprocessing->currentText().startsWith("Selected Screens")
            )
            obj["shallDisplay"] = false;
        else
            obj["shallDisplay"] = true;

        obj["ProcessStartId"] = StartId;
        obj["Project"] = sfm->getOwner()->property("project");

        // Need to fill the parameters with appropriate values
        QJsonArray params = obj["Parameters"].toArray(), bias;


        obj["Parameters"] = adjustParameterFromWidget(sfm, im, params, bias);

        params = obj["ReturnData"].toArray();
        for (int i = 0; i < params.size(); ++i )
        {
            QJsonObject par = params[i].toObject();

            if (par.contains("isOptional"))
            {

                QString tag = par["Tag"].toString();
                QCheckBox* wid = ui->processingArea->findChild<QCheckBox*>(tag);

                if (!wid) continue;
                if (_typeOfprocessing->currentText().contains("Selected Screens"))
                {
                    // Do not reclaim images if we are running heavy duty informations!!!
                    par["optionalState"] = false;
                    obj["BatchRun"]=true;
                }
                else
                    par["optionalState"] = (wid->checkState() == Qt::Checked);
                //                qDebug() << "Setting optional state:" << wid->checkState();
                params.replace(i, par);
            }

        }

        obj["ReturnData"] = params;


        obj["Pos"] = sfm->Pos();

        //            qDebug() << "Image" << obj;
        obj["CommitName"] = _commitName->text();
        if (sfm->getOwner())
            obj["XP"] = sfm->getOwner()->groupName() +"/"+sfm->getOwner()->name();
        obj["WellTags"] = sfm->getTags().join(";");

        QByteArray arr;
        arr += (QString("%1").arg(crypto_offset)).toLatin1();
        arr += QCborValue::fromJsonValue(obj).toByteArray();//QJsonDocument(obj).toBinaryData();
        arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
        arr += (QString("%1").arg(crypto_offset)).toLatin1();

        QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

        obj["CoreProcess_hash"] = QString(hash.toHex());

        crypto_offset ++;

        procArray.append(obj);
    }
    // Display first image set
    //    if (procArray.size() > 0)
    //        qDebug() << procArray[0];

    //    handler.startProcess(_preparedProcess, procArray);
    if (procArray.size()) started = true;

    return procArray;
    //return started;
}

void MainWindow::startProcess()
{
    //   QtConcurrent::run(this, &MainWindow::startProcessRun);
    startProcessRun();
}


int recurseField(QJsonObject ob, QString key)
{
    int val = -1;
    if (ob.contains(key))
    {
        if (ob[key].isArray())
        {
            if (ob[key].toArray().size() != 1)
                val = -1;
            val = ob[key].toArray().first().toInt();
        }
        val = ob[key].toInt();
    }

    for (auto it = ob.begin(), e = ob.end(); it != e; ++it)
    {
        int tmp = -1;
        if (it.value().isObject())
        {
            tmp = recurseField(it.value().toObject(), key);
            if (tmp != -1 && val == -1) val = tmp;
            else
                if (tmp != -1 && val != tmp) return -1;

        }
        if (it.value().isArray())
            for (auto item : it.value().toArray())
            {
                tmp = recurseField(item.toObject(), key);
                if (tmp != -1 && val == -1) val = tmp;
                else
                    if (tmp != -1 && val != tmp) return -1;
            }
    }
    return val;
}


QJsonArray& MainWindow::adjustParameterFromWidget(SequenceFileModel* sfm, QJsonObject& im, QJsonArray& params, QJsonArray& bias)
{
    for (int i = 0; i < params.size(); ++i )
    { // First set the images up, so that it will match the input data
        QJsonObject par = params[i].toObject();

        if (par.contains("ImageType"))
        {
            foreach (QString j, im.keys())
                par.insert(j, im[j]);


            if ((par["Data"][0]["Channels"].isArray() || par["Channels"].isArray()) && bias.isEmpty())
            {
                QJsonArray chs;
                if (par["Data"][0]["Channels"].isArray())
                    chs = par["Data"][0]["Channels"].toArray();
				else
					chs = par["Channels"].toArray();
                for (int j = 0; j < chs.size(); ++j)
                {
                    int channel = chs[j].toInt();
                    QString bias_file = sfm->property(QString("ShadingCorrectionSource_ch%1").arg(channel));
                    bias.append(bias_file);
                }
            }


            par["bias"] = bias;
            params.replace(i, par);
        }

    }
    QJsonArray cchans;
    for (int i = 0; i < params.size(); ++i )
    {
        QJsonObject par = params[i].toObject();
        if (par["Channels"].isArray() && cchans.isEmpty())
        {
            cchans = par["Channels"].toArray();
        }
    }
    //         qDebug() << "Found channels:" << cchans;
    for (int i = 0; i < params.size(); ++i )
    {
        QJsonObject par = params[i].toObject();

        QString tag = par["Tag"].toString();
        //            qDebug() << par["Channels"].toArray().size() << par["PerChannelParameter"].toBool();
        QList<QWidget*> wids = ui->processingArea->findChildren<QWidget*>(tag);

        if (cchans.size() &&  par["PerChannelParameter"].toBool())
        {
            tag = QString("%1_%2").arg(par["Tag"].toString()).arg(cchans.first().toInt());
            //                qDebug() << "Searching Tag" << tag << cchans.size() << cchans;
            wids = ui->processingArea->findChildren<QWidget*>(tag);
        }
        if (wids.isEmpty() && par["PerChannelParameter"].toBool())
        {
            // qDebug() << cchans.size() << _channelsIds.values();
            for (int i = 0; i < _channelsIds.size(); ++i)
            {
                tag = QString("%1_%2").arg(par["Tag"].toString()).arg(i);
                auto l = ui->processingArea->findChildren<QWidget*>(tag);
                //                    qDebug() << "Searching Tag:" << tag << l.size();

                for (auto item: l)
                    wids.append(item);
            }
            //                qDebug() << wids;
        }

        if (wids.empty()) {
            //                qDebug() << "Searching " << tag << "Params not found";
            continue;
        }

        std::sort(wids.begin(), wids.end(), sortWidgets);


        foreach (QWidget* wid, wids)
        {
            //                qDebug() << par << wids;
            getValue(wid, par, "Value", wids.size() > 1);
        }
        params.replace(i, par);

        if (par.contains("Comment2"))
        {
            QList<QWidget*> wids = ui->processingArea->findChildren<QWidget*>(tag+"2");

            if (wids.empty()) {
                continue;
            }

            std::sort(wids.begin(), wids.end(), sortWidgets);


            foreach (QWidget* wid, wids)
                getValue(wid, par, "Value2", wids.size() > 1);
            params.replace(i, par);
        }

    }


    return params;
}


void MainWindow::startProcessOtherStates(QList<bool> selectedChanns, QList<SequenceFileModel*> lsfm,
                                         bool started, QRegularExpression siteMatcher, QString exports)//, QMap<QString, QSet<QString> > tags_map)
{
    static int WorkID = 1;




    Q_UNUSED(started);
    //    _startingProcesses = true;
    QJsonObject objR, stored;

    CheckoutProcess& handler = CheckoutProcess::handler();

    handler.getParameters(_preparedProcess, objR);


    objR["WorkID"]=QString("%1").arg(WorkID);

    WorkID++;

    handler.setProcessCounter(new int());
    QSettings set;
    int minProcs = std::numeric_limits<int>::max(); // set.value("MinProcs", 2000).toInt();


    QJsonArray procArray;
    int count = 0;
    QMap<QString, int > adapt;
    //    bool deb = set.value("UserMode/Debug", false).toBool();


    QSet<QString> xps;


    // Need to check if the Image Type is a "WellPlate" type then this won't work :'(
    QString imgType;
    QStringList metaData;

    bool asVectorImage = isVectorImageAndImageType(objR, imgType, metaData);

    if (imgType.startsWith("WellPlate") )
    { // adjust the procArray !

        // Group again the data from the plate
        QMap<QString, QList<SequenceFileModel*> > plates;
        QString project;
        foreach (SequenceFileModel* sfm, lsfm)
        {
            if (sfm->getOwner())
            {
                QString s = sfm->getOwner()->groupName() +"/"+sfm->getOwner()->name();
                if (project.isEmpty())
                    project = sfm->getOwner()->property("project");
                xps.insert(s);
            }
            plates[sfm->getOwner()->name()] << sfm;
        }


        for (auto & kv: plates)
        {

            QJsonArray ar, tp;
            QString pos("A01");
            for (auto& sfm: kv)
            {
                // Convert to json...
                QList<QJsonObject>  images = sfm->toJSON(QString("TimeStackedImage%1").arg(imgType.endsWith("XP") ? "XP" : ""),
                                                        asVectorImage, selectedChanns, metaData, siteMatcher);

                // if asVectorImage is true we just have one data in images.size()

                if (ar.isEmpty())
                    for (auto & a: images)
                    {
                        Q_UNUSED(a);
                        auto ob = objR;
                        ar.append(ob); // copy the reference object
                        tp.append(QJsonObject());
                    }

                for (int i = 0; i < images.size(); ++i)
                {
                    QJsonObject data = tp[i].toObject();
                    QJsonObject oo = ar[i].toObject();

                    QStringList drop = QStringList() << "BasePath" << "DataHash" << "PlateName";
                    for (auto &d : drop) { data[d] = images[i][d]; images[i].remove(d); }

                    if (pos.isEmpty())     pos = images[i]["Pos"].toString();

                    auto p = sfm->pos();
                    QString x = QString("%1").arg(p.x()),
                        y = QString("%1").arg(p.y());
                    if (data.contains(x))
                    {
                        QJsonObject t = data[x].toObject();
                        t[y] = images[i];
                        data[x] = t;
                    }
                    else
                    {
                        QJsonObject t; t[y] = images[i];
                        data[x] = t;
                    }




                    QJsonArray params = objR["Parameters"].toArray(),  bias;

                    adjustParameterFromWidget(sfm, oo, params, bias);

                    int channel = -1,
                        fieldId = recurseField(data, "FieldId");

                    if (channel == -1 && !asVectorImage)
                        channel = recurseField(data, "Channel");


                    oo["PlateName"] = sfm->getOwner()->name();
                    oo["DataHash"] =  sfm->getOwner()->hash();
                    oo["Pos"]=pos;
                    oo["FieldId"]= imgType.contains("XP") ? -1 : fieldId;
                    oo["TimePos"]=-1;
                    oo["zPos"]=-1;
                    oo["XP"] = sfm->getOwner()->groupName() +"/"+sfm->getOwner()->name();

                    if (!asVectorImage)
                        oo["Channel"]=channel;

                    for (int i = 0; i < params.size(); ++i)
                    {
                        auto oob = params[i].toObject();
                        oob["DataHash"] =  sfm->getOwner()->hash();
                        oob["Pos"]="A01";
                        oob["FieldId"]= imgType.contains("XP") ? -1 : fieldId;
                        oob["TimePos"]=-1;
                        oob["zPos"]=-1;
                        oob["BasePath"] = sfm->getBasePath();
                        oob["XP"] = sfm->getOwner()->groupName() +"/"+sfm->getOwner()->name();
                        if (!asVectorImage)
                            oob["Channel"]=channel;

                        if (oob.contains("ContentType") && oob["ContentType"].toString()=="Image")
                        {
                            oob["Data"]=data;
                            params.replace(i, oob);
                        }
                    }


                    oo["Parameters"]=params;
                    oo["DataHash"] = sfm->getOwner()->hash();

                    ar.replace(i, oo);
                    tp.replace(i, data);
                }
            }

            static int crypto_offset = 0;
            for (int i = 0; i < ar.size(); ++i)
            {
                auto obj = ar.at(i).toObject();

                QByteArray arr;
                arr += (QString("%1").arg(crypto_offset)).toLatin1();
                arr += QCborValue::fromJsonValue(obj).toByteArray();//QJsonDocument(obj).toBinaryData();
                arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
                arr += (QString("%1").arg(crypto_offset)).toLatin1();

                QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

                obj["CoreProcess_hash"] = QString(hash.toHex());

                obj["ProcessStartId"] = StartId;
                obj["Project"] = project;

                obj["Pos"]="A01"; // Force return pos to be A01

                obj["CommitName"] = _commitName->text();

                auto params = obj["Parameters"].toArray();
                for (int i = 0; i < params.size(); ++i)
                    if (params[i].toObject().contains("Parameters"))
                    {
                        auto oo = params[i].toObject();
                        oo.remove("Parameters");
                        params.replace(i, oo);
                    }
                obj["Parameters"] = params;


                crypto_offset ++;

                procArray << obj;
            }

        }

        _StatusProgress->setMinimum(0);
        _StatusProgress->setMaximum(procArray.size());
        _StatusProgress->setValue(0);

        stored=procArray.first().toObject();

    }
    else
        foreach (SequenceFileModel* sfm, lsfm)
        {
            if (!sfm) continue;
            if (sfm->getOwner())
            {
                QString s = sfm->getOwner()->groupName() +"/"+sfm->getOwner()->name();
                xps.insert(s);
            }

            // startProcess is more a prepare process list function now
            QJsonArray tmp  = startProcess(sfm, objR, selectedChanns, siteMatcher);
            if (count == 0  && tmp.size())
            {
                stored=tmp[0].toObject();
            }

            if (sfm && sfm->getOwner())
                adapt[sfm->getOwner()->name()] += tmp.size();

            count += tmp.size();
            _StatusProgress->setMinimum(0);
            _StatusProgress->setMaximum(count);
            _StatusProgress->setValue(0);

            for (int i = 0; i < tmp.size(); ++i)
                procArray.append(tmp[i]);
            if (exports.isEmpty() && this->networking && procArray.size() > minProcs)
            {
                handler.startProcess(_preparedProcess, procArray);
                procArray = QJsonArray();
            }
        }

    if (exports.isEmpty() && this->networking &&  procArray.size())
        handler.startProcess(_preparedProcess, procArray);


    if (!exports.isEmpty())
    {
        QJsonDocument doc(procArray);
        QFile saveFile(exports);

        if (!saveFile.open(QIODevice::WriteOnly)) {
            qWarning("Couldn't open save file.");
        }
        else
            saveFile.write(doc.toJson());
        saveFile.close();
        _StatusProgress->setValue(_StatusProgress->maximum());
        return;
    }

    stored["Experiments"] = QJsonArray::fromStringList(QStringList(xps.begin(), xps.end()));

    // If no commit store the start in params, otherwise with the commit name !!!




    QString st = (stored["CommitName"].toString().isEmpty()) ? "/params/":
                     "/"+stored["CommitName"].toString() +"/";

    QString proc = _preparedProcess;



    QDir dir(set.value("databaseDir").toString());

    QString writePath = QString("%1/PROJECTS/%2/Checkout_Results/").arg(dir.absolutePath(), lsfm[0]->getOwner()->property("project"))
        ;

    dir.mkpath(writePath + st);
    QString fn = writePath + st +
                 proc.replace("/", "_").replace(" ", "_") + "_" +
                 QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+".json";

    qDebug() << "Saving run params to:" << fn;

    QJsonDocument doc(stored);
    QFile saveFile(fn);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
    }
    else
        saveFile.write(doc.toJson());
    saveFile.close();


    for (auto kv = adapt.begin(), ke = adapt.end(); kv != ke; ++kv) {
        if (mapValues.contains(kv.key()))
            mapValues[kv.key()] += kv.value();
        else
            mapValues[kv.key()] = kv.value();
    }


    qDebug() <<"Processes: "<< adapt << count;
    // Nico@DESKTOP-KH3G5D0:Tools/Speed/SpeedTesting(5)

    QString username = set.value("UserName", "").toString(),
        hostname = QHostInfo::localHostName();


    // Set the cancel name of the object
    _cancelation->setObjectName(QString("%1@%2:%3(%4)").arg(username,hostname, proc.replace("/", "") ).arg(WorkID));

    if (this->networking && handler.errors() > 0)
    {
        // First chance to try to recover from errors !
        //        handler.restartProcessOnErrors();
    }


    if ( !this->networking )
    {
        procArray = QJsonArray();
    }

}

void MainWindow::startProcessRun(QString exp)
{
    StartId++;

    if (!_typeOfprocessing)
        return;
    SequenceViewContainer& hdl = SequenceViewContainer::getHandler();


    QList<bool> selectedChanns;

    foreach (QCheckBox* b, _ChannelPicker)
        selectedChanns << (b->checkState() == Qt::Checked);

    if (_ChannelPicker.size() != 0 && selectedChanns.count() == 0)
    {
        QMessageBox::warning(this,
                             tr("Starting Process"),
                             tr("No Channels Selected,\n"\
                                "     you need to check at least 1 channel to be processed"
                                ));
        return;
    }

    QList<SequenceFileModel*> lsfm ;

    if (_typeOfprocessing->currentText() == "Selected Screens"  || _typeOfprocessing->currentText() == "Selected Screens and Filter")
    {

        QStringList checked = mdl->getCheckedDirectories();
        ScreensHandler& handler = ScreensHandler::getHandler();

        Screens s = handler.loadScreens(checked, true);
        QString projName;
        for(Screens::iterator it = s.begin(), e = s.end(); it != e; ++it)
        {


            if ((*it)->computedDataModel()) (*it)->computedDataModel()->clearAll();


            if ((*it)->property("project").isEmpty())
            {
                if (projName.isEmpty())
                {
                    bool ok;
                    QStringList sug=(*it)->fileName().replace("\\", "/").split("/");
                    if (sug.size() > 4)
                        sug[0] = sug[4];
                    else
                        sug[0] = sug[sug.size()-2];

                    QString text = QInputDialog::getText(this,
                                                         QString("Please specify project Name for: %1 (%2)").arg((*it)->name()).arg((*it)->groupName()),
                                                         "Project Name", QLineEdit::Normal,
                                                         sug.at(0), &ok);
                    if (ok && !text.isEmpty())
                        projName = text;
                }
                else (*it)->setProperties("project", projName);

            }
            else
                projName = (*it)->property("project");



            if (*it)
            {

                if (!(*it)->displayed())
                {
                    createWellPlateViews(*it);
                    (*it)->setDisplayed(true);
                }
                (*it)->setGroupName(this->mdl->getGroup((*it)->fileName()));

                QList<SequenceFileModel*> seqs = (*it)->getValidSequenceFiles();

                for (QList<SequenceFileModel*> ::Iterator si = seqs.begin(), se = seqs.end(); si != se; ++si)
                {
                    if ((*si)->getOwner()
                        && (*si)->getOwner()->hasMeasurements((*si)->pos())
                        && (*si)->isValid())
                    {
                        lsfm << *si;
                    }
                }
            }
        }
    }

    if (_typeOfprocessing->currentText() == "All loaded Sequences")
        for (auto& seq: hdl.getSequences())
            if (seq->isValid() && seq->getOwner()->hasMeasurements(seq->pos()))
                lsfm << seq;

    if (_typeOfprocessing->currentText() == "Current Well")
        lsfm.push_back(hdl.current());

    if (_typeOfprocessing->currentText() == "Selected Wells")
    {
        Screens s = ScreensHandler::getHandler().getScreens();
        for(Screens::iterator it = s.begin(), e = s.end(); it != e; ++it)
        {
            if ((*it)->computedDataModel()) (*it)->computedDataModel()->clearAll();

            QList<SequenceFileModel*> seqs = (*it)->getSelection();

            for (QList<SequenceFileModel*> ::Iterator si = seqs.begin(), se = seqs.end(); si != se; ++si)
            {
                if ((*si)->isValid() && (*si)->getOwner()->hasMeasurements((*si)->pos()))
                {
                    // si->Pos();
                    lsfm << *si;
                }
            }
        }
    }
    if (_typeOfprocessing->currentText() == "Current Image" &&
        _sinteractor.current())
        lsfm.push_back(_sinteractor.current()->getSequenceFileModel());


    if (_typeOfprocessing->currentText() == "All Loaded Screens")
    {
        ScreensHandler& handler = ScreensHandler::getHandler();
        Screens& data = handler.getScreens();
        foreach (ExperimentFileModel* efm, data)
        {
            if (efm->computedDataModel()) efm->computedDataModel()->clearAll();


            QList<SequenceFileModel*> t = efm->getAllSequenceFiles();
            foreach (SequenceFileModel* l, t)
                lsfm.push_back(l);
        }

    }

    QRegularExpression siteMatcher;
    if ( _typeOfprocessing->currentText() == "Selected Screens and Filter")
    {
        // Add the filtering part !!!

        bool ok;

        QString filtertags = QInputDialog::getText(this,
                                                   "Set Filter condition (separated by ';' )",
                                                   "Part of tag or Well Regexp 'W:' followed by regexp:", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        QStringList tag_filter = filtertags.isEmpty() ? QStringList() :
                                     filtertags.split(';');
        QStringList remTags;
        QRegularExpression wellMatcher;
        QList<QRegularExpression> tagRegexps;

        for (auto f : tag_filter)
        {
            if (f.startsWith("W:"))
            {
                remTags <<  f;
                wellMatcher.setPattern(f.replace("W:", ""));
            }
            if (f.startsWith("S:")) // Fix the site filter by passing to the startProcessOtherStates function
            {
                remTags << f;
                siteMatcher.setPattern(f.replace("S:", ""));
            }
            if (f.startsWith("T:"))
            {
                remTags << f;
                QRegularExpression r;
                r.setPattern(f.replace("T:", ""));
                tagRegexps << r;
            }
        }
        for (auto s: remTags) tag_filter.removeAll(s);


        QList<SequenceFileModel*> lsfm2 ;


        for (auto sfm: lsfm)
        {
            QStringList tgs = sfm->getTags();
            int matches = 0;
            for (auto& tf : tag_filter)
                for (auto& t : tgs)
                    matches += t.contains(tf);

            auto wPos = sfm->Pos();
            if (!wellMatcher.pattern().isEmpty() && wellMatcher.match(wPos).hasMatch())
                matches ++;

            for (auto r: tagRegexps) for (auto t: tgs) if (r.match(t).hasMatch())  matches++;

            if (matches > 0)
                lsfm2 << sfm;
            //if (tagRegexps.isEmpty() && wellMatcher.isEmpty())
            //    lsfm2 << sfm;
        }



        lsfm = lsfm2;
    }

    if (lsfm.isEmpty())
    {

        QMessageBox::warning(this,
                             tr("Starting Process"),
                             tr("No Image Selected,\n"\
                                "     you need to drag & drop at least one image (if 'Current Image' or 'Image In View' processing)\n"\
                                "Or\n"\
                                "     you need to load a WellPlate for 'Current Screen' processing"));
        return;
    }

    //SequenceInteractor* inter = _sinteractor.current();
    bool started = false;
    //    _startingProcesses = true;

    QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
    connect(watcher, SIGNAL(finished()), this, SLOT(setProgressBar()));

    lsfm = QSet<SequenceFileModel*>(lsfm.begin(), lsfm.end()).values(); // To remove duplicates

    // Let's check if we are in some data reconstruction mode, i.e. we want to complete some of the processings we already performed
    // if the commit name exists & plate name .fth we compute the missing processings & ask the user if he/she wants to perform the full process
    // or just continue with the missings data

    if (!_commitName->text().isEmpty())
    {

        QSet<QString> pls;
        for (auto& v: lsfm) pls.insert( v->getOwner()->name() );

        QSettings set;
        QDir dir(set.value("databaseDir").toString());

        for (auto& pl: pls)
        {
            QString writePath = QString("%1/PROJECTS/%2/Checkout_Results/%3/%4.fth")
                                    .arg(dir.absolutePath(),
                                         lsfm[0]->getOwner()->property("project"),
                                         _commitName->text(),
                                         pl);
            qDebug() << writePath;
        }
    }



    process_starttime = QDateTime::currentDateTime();

    qDebug() << "Wells to process" << lsfm.size() << " - "<< process_starttime.toString("yyyyMMdd hh:mm:ss.zzz");

    this->statusBar()->showMessage(QString("Wells to process %1 - %2").arg(lsfm.size()).arg(process_starttime.toString("yyyyMMdd hh:mm:ss.z")));

    // Start the computation.
    if (!_StatusProgress)
    {
        auto w = new QWidget();
        w->setLayout(new QHBoxLayout());
        w->layout()->setContentsMargins(0,0,0,0);
        w->layout()->setSpacing(1);

        _StatusProgress = new QProgressBar(this);
        _StatusProgress->setFormat("%v / %m");
        _cancelation = new QPushButton(QIcon(":/cancel.png"), "", this);

        _cancelation->connect(_cancelation, &QPushButton::clicked,
                              [this](bool){ auto cancel = this->_cancelation->objectName();
                                  NetworkProcessHandler::handler().sendCommand("mmi.cancel");
                                  _StatusProgress->setValue(_StatusProgress->maximum());

                              });


        w->layout()->addWidget(_StatusProgress);
        w->layout()->addWidget(_cancelation);

        this->statusBar()->addPermanentWidget(w);

    }

    _StatusProgress->setRange(0,0);

    run_time.start();
    startProcessOtherStates(selectedChanns, lsfm, started, siteMatcher, exp);


    QPushButton* s = ui->processingArea->findChild<QPushButton*>("ProcessStartButton");
    if (s && !exp.isEmpty()) s->setDisabled(true);

}


void MainWindow::on_pluginhistory(QString )
{
    auto cb = qobject_cast<QComboBox*>(sender());

    pluginHistory(cb);
}

bool compare_js(QJsonValue v1, QJsonValue v2)
{
    if (v1.type() != v2.type())
        return false;

    if (v1.isString())
        return v1.toString()==v2.toString();
    if (v1.isDouble())
        return v1.toDouble()==v2.toDouble();

    return true;
}

bool compare_par(QJsonObject ob1, QJsonObject ob2)
{
    bool comp = true;
    QStringList vs = QStringList() << "Value" << "Value2";

    if (!ob1.contains("Value"))
        return false;

    for (auto n: vs)
    {

        if (ob1[n].isString() || ob1[n].isDouble())
            comp &= compare_js(ob1[n],ob2[n]);

        if (ob1[n].isArray())
        {
            auto a1 = ob1[n].toArray(), a2 = ob2[n].toArray();

            for (int i = 0; i < std::min(a1.size(), a2.size()); ++i)
                comp &= compare_js(a1[i], a2[i]);

        }
    }
    return comp;
}

void MainWindow::pluginHistory(QComboBox * cb)
{

    QString path = cb->currentData().toString();
    // Reload the json !
    if (path.isEmpty())
    {
        //        QJsonObject params;
        CheckoutProcess::handler().getParameters(_preparedProcess, _processParams);
        int idx = cb->currentIndex();
        if (idx >= 0)
            setupProcessCall(_processParams, idx);
        return;
    }

    QJsonObject reloaded;
    QFile jfile(path);

    if (!jfile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }

    reloaded = QJsonDocument::fromJson(jfile.readAll()).object();
    // Check file contains Path & that this path is our process :p
    if (!reloaded.contains("Path") || reloaded["Path"] != this->_preparedProcess )
    {
        CheckoutProcess::handler().getParameters(_preparedProcess, _processParams);
        int idx = cb->currentIndex();
        if (idx >= 0)
            setupProcessCall(_processParams, idx);
    }

    // Quick & dirty check for file content if missing skip history reload
    for (auto str: { "Parameters" , "PluginVersion", "Project", "Experiments" })
        if (!reloaded.contains(QString(str)))
            return;

    QJsonArray params = _processParams["Parameters"].toArray(),
        rel_par = reloaded["Parameters"].toArray();
    // now we iterate through this params, search in reloaded a diffe
    for (int i = 0; i < params.size(); ++i)
    {
        auto ref_tag = params[i].toObject()["Tag"].toString();
        for (int j = 0; j < rel_par.size(); ++j)
        {
            if (ref_tag == rel_par[j].toObject()["Tag"].toString() &&
                !compare_par(params[i].toObject(), rel_par[j].toObject()))
            {
                auto ob=params[i].toObject();
                ob["Value"] = rel_par[j].toObject()["Value"];
                if (rel_par[j].toObject().contains("Value2"))
                    ob["Value2"] = rel_par[j].toObject()["Value2"];
                ob["NonDefault"]=true;
                params.replace(i, ob);
                break;
            }
        }
    }

    _processParams["Parameters"] = params;


    // We need to adjust the _processParams wrt to reloaded entry, to avoid the value changes effect
    setupProcessCall(_processParams, cb->currentIndex());
}
