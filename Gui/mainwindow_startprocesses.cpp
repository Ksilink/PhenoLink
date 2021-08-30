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
#ifdef WIN32
#include <QtWinExtras/QWinTaskbarProgress>
#endif
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
#include <QInputDialog>




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

void MainWindow::getValue(QWidget* wid, QJsonObject& obj, QString tag, bool list)
{

    if (wid->isHidden()) return; // Skip hidden widget,
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
                _typeOfprocessing->currentText() == "Selected Screens"
                ||
                _typeOfprocessing->currentText() == "Selected Screens and Filter"
                )
            obj["shallDisplay"] = false;
        else
            obj["shallDisplay"] = true;

        obj["ProcessStartId"] = StartId;

        // Need to fill the parameters with appropriate values
        QJsonArray params = obj["Parameters"].toArray(), bias;

        for (int i = 0; i < params.size(); ++i )
        { // First set the images up, so that it will match the input data
            QJsonObject par = params[i].toObject();

            if (par.contains("ImageType"))
            {
                foreach (QString j, im.keys())
                    par.insert(j, im[j]);

                if (par["Channels"].isArray() && bias.isEmpty())
                {
                    QJsonArray chs = par["Channels"].toArray();
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
                    tag = QString("%1_%2").arg(par["Tag"].toString()).arg(i+1);
                    auto l = ui->processingArea->findChildren<QWidget*>(tag);
                    //                    qDebug() << "Searching Tag:" << tag << l.size();

                    for (auto item: l)
                        wids.append(item);
                }
                qDebug() << wids;
            }

            if (wids.empty()) {
                qDebug() << "Searching " << tag << "Params not found";
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
                if (_typeOfprocessing->currentText() == "Selected Screens" || _typeOfprocessing->currentText() == "Selected Screens and Filter")
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

        QByteArray arr;    arr += QCborValue::fromJsonValue(obj).toByteArray();//QJsonDocument(obj).toBinaryData();
        arr += QDateTime::currentDateTime().toMSecsSinceEpoch();
        QByteArray hash = QCryptographicHash::hash(arr, QCryptographicHash::Md5);

        obj["CoreProcess_hash"] = QString(hash.toHex());
        obj["CommitName"] = _commitName->text();
        if (sfm->getOwner())
            obj["XP"] = sfm->getOwner()->groupName() +"/"+sfm->getOwner()->name();
        obj["WellTags"] = sfm->getTags().join(";");


        procArray.append(obj);
    }
    // Display first image set
    if (procArray.size() > 0)
        qDebug() << procArray[0];

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
    QJsonObject objR, stored;

    CheckoutProcess& handler = CheckoutProcess::handler();

    handler.getParameters(_preparedProcess, objR);

    handler.setProcessCounter(new int());
    QSettings set;
    int minProcs = set.value("MinProcs", 20).toInt();


    QJsonArray procArray;
    int count = 0;
    QMap<QString, int > adapt;
    bool deb = set.value("UserMode/Debug", false).toBool();
    if (deb)
        qDebug() << "Process to prepare: " << objR;

    QSet<QString> xps;

    foreach (SequenceFileModel* sfm, lsfm)
    {
        if (!sfm) continue;
        if (sfm->getOwner())
        {
            QString s = sfm->getOwner()->groupName() +"/"+sfm->getOwner()->name();
            xps.insert(s);
        }
        if (_shareTags && _shareTags->isChecked())
        {
            //            sfm->clearTags();
            foreach (QString l, tags_map[sfm->Pos()]) sfm->setTag(l);
        }
        // startProcess is more a prepare process list function now
        QJsonArray tmp  = startProcess(sfm, objR, selectedChanns);
        if (count == 0  && tmp.size())
        {
            QJsonDocument d(tmp[0].toObject());
            if (deb)
                qDebug() << d;
            stored=tmp[0].toObject();

            deb = false;
        }
        if (sfm && sfm->getOwner())
            adapt[sfm->getOwner()->name()] += tmp.size();
        count += tmp.size();
        _StatusProgress->setMinimum(0);
        _StatusProgress->setMaximum(count);
        _StatusProgress->setValue(0);

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

    stored["Experiments"] = QJsonArray::fromStringList(QStringList(xps.begin(), xps.end()));

    // If no commit store the start in params, otherwise with the commit name !!!




    QString st = (stored["CommitName"].toString().isEmpty()) ? "/params/":
                                                               "/"+stored["CommitName"].toString() +"/";

    QString proc = _preparedProcess;



    QDir dir(set.value("databaseDir").toString());
    //  dir.mkpath(set.value("databaseDir").toString() +st);

    // 20201210: Large behaviour change
    // Now: Assuming the following reordering:
    // dir + {tag.project} + Checkout_Results/ + prefix + / PlateName + .csv
    // If file exists move previous file with a post_fix info
    QString writePath = QString("%1/%2/Checkout_Results/").arg(dir.absolutePath())
            .arg(lsfm[0]->getOwner()->property("project"))
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

    if (_typeOfprocessing->currentText() == "Selected Screens"  || _typeOfprocessing->currentText() == "Selected Screens and Filter")
    {

        QStringList checked = mdl->getCheckedDirectories();
        ScreensHandler& handler = ScreensHandler::getHandler();

        Screens s = handler.loadScreens(checked, true);
        QString projName;
        for(Screens::iterator it = s.begin(), e = s.end(); it != e; ++it)
        {
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
            QList<SequenceFileModel*> t = efm->getAllSequenceFiles();
            foreach (SequenceFileModel* l, t)
                lsfm.push_back(l);
        }

    }
    //}

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
        QRegExp wellMatcher, siteMatcher;
        QList<QRegExp> tagRegexps;

        for (auto f : tag_filter)
        {
            if (f.startsWith("W:"))
            {
                remTags <<  f;
                wellMatcher.setPattern(f.replace("W:", ""));
            }
            if (f.startsWith("S:"))
            {
                remTags << f;
                siteMatcher.setPattern(f.replace("S:", ""));
            }
            if (f.startsWith("T:"))
            {
                remTags << f;
                QRegExp r;
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
            for (auto t : tag_filter)
                matches += tgs.contains(t);

            auto wPos = sfm->Pos();
            if (!wellMatcher.isEmpty() && !wellMatcher.exactMatch(wPos))
                matches ++;

            for (auto r: tagRegexps) for (auto t: tgs) if (r.exactMatch(t))  matches++;

            if (matches > 0)
                lsfm2 << sfm;
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

    run_time.start();
    startProcessOtherStates(selectedChanns, lsfm, started, tags_map);


    QPushButton* s = qobject_cast<QPushButton*>(sender());
    if (s) s->setDisabled(true);

}


void MainWindow::on_pluginhistory(QString )
{
    auto cb = qobject_cast<QComboBox*>(sender());

    QString path = cb->currentData().toString();
    // Reload the json !
    if (path.isEmpty())
    {
        QJsonObject params;
        CheckoutProcess::handler().getParameters(_preparedProcess, params);
        int idx = cb->currentIndex();
        if (idx >= 0)
            setupProcessCall(params, idx);
        return;
    }

    QJsonObject reloaded;
    QFile jfile(path);

    if (!jfile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }

    reloaded = QJsonDocument::fromJson(jfile.readAll()).object();
    qDebug() << reloaded;

    setupProcessCall(reloaded, cb->currentIndex());
}
