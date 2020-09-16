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
#include <QtWinExtras/QWinTaskbarProgress>

#include <ctkWidgets/ctkDoubleRangeSlider.h>
#include <ctkWidgets/ctkCollapsibleGroupBox.h>
#include <ctkWidgets/ctkRangeSlider.h>
#include <ctkWidgets/ctkPopupWidget.h>
#include <ctkWidgets/ctkDoubleSlider.h>
#include <ctkWidgets/ctkCheckableHeaderView.h>
#include <ctkWidgets/ctkColorPickerButton.h>
#include <ctkWidgets/ctkPathLineEdit.h>

#include <QCheckBox>

#include <QtSql>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>

#include <QtTest/QSignalSpy>

#include <Core/checkoutprocess.h>
#include <Core/imageinfos.h>

#include <ImageDisplay/imageform.h>

#include <ImageDisplay/scrollzone.h>
#include <QHBoxLayout>

#include "ScreensDisplay/screensgraphicsview.h"
#include <QLineEdit>
#include <QTextBrowser>

#ifdef CheckoutCoreWithPython
#include "Python/checkoutcorepythoninterface.h"
#endif

#include <optioneditor.h>

#include "Core/pluginmanager.h"
#include "Core/networkprocesshandler.h"


DllGuiExport QFile _logFile;

MainWindow::MainWindow(QProcess *serverProc, QWidget *parent) :
    QMainWindow(parent),
    networking(true),
    ui(new Ui::MainWindow),
    server(serverProc),
    _typeOfprocessing(0),
    _shareTags(0),
    //    _numberOfChannels(0),
    //	_startingProcesses(false),
    _python_interface(0),
    _commitName(0),
    //    _progress(0),
    _StatusProgress(0),
    StartId(0)
{
    ui->setupUi(this);

    ui->logWindow->hide(); // Hide the log window, since the content display is hidden now


#ifndef CheckoutCoreWithPython
    ui->menuScripts->setVisible(false);
    ui->menuScripts->hide();
#endif

    ui->dockImageControl->hide();
    ui->dockExperimentControl->hide();


    QSettings q;

    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(1, 10);
    slider->setValue(q.value("Image Flow/Count", 3).toInt());
    slider->setTickInterval(1);
    slider->setTickPosition(QSlider::TicksAbove);

    ui->tabWidget->setTabsClosable(true);
    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(removeWorkbench(int)));

    ui->tabWidget->setCornerWidget(slider);
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(changeLayout(int)));

    ui->tabWidget->clear();
    addImageWorkbench();

    // FIXME : Release the handle (v1)
    //  slider->hide();

    // FIXME: WellPlate Thumbnail
    // FIXME: Remove hide from thumbnail (v1)

    ui->thumbnailDock->hide();
    ui->thumbnail->verticalScrollBar()->setEnabled(false);
    ui->thumbnail->horizontalScrollBar()->setEnabled(false);

    tabifyDockWidget(ui->logWindow, ui->computed_features);

    mdl = new ScreensModel();


    QSettings set;
    QStringList l = set.value("ScreensDirectory", QVariant(QStringList())).toStringList();

    ui->actionAdvanced_User->setChecked(set.value("UserMode/Advanced", false).toBool());
    ui->actionVery_Advanced->setChecked(set.value("UserMode/VeryAdvanced", false).toBool());
    ui->actionDebug->setChecked(set.value("UserMode/Debug", false).toBool());



    if (!l.isEmpty())
    {
        for (QStringList::iterator it = l.begin(), e = l.end(); it != e; ++it)
        {

            if (!QFileInfo::exists(*it))
            {
                continue;
            }
            QStandardItem* itm = mdl->addDirectory(*it);
            if (!itm) continue;

            QString key = QString("Icons/%1").arg(itm->data(Qt::DisplayRole).toString());

            if (itm && set.contains(key))
            {
                int v = set.value(key, -1).toInt();
                QIcon ico(QString(":/MicC%1.png").arg(v));

                if (!ico.isNull())
                    itm->setData(ico, Qt::DecorationRole);
            }
        }
    }

    ui->treeView->setModel(mdl);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);


    connect(server, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(server_processError(QProcess::ProcessError)));

    connect(server, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(server_processFinished(int, QProcess::ExitStatus)));



    connect(&CheckoutProcess::handler(), SIGNAL(newPaths()),
            this, SLOT(refreshProcessMenu()));

    connect(&CheckoutProcess::handler(), SIGNAL(parametersReady(QJsonObject)),
            this, SLOT(setupProcessCall(QJsonObject)));

    connect(&CheckoutProcess::handler(), SIGNAL(processStarted(QString)),
            this, SLOT(processStarted(QString)));

    connect(&CheckoutProcess::handler(), SIGNAL(updateProcessStatus(QJsonArray)),
            this, SLOT(updateProcessStatusMessage(QJsonArray)));


    connect(&CheckoutProcess::handler(), SIGNAL(processFinished(QJsonObject)),
            this, SLOT(networkProcessFinished(QJsonObject)));

    connect(&CheckoutProcess::handler(), SIGNAL(emptyProcessList()),
            this, SLOT(processFinished()));


    connect(&CheckoutProcess::handler(), SIGNAL(payloadAvailable(QString)),
            this, SLOT(retrievedPayload(QString)));


    connect(ui->wellPlateViewTab->tabBar(), SIGNAL(tabCloseRequested(int)),
            this, SLOT(wellplateClose(int)));

    refreshProcessMenu();

    startTimer(set.value("RefreshRate", 300).toInt());

#ifdef CheckoutCoreWithPython
    setupPython();

    QFile f(set.value("Python/InitScript",
                      QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/checkout_init.py").toString()
            );
    if (f.exists())
    {
        _python_interface->setSelectedWellPlates(mdl->getCheckedDirectories(false));
        _python_interface->runFile(f.fileName());
        qDebug() << f.fileName();
    }
#endif

    //QProgressBar
    //    _progress = new QWinTaskbarProgress(this);
    //    _progress->setVisible(true);


}


MainWindow::~MainWindow()
{
    // Find everything that needs to be stopped :)

    delete ui;
}




void MainWindow::on_tabWidget_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    /// FIXME: Need to construct some plugin based solution to add query data

    QAction* addWb = menu.addAction(tr("Add an &Image Workbench tab"), this, SLOT(addImageWorkbench()));

    QAction* addWflw = menu.addAction(tr("Add a &Workflow tab"));
    QAction* addExpWb = menu.addAction(tr("Add an &Experiment Workbench tab"), this, SLOT(addExperimentWorkbench()));

    //    addWb->setDisabled(true);
    addWflw->setDisabled(true);

    menu.exec(ui->tabWidget->mapToGlobal(pos));
}



void MainWindow::databaseModified()
{

    /// FIXME !!!!
    //    _mdl->select();
}

