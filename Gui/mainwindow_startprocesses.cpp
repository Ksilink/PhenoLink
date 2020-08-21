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
#include <QtWinExtras/QWinTaskbarProgress>

#include <QScrollBar>

#include <ScreensDisplay/graphicsscreensitem.h>

#include <QGraphicsRectItem>
//#include <QPushButton>

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
#include <QtSql>
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




bool isVectorImageAndImageType(QJsonObject obj,  QString& imgType, QStringList& metaData)
{
    bool asVectorImage = false;
    {
        QJsonArray params = obj["Parameters"].toArray();
        for (int i = 0; i < params.size(); ++i )
        {
            QJsonObject par = params[i].toObject();
            if (par.contains("ImageType"))
            {
                imgType = par["ImageType"].toString();
                asVectorImage = par["asVectorImage"].toBool();
            }
            if (par.contains("Properties"))
            {
                QJsonArray ar = par["Properties"].toArray();
                for (int i =0; i < ar.size(); i++)
                    metaData << ar.at(i).toString();
            }
        }
    }
    return asVectorImage;
}

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

void MainWindow::getValue(QWidget* wid, QJsonObject& obj, QString tag, bool list)
{

    if (list && !obj.contains(tag))
        obj[tag] = QJsonArray();

    setData(obj, tag, list, dynamic_cast<QSlider*>(wid));
    setData(obj, tag, list, dynamic_cast<QSpinBox*>(wid));
    setData(obj, tag, list, dynamic_cast<ctkDoubleSlider*>(wid));
    setData(obj, tag, list, dynamic_cast<QDoubleSpinBox*>(wid));
    setData(obj, tag, list, dynamic_cast<QComboBox*>(wid));
    setData(obj, tag, list, dynamic_cast<QLineEdit*>(wid));
    setData(obj, tag, list, dynamic_cast<ctkPathLineEdit*>(wid));
}


bool sortWidgets(QWidget* a, QWidget* b)
{
    int sa = a->parentWidget()->objectName().remove("Channel ").toInt();
    int sb = b->parentWidget()->objectName().remove("Channel ").toInt();
    //qDebug() << a->parentWidget()->objectName() << b->parentWidget()->objectName()<< sa << sb;
    return sa < sb;
}


QJsonArray MainWindow::startProcess(SequenceFileModel* sfm, QJsonObject obj,
                                    QList<bool> selectedChanns )
{
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

    QList<QJsonObject>  images =  sfm->toJSON(imgType, asVectorImage, selectedChanns, metaData);
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
                _typeOfprocessing->currentText() == "Selected Screens"   )
            obj["shallDisplay"] = false;
        else
            obj["shallDisplay"] = true;

        obj["ProcessStartId"] = StartId;

        // Need to fill the parameters with appropriate values
        QJsonArray params = obj["Parameters"].toArray();
        for (int i = 0; i < params.size(); ++i )
        { // First set the images up, so that it will match the input data
            QJsonObject par = params[i].toObject();

            if (par.contains("ImageType"))
            {
                foreach (QString j, im.keys())
                    par.insert(j, im[j]);
                params.replace(i, par);
            }


        }

        QJsonArray cchans;
        for (int i = 0; i < params.size(); ++i )
        {
            QJsonObject par = params[i].toObject();
            if (par["Channels"].isArray() && cchans.isEmpty())
                cchans = par["Channels"].toArray();
        }
        //        qDebug() << "Found channels:" << cchans;
        for (int i = 0; i < params.size(); ++i )
        {
            QJsonObject par = params[i].toObject();

            QString tag = par["Tag"].toString();
            //            qDebug() << par["Channels"].toArray().size() << par["PerChannelParameter"].toBool();
            if (cchans.size() &&  par["PerChannelParameter"].toBool())
            {
                tag = QString("%1_%2").arg(par["Tag"].toString()).arg(cchans.first().toInt());
            }
            //            qDebug() << "Searching widget" << tag;
            QList<QWidget*> wids = ui->processingArea->findChildren<QWidget*>(tag);

            if (wids.empty()) {
                continue;
            }

            std::sort(wids.begin(), wids.end(), sortWidgets);


            foreach (QWidget* wid, wids)
                getValue(wid, par, "Value", wids.size() > 1);
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

        obj["Parameters"] = params;

        params = obj["ReturnData"].toArray();
        for (int i = 0; i < params.size(); ++i )
        {
            QJsonObject par = params[i].toObject();

            if (par.contains("isOptional"))
            {

                QString tag = par["Tag"].toString();
                QCheckBox* wid = ui->processingArea->findChild<QCheckBox*>(tag);

                if (!wid) continue;
                if (_typeOfprocessing->currentText() == "Selected Screens")
                    // Do not reclaim images if we are running heavy duty informations!!!
                    par["optionalState"] = false;
                else
                    par["optionalState"] = (wid->checkState() == Qt::Checked);
                //                qDebug() << "Setting optional state:" << wid->checkState();
                params.replace(i, par);
            }

        }

        obj["ReturnData"] = params;


        obj["Pos"] = sfm->Pos();

        //            qDebug() << "Image" << obj;

        QByteArray arr;    arr += QJsonDocument(obj).toBinaryData();
        arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
        QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

        obj["CoreProcess_hash"] = QString(hash.toHex());
        obj["CommitName"] = _commitName->text();
        obj["WellTags"] = sfm->getTags().join(";");

        procArray.append(obj);

        //            qDebug()  << im["zPos"]<< im["FieldId"]<< im["TimePos"]<< im["Channel"]<< im["Channels"];
    }


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


void MainWindow::startProcessOtherStates(QList<bool> selectedChanns, QList<SequenceFileModel*> lsfm,
                                         bool started, QMap<QString, QSet<QString> > tags_map)
{
    Q_UNUSED(started);
    //    _startingProcesses = true;
    QJsonObject objR;

    CheckoutProcess& handler = CheckoutProcess::handler();

    handler.getParameters(_preparedProcess, objR);
    handler.setProcessCounter(new int());
    QSettings set;
    int minProcs = set.value("MinProcs", 20).toInt();


    QJsonArray procArray;
    int count = 0;
    QMap<QString, int > adapt;



    foreach (SequenceFileModel* sfm, lsfm)
    {
        if (_shareTags->isChecked())
        {
            sfm->clearTags();
            foreach (QString l, tags_map[sfm->Pos()]) sfm->setTag(l);
        }
        // startProcess is more a prepare process list function now
        QJsonArray tmp  = startProcess(sfm, objR, selectedChanns);

        adapt[sfm->getOwner()->name()] += tmp.size();
        count += tmp.size();
        _StatusProgress->setMaximum(count);

        for (int i = 0; i < tmp.size(); ++i)
            procArray.append(tmp[i]);
        if (this->networking && procArray.size() > minProcs)
        {
            handler.startProcess(_preparedProcess, procArray);
            procArray = QJsonArray();
        }
    }

    if (this->networking &&  procArray.size())
        handler.startProcess(_preparedProcess, procArray);


    for (auto kv = adapt.begin(), ke = adapt.end(); kv != ke; ++kv) {
        if (mapValues.contains(kv.key()))
            mapValues[kv.key()] += kv.value();
        else
            mapValues[kv.key()] = kv.value();
    }


    qDebug() <<"Processes: "<< adapt << count;



    if (this->networking && handler.errors() > 0)
    {
        // First chance to try to recover from errors !
        handler.restartProcessOnErrors();
    }


    if ( !this->networking )
    {
        procArray = QJsonArray();
    }

    //    qDebug() << "Sending " << count << " processes to servers";
    // handler.startProcess(_preparedProcess, procArray);
    //    _startingProcesses = false;
}

void MainWindow::startProcessRun()
{
    StartId++;


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


    QMap<QString, QSet<QString> > tags_map;

    if (_shareTags->isChecked())
    {
        QList<SequenceFileModel*> lsfm = hdl.getSequences();
        foreach (SequenceFileModel* sfm, lsfm)
        {
            auto tags = sfm->getTags();
            tags_map[sfm->Pos()].unite(QSet<QString>(tags.begin(), tags.end()));// ::fromList(tags));
        }
        lsfm.clear();
    }

    QList<SequenceFileModel*> lsfm ;

    if (_typeOfprocessing->currentText() == "Selected Screens")
    {

        QStringList checked = mdl->getCheckedDirectories();
        ScreensHandler& handler = ScreensHandler::getHandler();

        Screens s = handler.loadScreens(checked, true);
        for(Screens::iterator it = s.begin(), e = s.end(); it != e; ++it)
        {

            if (*it)
            {
                //(*it)->addToDatabase();
                /*   if (!QSqlDatabase::contains((*it)->hash()))
                {
                    QSettings set;
                    QDir dir(set.value("databaseDir").toString());
                    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", (*it)->hash());
                    db.setDatabaseName(dir.absolutePath() + "/" + (*it)->hash() + ".db");
                    db.open();
                }*/

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
                        // si->Pos();
                        lsfm << *si;
                    }
                }
            }
        }
    }

    if (_typeOfprocessing->currentText() == "All loaded Sequences")
        lsfm = hdl.getSequences();
    if (_typeOfprocessing->currentText() == "Current Well")
        lsfm.push_back(hdl.current());
    if (_typeOfprocessing->currentText() == "Selected Wells")
    {
        Screens s = ScreensHandler::getHandler().getScreens();
        for(Screens::iterator it = s.begin(), e = s.end(); it != e; ++it)
        {
            QList<SequenceFileModel*> seqs = (*it)->getSelection();

            for (QList<SequenceFileModel*> ::Iterator si = seqs.begin(), se = seqs.end(); si != se; ++si)
            {
                if ((*si)->isValid())
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
            QList<SequenceFileModel*> t = efm->getAllSequenceFiles();
            foreach (SequenceFileModel* l, t)
                lsfm.push_back(l);
        }

    }
    //}

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
    qDebug() << "Starting" << lsfm.size() << "# of processes";
    this->statusBar()->showMessage(QString("Starting %1 # of processes").arg(lsfm.size()));
    // Start the computation.
    if (!_StatusProgress)
    {
        _StatusProgress = new QProgressBar(this);
        this->statusBar()->addPermanentWidget(_StatusProgress);

        _StatusProgress->setFormat("%v/%m");
    }
    _StatusProgress->setRange(0,0);

    QFuture<void> future = QtConcurrent::run(this, &MainWindow::startProcessOtherStates,
                                             selectedChanns, lsfm, started, tags_map);
    watcher->setFuture(future);
    _watchers.insert(watcher);


    // if (started)
    //    {
    QPushButton* s = qobject_cast<QPushButton*>(sender());
    if (s) s->setDisabled(true);
    //    }

}