QCheckBox* MainWindow::setupActiveBox(QCheckBox* box, ImageInfos* fo, int channel, bool reconnect)
{
    if (!reconnect)
    {
        box->setObjectName(QString("box%1").arg(channel));
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->setChecked(fo->active());
    }
    //  box->setToolTip(QString("box%1").arg(channel));

    connect(box, SIGNAL(toggled(bool)), this, SLOT(active_Channel(bool)), Qt::UniqueConnection);

    return box;
}

QDoubleSpinBox *MainWindow::setupVideoFrameRate(QDoubleSpinBox *extr, QString text)
{
   // qDebug() << _sinteractor.current()->getFps();

    extr->setObjectName(text);
    extr->setMinimum(0);
    extr->setMaximum(500);
    extr->setDecimals(1);
    extr->setValue(_sinteractor.current()->getFps());

    extr->setToolTip(text);

    extr->disconnect();

        //      connect(extr, SIGNAL(valueChanged(double)), fo, SLOT(forceMaxValue(double)),  Qt::UniqueConnection);
    connect(extr, SIGNAL(valueChanged(double)), this, SLOT(changeFpsValue(double)), Qt::UniqueConnection);

    return extr;
}


void MainWindow::active_Channel(bool c)
{

    SequenceInteractor* inter = _sinteractor.current();

    QString name = sender()->objectName().replace("box", "");
    ImageInfos* fo = inter->getChannelImageInfos(name.toInt() + 1);
    fo->setActive(c);


    qDebug() << "Interactor: change active channel " << sender()->objectName()  << fo;

}


ctkDoubleRangeSlider* MainWindow::RangeWidgetSetup(ctkDoubleRangeSlider* w, ImageInfos* fo, int channel, bool reconnect)
{
    if (!reconnect)
    {
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setObjectName(QString("Channel%1").arg(channel));


        w->setRange(fo->getMin(), fo->getMax());
        w->setPositions(fo->getDispMin(), fo->getDispMax());
    }

    //  w->disconnect();
    //  w->setToolTip(QString("Channel%4 %1: %2 %3").arg((uint)w,8,16).arg(w->minimum()).arg(w->maximum()).arg(channel));
    //  qDebug() << "Connect: " << w  << "to" << fo << this;
    //  connect(w, SIGNAL(valuesChanged(double,double)), fo, SLOT(rangeChanged(double,double)),Qt::UniqueConnection);
    connect(w, SIGNAL(valuesChanged(double, double)), this, SLOT(rangeChange(double, double)), Qt::UniqueConnection);
    return w;
}


ctkColorPickerButton *MainWindow::colorWidgetSetup(ctkColorPickerButton* bu, ImageInfos* fo, int channel, bool reconnect)
{
    if (!reconnect)
    {
        bu->setAttribute(Qt::WA_DeleteOnClose);
        bu->setObjectName(QString("ColorChannel%1").arg(channel));


        bu->setColor(fo->getColor());
        bu->setDisplayColorName(false);
        bu->setAttribute(Qt::WA_DeleteOnClose);
    }

    // bu->setToolTip(QString("ColorChannel%1").arg(channel));

    //  bu->disconnect();
    connect(bu, SIGNAL(colorChanged(QColor)), this, SLOT(setColor(QColor)), Qt::UniqueConnection);
    return bu;
}



void MainWindow::setColor(QColor c)
{
    SequenceInteractor* inter = _sinteractor.current();

    QString name = sender()->objectName().replace("ColorChannel", "");
    ImageInfos* fo = inter->getChannelImageInfos(name.toInt() + 1);
    fo->setColor(c);
}

QDoubleSpinBox* MainWindow::setupMinMaxRanges(QDoubleSpinBox* extr, ImageInfos* fo, QString text, bool isMin, bool reconnect)
{
    if (!reconnect)
    {
        extr->setObjectName(text);
        extr->setMinimum(-10 * fabs(fo->getMin()));
        extr->setMaximum(10 * fabs(fo->getMax()));
        extr->setDecimals(0);
        extr->setValue(isMin ? fo->getMin() : fo->getMax());
    }

    extr->setToolTip(text);

    extr->disconnect();
    if (isMin)
    {
        //      connect(extr, SIGNAL(valueChanged(double)), fo, SLOT(forceMinValue(double)),  Qt::UniqueConnection);
        connect(extr, SIGNAL(valueChanged(double)), this, SLOT(changeRangeValueMin(double)), Qt::UniqueConnection);
    }
    else
    {
        //      connect(extr, SIGNAL(valueChanged(double)), fo, SLOT(forceMaxValue(double)),  Qt::UniqueConnection);
        connect(extr, SIGNAL(valueChanged(double)), this, SLOT(changeRangeValueMax(double)), Qt::UniqueConnection);
    }
    return extr;
}



void MainWindow::updateCurrentSelection()
{
    SequenceInteractor* inter = _sinteractor.current();

    SequenceViewContainer & container = SequenceViewContainer::getHandler();
    container.setCurrent(inter->getSequenceFileModel());

    /// Change the plate tab widget to focus on the proper plate view

    QList<QWidget*> toDel;

    foreach(QWidget* wid, _imageControls.values())
    {
        if (wid) wid->hide(); // hide everything
        toDel << wid;
    }

    unsigned channels = inter->getChannels();
    // qDebug() << "#of Channels" << channels;

    QWidget* wwid = 0;

    if (_imageControls.contains(inter->getExperimentName()))
        toDel << _imageControls[inter->getExperimentName()];

    wwid = new QWidget;
    QVBoxLayout* bvl = new QVBoxLayout(wwid);
    bvl->setSpacing(1);

    //  qDebug() << "Creating Controls" << channels;
    for (unsigned i = 0; i < channels; ++i)
    {

        ImageInfos* fo = inter->getChannelImageInfos(i + 1);
        //          qDebug() << "Adding " << i << QString("Channel%1").arg(i);

        if (Q_UNLIKELY(!fo))        return;

        QWidget* wid = new QWidget;
        //            wid->setAttribute();
        QHBoxLayout* lay = new QHBoxLayout(wid);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(2);

        int ncolors = fo->getMax() - fo->getMin();
        int lastPal = 0;

        if (fo->getMin() >= 0 && fo->getMax() - fo->getMin() < 16)
        {
            lay->addWidget(setupActiveBox(new QCheckBox(wid), fo, i));

            int count = fo->getMax() - fo->getMin() + 1;
            QVector<QColor> pal = fo->getPalette();
            QVector<int> act = fo->getState();

            for (int l = 0; l < count; ++l)
            {
                //                QCheckBox* b = new QCheckBox(QString("%1").arg(l),wid);
                QLabel* lbl = new QLabel(QString("<html><head/><body><a href=\"%1,%2\" style='text-decoration:none; color: rgb(%5,%4,%3);'>%2</a></body></html>")
                                         .arg(i).arg(l)
                                         .arg(act[l] ? pal[l == 0 ? 0 : ((l + lastPal) % 16)].red() : 128)
                                         .arg(act[l] ? pal[l == 0 ? 0 : ((l + lastPal) % 16)].green() : 128)
                                         .arg(act[l] ? pal[l == 0 ? 0 : ((l + lastPal) % 16)].blue() : 128)
                                         , wid);
                lbl->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

                connect(lbl, SIGNAL(linkActivated(QString)), this, SLOT(changeColorState(QString)), Qt::UniqueConnection);
                lay->addWidget(lbl);
            }
            lay->addStretch();
            lastPal += ncolors;
        }
        else
        {
            lay->addWidget(setupActiveBox(new QCheckBox(wid), fo, i));
            lay->addWidget(setupMinMaxRanges(new QDoubleSpinBox(wid), fo, QString("vMin%1").arg(i), true));
            lay->addWidget(RangeWidgetSetup(new ctkDoubleRangeSlider(Qt::Horizontal, wid), fo, i));
            lay->addWidget(setupMinMaxRanges(new QDoubleSpinBox(wid), fo, QString("vMax%1").arg(i), false));
            lay->addWidget(colorWidgetSetup(new ctkColorPickerButton(wid), fo, i));
        }



        bvl->addWidget(wid);
        _imageControls[inter->getExperimentName()] = wwid;
    }

    // Add FrameRate control if it makes sense
    if (inter->getTimePointCount() > 1) {
      bvl->addWidget(setupVideoFrameRate(new QDoubleSpinBox(wwid), QString("Video Frame Rate")));
    }

    ui->imageControl->layout()->addWidget(wwid);

    // Cosmetics... Set the width of spinbox to a fixed largest value, to fit the screen evenly for all channels
    // Find largest vMin & vMax...
    int width_vmin = 0, width_vmax = 0;
    for (unsigned i = 0; i < channels; ++i)
    {
        width_vmin = std::max(width_vmin,
                              getWidgetWidth<QDoubleSpinBox*>(QString("vMin%1").arg(i)));
        width_vmax = std::max(width_vmax,
                              getWidgetWidth<QDoubleSpinBox*>(QString("vMax%1").arg(i)));
    }

    for (unsigned i = 0; i < channels; ++i)
    {
        setWidgetWidth<QDoubleSpinBox*>(QString("vMin%1").arg(i), width_vmin);
        setWidgetWidth<QDoubleSpinBox*>(QString("vMax%1").arg(i), width_vmax);
    }

    wwid->show();

    ui->treeWidget->clear();
    QList<QTreeWidgetItem*> items;

    QStringList file = inter->getFileName().split("/", Qt::SkipEmptyParts);

    file.pop_back();
    QString exp = file.last(); file.pop_back();
    QString own = file.last();


    items.append(new QTreeWidgetItem(QStringList() << "File " << QString("%2 %1").arg(exp).arg(own)));
    items.append(new QTreeWidgetItem(QStringList() << "Well " << QString("%1").arg(inter->getSequenceFileModel()->Pos())));
    items.append(new QTreeWidgetItem(QStringList() << "Z : " << QString("%2/%1").arg(inter->getZCount()).arg(inter->getZ())));
    items.append(new QTreeWidgetItem(QStringList() << "timePoints" << QString("%2/%1").arg(inter->getTimePointCount()).arg(inter->getTimePoint())));
    items.append(new QTreeWidgetItem(QStringList() << "Fields" << QString("%2/%1").arg(inter->getFieldCount()).arg(inter->getField())));
    items.append(new QTreeWidgetItem(QStringList() << "Channels" << QString("%1").arg(channels)));
    items.append(new QTreeWidgetItem(QStringList() << "Files" << QString("%1").arg(inter->getFile())));


    ui->treeWidget->insertTopLevelItems(0, items);
    ui->groupBox_2->setCollapsed(true);

    // Set the object in key as the currently displayed value un the wellplateviewtab
    for (QMap<QWidget*, ExperimentFileModel* >::iterator it = assoc_WellPlateWidget.begin(), e = assoc_WellPlateWidget.end(); it != e; ++it)
        if (it.value() == inter->getSequenceFileModel()->getOwner())
        {
            ui->wellPlateViewTab->setCurrentWidget(it.key());
            ScreensGraphicsView* view = dynamic_cast<ScreensGraphicsView*>(it.key());
            if (view)
                view->viewport()->repaint();
        }
    // Update the current plateview with the correct selection (clear the selection)

    // Force the database/table selection
    selectDataInTables();

    foreach(QWidget* w, toDel)
    {
        w->close();
    }

}

void MainWindow::changeLayout(int value)
{
    QWidget* wid = (ui->tabWidget->currentWidget());

    QSettings q;
    q.setValue("Image Flow/Count", value);

    wid->resize(wid->size() + QSize(0, 1));
}

void MainWindow::refreshProcessMenu()
{
    QMap<QString, QMenu*> acts;
    CheckoutProcess& procs = CheckoutProcess::handler();

    ui->menuProcess->clear();

    //  for (QStringList::iterator it = procs.paths().begin(), e = procs.paths().end(); it != e; ++it)
    foreach(QString path, procs.paths())
    {
        QStringList lst = path.split("/", Qt::SkipEmptyParts);
        QMenu* curMenu = ui->menuProcess;
        QString pos;
        for (int i = 0; i < lst.size(); ++i)
        {
            pos += lst.at(i);

            if (i == lst.size() - 1)
            {
                QAction * act = curMenu->addAction(lst.at(i));
                act->setData(path);
                connect(act, SIGNAL(triggered()), this, SLOT(prepareProcessCall()));
                // Connect action to effect
            }
            else
            {
                if (acts.contains(pos))
                    curMenu = acts[pos];
                else
                {
                    curMenu = curMenu->addMenu(lst.at(i));
                    acts[pos] = curMenu;
                }
            }
        }
    }
}

void MainWindow::conditionChanged(QWidget* sen, int val)
{
    //  qDebug() << "Changed !!!" << sen;
    if (_enableIf.contains(sen))
    {
        foreach(QWidget* w, _disable[sen])
        {
            //QWidget* w = _disable[sen];//->hide();
            w->hide();
            QFormLayout* lay = dynamic_cast<QFormLayout*>(w->parentWidget()->layout());

            if (lay && lay->labelForField(w))
                lay->labelForField(w)->hide();

            if (_enableIf[sen].contains(val))
            {

                foreach(QWidget* w, _enableIf[sen][val])
                {
                    // QWidget* w = _enableIf[sen][val];
                    w->show();
                    if (w->parentWidget())
                    {
                        QFormLayout* lay = dynamic_cast<QFormLayout*>(w->parentWidget()->layout());
                        if (lay)
                            lay->labelForField(w)->show();
                    }
                }
            }

        }
    }

}

void MainWindow::conditionChanged(int val)
{
    QWidget* sen = dynamic_cast<QWidget*>(sender());
    conditionChanged(sen, val);


}

void MainWindow::processStarted(QString hash)
{
    //    QList<QWidget*> list = ui->processingArea->findChildren<QWidget*>();
    //    foreach(QWidget* wid, list)
    //    {
    //        wid->hide();
    //        wid->close();

    //        ui->processingArea->layout()->removeWidget(wid);
    //        //      if (wid != this->_typeOfprocessing)
    //        //        delete wid;
    //    }

    //	ui->statusBar->showMessage(QString("Process Started %1").arg(hash));

    //    if (_StatusProgress)
    //    {
    //        _StatusProgress->setMaximum(_StatusProgress->maximum()+1);
    //        _StatusProgress->show();
    //    }
    //    else
    //    {
    //        _StatusProgress = new QProgressBar(this);
    //        this->statusBar()->addPermanentWidget(_StatusProgress);

    //        _StatusProgress->setValue(1);
    //        _StatusProgress->setFormat("%v/%m");
    //        _StatusProgress->setRange(0, 1);
    //    }


    //    if (_progress)
    //    {
    //        _progress->setMaximum(_progress->maximum()+1);
    //    }



}

void MainWindow::server_processFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit)
    {
        QMessageBox::information(this, "Processing Server unexpectedly stopped",
                                 QString("The Processing server stopped with exit code %1").arg(exitCode)
                                 );
        on_actionRe_start_Server_triggered();
    }
}

void MainWindow::processFinished()
{
    //    qDebug() << "Process Finished" << CheckoutProcess::handler().numberOfRunningProcess();
    //    if (_progress)
    //    {
    //        _progress->setValue(_progress->value()+1);
    //    }
    if (_StatusProgress)
    {
        _StatusProgress->setValue(_StatusProgress->value()+1);
    }

    if (CheckoutProcess::handler().numberOfRunningProcess() != 0)
    {
        return;
    }
    //	if (_startingProcesses) return;

    this->statusBar()->showMessage("Processing finished: All queued process are finished");

    QList<QPushButton*> list = ui->processingArea->findChildren<QPushButton*>();
    foreach(QPushButton* b, list)
        b->setEnabled(true);

    //    if (_StatusProgress)
    //    {
    //        _StatusProgress->setValue(0);
    //        _StatusProgress->setMaximum(0);
    //        _StatusProgress->hide();
    //    }

    // Auto commit delegated to the renew screen
    Screens& screens = ScreensHandler::getHandler().getScreens();
    ExperimentDataModel* datamdl = 0;
    foreach (ExperimentFileModel* mdl, screens)
        if (mdl->hasComputedDataModel() && !mdl->computedDataModel()->getCommitName().isEmpty())
        {
            int vals = mdl->computedDataModel()->commitToDatabase(mdl->hash(), mdl->computedDataModel()->getCommitName());
            //            if (vals != mapValues[mdl->name()])
            //                QMessageBox::warning(this, "Finished processes",
            //                                     QString("Warning %1 is missing values").arg(mdl->name())
            //                                     );


            mdl->reloadDatabaseData();
            mdl->computedDataModel()->setCommitName(QString());
        }


}


void MainWindow::server_processError(QProcess::ProcessError error)
{
    QString errormsg[] = {
        "Failed to Start",
        "Crashed",
        "Timeout",
        "Write to process error",
        "Read from process error",
        "Unknown error"
    };

    QMessageBox::information(this, "Processing Server unexpectedly stopped",
                             QString("Error from the process: %1 (%2)")
                             .arg(errormsg[(int)error])
            .arg((int)error)
            );

    on_actionRe_start_Server_triggered();

}



void MainWindow::prepareProcessCall()
{
    QAction* s = dynamic_cast<QAction*>(sender());
    QString process;
    if (s) {
        process = s->data().toString();
    }

    //  _typeOfprocessing->hide();
    //  _typeOfprocessing->setParent(this);

    if (process.isEmpty()) process = _preparedProcess;
    _preparedProcess = process;

    if (_preparedProcess.isEmpty()) return;

    CheckoutProcess::handler().getParameters(process);
}


template <class Widget>
Widget* setupProcessParameterInt(Widget* s, QJsonObject& par, QString def)
{
    s->setRange(par["Range/Low"].toInt(), par["Range/High"].toInt());
    if (par.contains(def))
        s->setValue(par[def].toInt());
    return s;
}

template <class Widget>
Widget* setupProcessParameterDouble(Widget* s, QJsonObject& par, QString def)
{
    s->setRange(par["Range/Low"].toDouble(), par["Range/High"].toDouble());
    if (par.contains(def))
        s->setValue(par[def].toDouble());
    return s;
}


QWidget* MainWindow::widgetFromJSON(QJsonObject& par)
{
//    qDebug() << par;
    QWidget* wid = 0;

    if (par.contains("isImage") && par["isImage"].toBool() == true)
        return wid; // skip image case...

    if (par.contains("Comment2"))
    {
        QWidget* wid = new QWidget();
        QHBoxLayout * lay = new QHBoxLayout();
        wid->setLayout(lay);
        QSpinBox* t1 = setupProcessParameterInt(new QSpinBox(), par, "Default");
        t1->setObjectName(par["Tag"].toString());
        t1->setToolTip(par["Comment"].toString());

        QSpinBox* t2 = setupProcessParameterInt(new QSpinBox(), par, "Default2");
        t2->setObjectName(par["Tag"].toString() + "2");
        t2->setToolTip(par["Comment2"].toString());

        lay->setSpacing(1);
        lay->addWidget(t1);
        lay->addWidget(t2);
        return wid;
    }
    else
        if (par.contains("isIntegral"))
        {
            if (par["isIntegral"].toBool())
            {
                wid = par["isSlider"].toBool() ?
                            (QWidget*)setupProcessParameterInt(new QSlider(Qt::Horizontal), par, "Default")
                          :
                            (QWidget*)setupProcessParameterInt(new QSpinBox(), par, "Default");
            }
            else
            {
                wid = par["isSlider"].toBool() ?
                            (QWidget*)setupProcessParameterDouble(new ctkDoubleSlider(Qt::Horizontal), par, "Default")
                          :
                            (QWidget*)setupProcessParameterDouble(new QDoubleSpinBox(), par, "Default");
            }


        }
    if (par.contains("Enum"))
    {
        QComboBox* box = new QComboBox();
        QJsonArray pp = par["Enum"].toArray();

        for (int i = 0; i < pp.size(); ++i)
        {
            box->addItem(pp[i].toString());
        }

        box->setCurrentIndex(par["Default"].toInt());

        wid = box;
    }
    if (par.contains("Type"))
    {

        if (par["Type"].toString() == "ChannelSelector")

        {
            QComboBox* box = new QComboBox();
            QList<int> list = _channelsIds.values();
            std::sort(list.begin(), list.end());

            foreach(int i, list)
            {
                box->addItem(QString("%1").arg(i));
            }

            if (par.contains("Default") && par["Default"].isDouble())
                box->setCurrentIndex(par["Default"].toInt() - 1);

            wid = box;

        }
        else
            qDebug() << "Not handled" << par ;
        /// FIXME: Add GUI for Containers & complex datatypes
    }
    if (par.contains("isString"))
    {
        ///
        /// \brief le
        if (par["isPath"].toBool())
        {
            auto le = new ctkPathLineEdit();
            if (par.contains("Default"))
            {
            le->setCurrentPath(par["Default"].toString());
            }
            wid = le;
        }
        else
        {
            auto le = new QLineEdit();
            if (par.contains("Default"))
            {
                le->setText(par["Default"].toString());
            }
            wid = le;
        }
    }
    if (wid == nullptr)
        qDebug() << "Not handled" << par;
    return wid;
}

QJsonArray MainWindow::sortParameters(QJsonArray & arr)
{
    QJsonArray rr;

    QMap<int, QList<QJsonObject> > map;

    for (int i = 0; i < arr.size(); ++i)
    {
        int p = arr[i].toObject()["Position"].toInt();
        if (p < 0) p = std::numeric_limits<int>::max();
        map[p] << arr[i].toObject();
    }

    foreach(QList<QJsonObject> list, map)
    {
        foreach(QJsonObject j, list)
        {
            rr.append(j);
        }
    }

    return rr;
}




void MainWindow::setupProcessCall(QJsonObject obj)
{

    QString process = obj["Path"].toString();
    //    qDebug() << obj;

    QList<QWidget*> list = ui->processingArea->findChildren<QWidget*>();
    _enableIf.clear();
    _disable.clear();

    foreach(QWidget* wid, list)
    {
        wid->hide();
        wid->close();
        ui->processingArea->layout()->removeWidget(wid);
    }




    QFormLayout* layo = dynamic_cast<QFormLayout*>(ui->processingArea->layout());
    if (!layo) {
        qDebug() << "Error layout not in correct format";
        return;
    }
    while (layo->count() != 0) // Check this first as warning issued if no items when calling takeAt(0).
    {
        QLayoutItem *forDeletion = layo->takeAt(0);
        forDeletion->widget()->close();
        delete forDeletion;
    }


    layo->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);

    QStringList l = process.split('/');
    //  qDebug() << l;

    QLabel * lb = new QLabel(l.back());
    lb->setObjectName(process);
    lb->setToolTip(obj["Comment"].toString());
    layo->addRow(lb);

    // FIXME: Properly handle the "Position" of parameter
    // FIXME: Properly handle other data types

    bool simpleImage = false;

    QMap<QString, QFormLayout*> mapper;

    QJsonArray params = obj["Parameters"].toArray();
    params = sortParameters(params);


    for (int i = 0; i < params.size(); ++i)
    {
        QFormLayout* lay = layo;
        QJsonObject par = params[i].toObject();

        //        qDebug() << par["Tag"];

        if (par.contains("group"))
        {
            QString nm = par["group"].toString();
            lay = mapper[nm];
            if (!lay)
            {
                ctkCollapsibleGroupBox* gbox = new ctkCollapsibleGroupBox(nm);
                gbox->setObjectName(nm);
                lay = new QFormLayout(gbox);
                lay->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);

                layo->addRow(gbox);
                mapper[nm] = lay;
            }
        }




        QWidget* wid = 0;

        if (par["PerChannelParameter"].toBool() && _channelsIds.size() > 1)
        {
            wid = new QWidget();
            QFormLayout* l = new QFormLayout(wid);
            l->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);

            //            for (unsigned i = 0; i < _numberOfChannels; ++i)
            int p = 1;
            QList<int> list = _channelsIds.values();
            std::sort(list.begin(), list.end());
            foreach(int i, list)
            {

                QWidget* w = widgetFromJSON(par);
                l->addRow(QString("Channel %1").arg(p++), w);
                w->setObjectName(QString("%1_%2").arg(par["Tag"].toString()).arg(i));

            }
        }
        else  if (par.contains("Type") && par["Type"].toString() == "Container")
        {
            if (par.contains("perChannel") && par["perChannel"].toBool())
            {
                QList<int> list = _channelsIds.values();
                std::sort(list.begin(), list.end());

                int c = 1;
                int start = par["startChannel"].toInt() + 1;
                int end = par["endChannel"].toInt();
                end = end < 0 ? list.size() : end;
                int c_def = par.contains("Default") ? par["Default"].toInt() : start;

                foreach(int i, list)
                {

                    if (c < start) { c++; continue; }
                    if (c > end) break;

                    QString nm = QString("Channel %1").arg(i);
                    lay = mapper[nm];

                    if (!lay)
                    {
                        ctkCollapsibleGroupBox* gbox = new ctkCollapsibleGroupBox(nm);
                        gbox->setObjectName(nm);
                        lay = new QFormLayout(gbox);
                        lay->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);

                        layo->addRow(gbox);
                        mapper[nm] = lay;
                    }
                    par["Type"] = par["InnerType"];

                    if (par["InnerType"].toString() == "unsigned") par["isIntegral"] = true;
                    if (par["InnerType"].toString() == "double") par["isIntegral"] = false;
                    if (par["Type"].toString() == "ChannelSelector")
                    {
                        par["Default"] = c_def; // increment the default value for each channel.
                        c_def++;
                    }
                    QWidget* w = widgetFromJSON(par);

                    if (w)
                    {
                        w->setAttribute(Qt::WA_DeleteOnClose, true);
                        w->setObjectName(par["Tag"].toString());
                        if (par["PerChannelParameter"].toBool())
                            lay->addRow(w);
                        else
                            lay->addRow(par["Tag"].toString(), w);
                        w->setToolTip(par["Comment"].toString());
                    }

                }

            }

        }
        else
            wid = widgetFromJSON(par);


        if (par.contains("ImageType") && _channelsIds.size() != 0)
        { // Per Channel Processsing add Channel Picker

            foreach(QCheckBox* b, _ChannelPicker)  delete b;
            _ChannelPicker.clear();

            QVBoxLayout *vbox = new QVBoxLayout;
            ctkCollapsibleGroupBox* gbox = new ctkCollapsibleGroupBox("Channels Selection");
            // How many channels do we have
            //            for (unsigned channels = 0; channels < _numberOfChannels; ++channels)
            int p = 1;
            QList<int> list = _channelsIds.values();
            std::sort(list.begin(), list.end());
            foreach(int channels, list)
            {
                QCheckBox* box = new QCheckBox(QString("Channel %1").arg(p++));
                box->setObjectName(QString("Channel_%1").arg(channels));
                box->setChecked(true);
                vbox->addWidget(box);
                //                gbox->
                _ChannelPicker << box;
            }
            gbox->setLayout(vbox);
            gbox->setCollapsed(true);
            lay->addRow(gbox);

            if (par["ImageType"].toString() == "ImageContainer")
                simpleImage = true;

        }

        if (wid)
        {
            if (par.contains("Level"))
            {
                //            qDebug() << par["Level"];
                if (par["Level"] == "Advanced" &&
                        !ui->actionAdvanced_User->isChecked())
                    wid->hide();

                if (par["Level"] == "VeryAdvanced" &&
                        !ui->actionVery_Advanced->isChecked())
                    wid->hide();

                if (par["Level"] == "Debug" &&
                        !ui->actionDebug->isChecked())
                    wid->hide();
            }


            wid->setAttribute(Qt::WA_DeleteOnClose, true);
            wid->setObjectName(par["Tag"].toString());
            //            qDebug() << par["Tag"].toString();
            if (par["PerChannelParameter"].toBool())
                lay->addRow(wid);
            else
                lay->addRow(par["Tag"].toString(), wid);
            wid->setToolTip(par["Comment"].toString());
            //            wid->setToolTip(par["Tag"].toString());
        }

    }


    // Add context dependent parameters
    for (int i = 0; i < params.size(); ++i)
    {
        QJsonObject par = params[i].toObject();

        if (par.contains("enableIf"))
        {
            QString rs = par["Tag"].toString();
            QWidget* ww = ui->processingArea->findChild<QWidget*>(rs);

            QJsonArray ar = par["enableIf"].toArray();
            for (int i = 0; i < ar.size(); ++i)
            {
                //                qDebug() << r << ar.at(i);
                QStringList keys = ar[i].toObject().keys();
                foreach(QString r, keys)
                {
                    QWidget* ll = ui->processingArea->findChild<QWidget*>(r);
                    int val = ar[i].toObject()[r].toInt();

                    //qDebug() << rs << ww << r << ll;

                    _enableIf[ll][val] << ww;
                    _disable[ll] << ww;

                    QComboBox* box = dynamic_cast<QComboBox*>(ll);
                    if (box)
                    {
                        //                        qDebug() << "connect";
                        connect(box, SIGNAL(currentIndexChanged(int)), this, SLOT(conditionChanged(int)));
                        box->setCurrentIndex(box->currentIndex());
                        conditionChanged(box, box->currentIndex());

                    }

                    QSpinBox* ss = dynamic_cast<QSpinBox*>(ll);
                    if (ss)
                    {
                        //                        qDebug() << "connect 2";
                        connect(ss, SIGNAL(valueChanged(int)), this, SLOT(conditionChanged(int)));
                        ss->setValue(ss->value());
                        conditionChanged(ss, ss->value());
                    }

                    QDoubleSpinBox* dss = dynamic_cast<QDoubleSpinBox*>(ll);
                    if (dss)
                    {
                        //                        qDebug() << "connect 2";
                        connect(dss, SIGNAL(valueChanged(int)), this, SLOT(conditionChanged(int)));
                        dss->setValue(dss->value());
                        conditionChanged(dss, dss->value());
                    }
                    QSlider* qs = dynamic_cast<QSlider*>(ll);
                    if (qs)
                    {
                        //                        qDebug() << "connect 2";
                        connect(qs, SIGNAL(valueChanged(int)), this, SLOT(conditionChanged(int)));
                        qs->setValue(qs->value());
                        conditionChanged(qs, qs->value());
                    }
                    ctkDoubleSlider* cds = dynamic_cast<ctkDoubleSlider*>(ll);
                    if (cds)
                    {
                        //                        qDebug() << "connect 2";
                        connect(cds, SIGNAL(valueChanged(int)), this, SLOT(conditionChanged(int)));
                        cds->setValue(cds->value());
                        conditionChanged(cds, cds->value());
                    }
                }




            }
        }


    }
    //}


    params = obj["ReturnData"].toArray();
    int resWidgets = 0;
    ctkCollapsibleGroupBox* retGbox = new ctkCollapsibleGroupBox("Optional Outputs");
    QVBoxLayout *retVbox = new QVBoxLayout;
    for (int i = 0; i < params.size(); ++i)
    {
        QJsonObject par = params[i].toObject();

        if (par.contains("Level"))
        {
            //            qDebug() << par["Level"];
            if (par["Level"] == "Advanced" &&
                    !ui->actionAdvanced_User->isChecked())
                continue;

            if (par["Level"] == "VeryAdvanced" &&
                    !ui->actionVery_Advanced->isChecked())
                continue;

            if (par["Level"] == "Debug" &&
                    !ui->actionDebug->isChecked())
                continue;
        }

        if (par["isOptional"].toBool())
        {
            resWidgets++;
            QCheckBox* wid = new QCheckBox(par["Tag"].toString());
            wid->setChecked(par["optionalState"].toBool());
            wid->setAttribute(Qt::WA_DeleteOnClose, true);
            wid->setObjectName(par["Tag"].toString());
            retVbox->addWidget(wid);
        }

    }

    if (resWidgets)
    {
        retGbox->setLayout(retVbox);
        retGbox->setCollapsed(true);
        layo->addRow(retGbox);
        //        lay->labelForField(retGbox)->setToolTip(par["Comment"].toString());
    }
    else
    {
        delete retGbox;
        delete retVbox;
    }

    if (_typeOfprocessing) delete _typeOfprocessing;
    _typeOfprocessing = new QComboBox();

    QStringList names;
    if (simpleImage) names << "Current Image";
    names << "Current Well" << "All loaded Sequences"
          << "Selected Wells"
             /* <<  "Images in View" << "Current Screen" */
          << "Selected Screens" << "All Loaded Screens";

    _typeOfprocessing->addItems(names);

    if (_shareTags) delete _shareTags;
    _shareTags = new QCheckBox("Share Tags");


    if (_commitName) delete _commitName;
    _commitName = new QLineEdit();
    _commitName->setToolTip("If non empty data will be saved to database with table having specified name");

    layo->addRow(_typeOfprocessing);
    layo->addRow(_shareTags);
    layo->addRow("Commit Name:", _commitName);

    QPushButton* button = new QPushButton("Start");
    connect(button, SIGNAL(pressed()), this, SLOT(startProcess()));
    layo->addRow(button);
}




void MainWindow::timerEvent(QTimerEvent *event)
{
    if (ui->menuProcess->actions().size() == 0)
        NetworkProcessHandler::handler().establishNetworkAvailability();

    //	if (_startingProcesses) return;


    event->accept();
    CheckoutProcess::handler().refreshProcessStatus();
    //    static int lastCount = 0;
    //    static unsigned count = 0;
    //  if (_logChanged /*&& count % 2 == 0*/)
    //    {
    //        QFile log(_logFile.fileName());

    //        if (!log.open(QIODevice::ReadOnly | QIODevice::Text))
    //            return;
    //        log.seek(lastCount);

    //        QTextStream in(&log);
    //        QString r = in.readAll();
    //        lastCount += r.size();

    //        ui->textLog->setHtml(r);

    //        QScrollBar *sb = ui->textLog->verticalScrollBar();
    //        sb->setValue(sb->maximum());
    //        //      _logChanged = false;
    //    }

    //    count++;
    //}



    //            stream << QString("Proc: %1 %2 %3\n").arg(thread).arg(ob["Path"].toString()).arg(ob["Pos"].toString());
    //            stream << QString("\t%1 %2 % (%3 %4 %)\n")
    if (_logData.size() > 1e5)
    {
        _logData.remove(0, 5e4);
    }
    ui->textLog->setPlainText(_logData);
    QScrollBar *sb = ui->textLog->verticalScrollBar();
    sb->setValue(sb->maximum());

}



void MainWindow::on_actionPython_Core_triggered()
{
#ifdef CheckoutCoreWithPython
    // Let call a separate thread with all the python stuff there
    if (!_python_interface)
    {
        setupPython();
    }


    QString script = QFileDialog::getOpenFileName(this, "Choose Python script to execute",
                                                  QDir::home().path(), "Python file (*.py)",
                                                  0, /*QFileDialog::DontUseNativeDialog
                                                  | */QFileDialog::DontUseCustomDirectoryIcons
                                                  );

    if (!script.isEmpty())
    {
        _python_interface->setSelectedWellPlates(mdl->getCheckedDirectories(false));
        _python_interface->runFile(script);

        QFile f(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/Checkout_Python_Log.txt");
        if (!f.open(QFile::ReadOnly)) return;
        _logData += f.readAll();
        ui->textLog->setPlainText(_logData);
    }
    //  ui->textLog->
    //  qDebug() << _logData;
#endif
}



void MainWindow::on_actionOptions_triggered()
{
    //	qDebug() << "Calling option widget";
    ConfigDialog* diag = new ConfigDialog();
    diag->exec();
}



void MainWindow::exec_pythonCommand()
{
#ifdef CheckoutCoreWithPython

    //  qDebug() << "Python command" << sender()->objectName();
    QLineEdit* s = qobject_cast<QLineEdit*>(sender());
    if (!s) return;
    //  qDebug() << "Running code" << s->text();
    QStringList l = mdl->getCheckedDirectories(false);
    _python_interface->setSelectedWellPlates(l);


    _python_interface->runCode(s->text());

    QFile f(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/Checkout_Python_Log.txt");
    if (!f.open(QFile::ReadOnly)) return;
    _logData = f.readAll();

    QTextBrowser* tb = ui->textLog;
    tb->setPlainText(_logData);
    //    tb->verticalScrollBar()->setValue(tb->verticalScrollBar()->maximum());
    s->clear();
#endif
}

QDoubleSpinBox* MainWindow::constructDoubleSpinbox(QHBoxLayout* popupLayout,
                                                   QWidget *popup,
                                                   ImageInfos* fo, QString objName, QString text)
{
    QDoubleSpinBox* b1 = new QDoubleSpinBox(popup);
    b1->setObjectName(objName);
    b1->setMinimum(-10 * fabs(fo->getMin()));
    b1->setMaximum( 10 * fabs(fo->getMax()));


    popupLayout->addWidget(new QLabel(text));
    popupLayout->addWidget(b1);

    return b1;
}

void MainWindow::setupPython()
{
#ifdef CheckoutCoreWithPython
    if (_python_interface) return;
    _python_interface = new CheckoutCorePythonInterface();

    connect(_python_interface, SIGNAL(addDirectory(QString)), this, SLOT(addDirectoryName(QString)));
    connect(_python_interface, SIGNAL(deleteDirectory(QString)), this, SLOT(deleteDirPath(QString)));
    connect(_python_interface, SIGNAL(loadWellplates(QStringList)), this, SLOT(loadSelection(QStringList)));
    connect(_python_interface, SIGNAL(displayWellSelection()), this, SLOT(displayWellSelection()));

    connect(_python_interface, SIGNAL(addImage(QStringList)), this, SLOT(addImageFromPython(QStringList)));

    //      this->
    QLineEdit* l = new QLineEdit();
    l->setObjectName("PythonLine");
    ui->statusBar->addWidget(l);
    connect(l, SIGNAL(returnPressed()), this, SLOT(exec_pythonCommand()));
#endif
}

void MainWindow::rangeChange(double mi, double ma)
{

    SequenceInteractor* inter = _sinteractor.current();

    QWidget* wwid = 0;
    wwid = _imageControls[inter->getExperimentName()];
    QString name = sender()->objectName().replace("Channel", "");


    //  qDebug() << "Value Range!!!" << mi << ma << name;

    if (!wwid) return;

    QList<QDoubleSpinBox*> vmi = wwid->findChildren<QDoubleSpinBox*>(QString("vMin%1").arg(name));

    if (vmi.size()) vmi.first()->setValue(mi);

    QList<QDoubleSpinBox*> vma = wwid->findChildren<QDoubleSpinBox*>(QString("vMax%1").arg(name));
    if (vma.size()) vma.first()->setValue(ma);


    ImageInfos* fo = inter->getChannelImageInfos(name.toInt() + 1);
    fo->rangeChanged(mi, ma);


}

void MainWindow::changeRangeValueMax(double val)
{
    SequenceInteractor* inter = _sinteractor.current();

    QWidget* wwid = 0;
    wwid = _imageControls[inter->getExperimentName()];
    QString name = sender()->objectName().replace("vMax", "");
    //  qDebug() << "Value Max!!!" << val << name;
    qDebug() << "Interactor: changeRangeValue " << sender()->objectName();// << fo;

    if (!wwid) return;
    QList<ctkDoubleRangeSlider*> crs = wwid->findChildren<ctkDoubleRangeSlider*>(QString("Channel%1").arg(name));
    if (crs.size()) crs.first()->setMaximumValue(val);
    if (crs.size() && crs.first()->maximum() < val) crs.first()->setMaximum(val);

    ImageInfos* fo = inter->getChannelImageInfos(name.toInt() + 1);
    fo->forceMaxValue(val);
    qDebug() << "Interactor: changeRangeValue " << sender()->objectName() << fo;

}

void MainWindow::changeRangeValueMin(double val)
{
    SequenceInteractor* inter = _sinteractor.current();

    QWidget* wwid = 0;
    wwid = _imageControls[inter->getExperimentName()];
    QString name = sender()->objectName().replace("vMin", "");
    //  qDebug() << "Value Min !!!" << val << name;
    

    qDebug() << "Interactor: changeRangeValue " << sender()->objectName();// << fo;


    if (!wwid) return;
    QList<ctkDoubleRangeSlider*> crs = wwid->findChildren<ctkDoubleRangeSlider*>(QString("Channel%1").arg(name));
    if (crs.size()) crs.first()->setMinimumValue(val);
    if (crs.size() && crs.first()->minimum() > val) crs.first()->setMinimum(val);

    ImageInfos* fo = inter->getChannelImageInfos(name.toInt() + 1);
    fo->forceMinValue(val);
    qDebug() << "Interactor: changeRangeValue " << sender()->objectName() << fo;

}

void MainWindow::changeFpsValue(double val)
{
    SequenceInteractor* inter = _sinteractor.current();
    inter->setFps(val);

    qDebug() << "Interactor: changeFps " << sender()->objectName();// << fo;

}


void MainWindow::on_actionRe_start_Server_triggered()
{
    // Avoid being told for error
    disconnect(server, SIGNAL(error(QProcess::ProcessError)),
               this, SLOT(server_processError(QProcess::ProcessError)));

    disconnect(server, SIGNAL(finished(int, QProcess::ExitStatus)),
               this, SLOT(server_processFinished(int, QProcess::ExitStatus)));

    server->close();


    connect(server, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(server_processError(QProcess::ProcessError)));

    connect(server, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(server_processFinished(int, QProcess::ExitStatus)));


    server->start();
    if (!server->waitForStarted())
        qDebug() << "Server not properly started" << server->errorString();

    NetworkProcessHandler::handler().establishNetworkAvailability();

}

void MainWindow::changeColorState(QString link)
{

    //  qDebug() << "Changing color set:" << link;

    QStringList l = link.split(",");

    SequenceInteractor* inter = _sinteractor.current();

    ImageInfos* fo = inter->getChannelImageInfos(l.at(0).toInt() + 1);
    fo->changeColorState(l.at(1).toInt());

    qDebug() << "Interactor: changeColorState " << sender()->objectName() << fo;

}



void MainWindow::on_actionPick_Intensities_toggled(bool arg1)
{

    // Change Cursor in current view!
    QCursor cur(QPixmap(":/ColorPickerCursor.png"), 4, 24);

    //    if (arg1)
    //        _images->getCurrent()->setCursor(cur);
    //    else
    //        _images->getCurrent()->setCursor(QCursor(Qt::ArrowCursor));


}

void MainWindow::on_actionStandard_toggled(bool arg1)
{
    QSettings set;

    set.setValue("UserMode/Standard", arg1);

    prepareProcessCall();
}

void MainWindow::on_actionAdvanced_User_toggled(bool arg1)
{
    QSettings set;

    set.setValue("UserMode/Advanced", arg1);
    prepareProcessCall();
}

void MainWindow::on_actionDebug_toggled(bool arg1)
{
    QSettings set;

    set.setValue("UserMode/Debug", arg1);
    prepareProcessCall();
}

void MainWindow::on_actionVery_Advanced_toggled(bool arg1)
{
    QSettings set;

    set.setValue("UserMode/VeryAdvanced", arg1);
    prepareProcessCall();
}

void MainWindow::setDataIcon()
{
    QAction* act = dynamic_cast<QAction*>(sender());

    if (!act) { qDebug() << "No action underneath..."; return; }

    int v = act->data().toInt();
    QIcon ico(QString(":/MicC%1.png").arg(v));

    if (ico.isNull())   return;

    ui->treeView->model()->setData(_icon_model, ico, Qt::DecorationRole);

    QSettings q;
    q.setValue(QString("Icons/%1").arg(ui->treeView->model()->data(_icon_model).toString()), v);
}
bool MainWindow::close()
{
    //  CheckoutProcess::handler().exitServer();
    return QMainWindow::close();
}

void MainWindow::on_action_Exit_triggered()
{
    emit close();
}

void MainWindow::on_treeView_customContextMenuRequested(const QPoint &pos)
{

    QModelIndex idx = ui->treeView->indexAt(pos);

    if (ui->treeView->rootIndex() == idx.parent())
    {
        QMenu menu(this);

        for (int i = 0; i < 15; ++i)
        {
            QIcon cmic(QString(":/MicC%1.png").arg(i));
            QAction* nmic = new QAction(cmic, tr("&Set Icon"), this);
            connect(nmic, SIGNAL(triggered()), this, SLOT(setDataIcon()));
            menu.addAction(nmic);
            nmic->setData(i);
        }

        this->_icon_model = idx;

        menu.exec(ui->treeView->mapToGlobal(pos));
    }
}



void MainWindow::on_actionOpen_Single_Image_triggered()
{
    QSettings set;
    QStringList files = QFileDialog::getOpenFileNames(this, "Choose File to open",
                                                      set.value("DirectFileOpen",QDir::home().path()).toString(), "tiff file (*.tif *.tiff);;jpeg (*.jpg *.jpeg)",
                                                      0, /* QFileDialog::DontUseNativeDialog
                                                      |*/ QFileDialog::DontUseCustomDirectoryIcons
                                                      );

    if (files.empty()) return;

    QDir path(files.first())  ;
    path.cdUp();

    //  qDebug() << path.absolutePath() << files;

    set.setValue("DirectFileOpen", path.absolutePath());

    ExperimentFileModel* r = new ExperimentFileModel();
    QCryptographicHash hash(QCryptographicHash::Md5);
    foreach (QString l, files)
        hash.addData(l.toUtf8());
    r->setProperties("hash", hash.result().toHex());
    r->setProperties("file", files.at(0));

    r->setProperties("RowCount", QString("%1").arg(1));

    r->setProperties("ColumnCount",QString("%1").arg(1));


    SequenceFileModel& seq = (*r)(0, 0);
    seq.setOwner(r);
    r->setMeasurements(QPoint(0, 0), true);

    for (int c = 0; c < files.size(); ++c)
        seq.addFile(1, 1, 1, c+1, files.at(c));


    seq.checkValidity();
    r->setFileName(path.path());
    r->setName(path.dirName());
    r->setGroupName(path.absolutePath());

    ScreensHandler::getHandler().addScreen(r);

    createWellPlateViews(r);
    //   addSelectedWells
    seq.setSelectState(true);
    if (_scrollArea)
        _scrollArea->addSelectedWells();
    // r->setGroupName(this->mdl->getGroup(mdl->fileName()));
}

void MainWindow::on_actionRe_load_servers_triggered()
{
    NetworkProcessHandler::handler().establishNetworkAvailability();

    refreshProcessMenu();

}



void MainWindow::on_actionNo_network_toggled(bool arg1)
{
    this->networking = ! arg1;
}




QString MainWindow::workbenchKey()
{
    return QString("%1").arg(ui->tabWidget->currentIndex());
}
