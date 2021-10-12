#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "screensmodel.h"

#include <map>
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

#ifdef WIN32
#include <QtWinExtras/QWinTaskbarProgress>
#endif


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

#include <QErrorMessage>
#include <QCloseEvent>
#include <guiserver.h>

#include <QColorDialog>
#include <QShortcut>

DllGuiExport QFile _logFile;

MainWindow::MainWindow(QProcess *serverProc, QWidget *parent) :
    QMainWindow(parent),
    server(serverProc),
    networking(true),
    ui(new Ui::MainWindow),
    _typeOfprocessing(0),
    //    _numberOfChannels(0),
    //	_startingProcesses(false),
    _commitName(0),
    _shareTags(0),
    _python_interface(0),
    //    _progress(0),
    _StatusProgress(0),
    StartId(0),
    _gui_server(this)
{
    ui->setupUi(this);

    ui->logWindow->hide(); // Hide the log window, since the content display is hidden now
    // QErrorMessage::qtHandler();

#ifndef CheckoutCoreWithPython
    ui->menuScripts->setVisible(false);
    ui->menuScripts->hide();
#endif

    ui->dockImageControl->hide();
    ui->dockExperimentControl->hide();

    tabifyDockWidget(ui->dockWidget, ui->dockWidget_3);


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

    ui->sync_fields->setChecked(set.value("SyncFields", true).toBool());
    ui->sync_zstack->setChecked(set.value("SyncZstack", true).toBool());


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

    connect(&CheckoutProcess::handler(), SIGNAL(updateProcessStatus(QJsonArray)),
            this, SLOT(updateProcessStatusMessage(QJsonArray)));


    connect(&CheckoutProcess::handler(), SIGNAL(processFinished(QJsonArray)),
            this, SLOT(networkProcessFinished(QJsonArray)));

    connect(&NetworkProcessHandler::handler(), SIGNAL(finishedJob()),
            this, SLOT(finishedJob()));

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

    ui->overlayInfos->hide();
    connect(ui->pickOverlay, SIGNAL(currentTextChanged(const QString &)), this, SLOT(overlay_selection(const QString&)));

    shrt_startR = new QShortcut(this);
    shrt_startR->setKey(Qt::CTRL + Qt::Key_Return);
    connect(shrt_startR, SIGNAL(activated()), this, SLOT(startProcess()));

    shrt_startEnt = new QShortcut(this);
    shrt_startEnt->setKey(Qt::CTRL + Qt::Key_Enter);
    connect(shrt_startEnt, SIGNAL(activated()), this, SLOT(startProcess()));



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
    QAction* rename = menu.addAction(tr("Rename Workbench tab"), this, SLOT(renameWorkbench()));

    QAction* addWflw = menu.addAction(tr("Add a &Workflow tab"));
    QAction* addExpWb = menu.addAction(tr("Add an &Experiment Workbench tab"), this, SLOT(addExperimentWorkbench()));

    Q_UNUSED(rename);
    Q_UNUSED(addWb);
    Q_UNUSED(addExpWb);

    //    addWb->setDisabled(true);
    addWflw->setDisabled(true);

    menu.exec(ui->tabWidget->mapToGlobal(pos));
}



void MainWindow::databaseModified()
{

    /// FIXME !!!!
    //    _mdl->select();
}

void MainWindow::channelCheckboxMenu(const QPoint & pos)
{
    QPoint globalPos = ((QWidget*) sender())->mapToGlobal(pos);

    int chan = sender()->objectName().replace("box", "").toInt();
    SequenceInteractor* inter = _sinteractor.current();
    ImageInfos* fo = inter->getChannelImageInfos(chan);


    QMenu myMenu;
    auto select = myMenu.addAction("Select only this channel");
    select->setCheckable(false);

    auto action = myMenu.addAction("Saturated Channel");
    action->setCheckable(true);
    action->setChecked(fo->isSaturated());

    auto inverted = myMenu.addAction("Inverted Signal");
    inverted->setCheckable(true);
    inverted->setChecked(fo->isInverted());


    auto binarized = myMenu.addAction("Binarize");
    binarized->setCheckable(true);
    binarized->setChecked(fo->isBinarized());

    auto submenu = myMenu.addMenu("Colormap");

    submenu->addAction("");
    submenu->addAction("inferno");
    submenu->addAction("jet");
    submenu->addAction("random");


    QAction* selectedItem = myMenu.exec(globalPos);
    if (selectedItem)
    {
        if (selectedItem == select) {

            // Loop over all other channels & unselect
            for (auto i: inter->getChannelsIds())
            {
                ImageInfos* fo = inter->getChannelImageInfos(i);
                fo->setActive(false, false);

                auto box = ui->imageControl->findChild<QCheckBox*>(QString("box%1").arg(i));
                if (box)
                {
                    box->blockSignals(true);
                    box->setChecked(false);
                    box->blockSignals(false);
                }
            }
            ImageInfos* fo = inter->getChannelImageInfos(chan);
            fo->setActive(true);

            auto box = ui->imageControl->findChild<QCheckBox*>(sender()->objectName());
            box->setChecked(true);

        }
        else if (selectedItem == action) {
            fo->toggleSaturate();
        } else if (selectedItem == inverted) {
            fo->toggleInverted();
        }  else if (selectedItem == binarized) {
            fo->toggleBinarized();
        }else {
            QString cmap = selectedItem->text();
            fo->setColorMap(cmap);
        }
        //          inter->;
    }
    else
    {
        // nothing was chosen
    }
}


QCheckBox* MainWindow::setupActiveBox(QCheckBox* box, ImageInfos* fo, int channel, bool reconnect)
{
    if (!reconnect)
    {
        box->setObjectName(QString("box%1").arg(channel));
        //        qDebug() << "Created box" << channel << box;
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->setChecked(fo->active());

        box->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(box, SIGNAL(customContextMenuRequested(const QPoint&)),
                this, SLOT(channelCheckboxMenu(const QPoint&)));

    }
    if (!fo->getChannelName().isEmpty()) // Set the Channel Name
        box->setToolTip(fo->getChannelName());

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

    //    extr->disconnect();

    //      connect(extr, SIGNAL(valueChanged(double)), fo, SLOT(forceMaxValue(double)),  Qt::UniqueConnection);
    connect(extr, SIGNAL(valueChanged(double)), this, SLOT(changeFpsValue(double)), Qt::UniqueConnection);

    return extr;
}


void MainWindow::active_Channel(bool c)
{

    SequenceInteractor* inter = _sinteractor.current();

    QString name = sender()->objectName().replace("box", "");
    ImageInfos* fo = inter->getChannelImageInfos(name.toInt() );
    fo->setActive(c);


    //    qDebug() << "Interactor: change active channel " << sender()->objectName()  << fo;

}


void MainWindow::displayTile(bool disp)
{
    if (sender())
    {
        QString name = sender()->objectName();
        _sinteractor.current()->toggleOverlay(name, disp);
    }
}

void MainWindow::setOverlayWidth(double w)
{
    _sinteractor.current()->setOverlayWidth(w);
}


void MainWindow::overlayChanged(QString id)
{
    if (sender())
    {
        QString name = sender()->objectName();
        _sinteractor.current()->overlayChange(name, id);
    }
}


void MainWindow::overlayChangedCmap(QString id)
{
    if (sender())
    {
        QString name = sender()->objectName();
        if ("Color Coding" == id)
        {
            QColor c = QColorDialog::getColor(Qt::white, this, "Set Color for overlay");
            if (c.isValid())
                id = c.name();
            else
                return;
        }
        _sinteractor.current()->overlayChangeCmap(name, id);
    }
}


void MainWindow::exportContent()
{
    if (sender())
    {
        QString name = sender()->objectName();
        QString fname = QFileDialog::getSaveFileName(this, "Save File",
                                                     QDir::homePath(), tr("csv File (*.csv)"));
        if (!fname.isEmpty())
        {
            _sinteractor.current()->exportOverlay(name, fname);
        }
    }
}


void MainWindow::setTile(int tile)
{
    if (sender())
    {
        QString name = sender()->objectName();
        _sinteractor.current()->setOverlayId(name, tile);
    }
}


ctkDoubleRangeSlider* MainWindow::RangeWidgetSetup(ctkDoubleRangeSlider* w, ImageInfos* fo, int channel, bool reconnect)
{
    if (!reconnect)
    {
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setObjectName(QString("Channel%1").arg(channel));

        QSettings set;

        double rat = set.value("RefreshSliderRatio", 0.05).toDouble();
        double dv = rat *( fo->getDispMax() - fo->getDispMin());
        w->setMinMax(fo->getDispMin() - dv,
                     fo->getDispMax() + dv);

        w->setMinimumValue(fo->getDispMin());
        w->setMaximumValue(fo->getDispMax());

    }

    w->setSymmetricMoves(false);
    connect(w, SIGNAL(valuesChanged(double, double)), this, SLOT(rangeChange(double, double)), Qt::UniqueConnection);
    connect(fo, SIGNAL(updateRange(double, double)), w, SLOT(setMinMax(double, double)));

    fo->timerEvent(NULL);

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
    ImageInfos* fo = inter->getChannelImageInfos(name.toInt() );
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
        extr->setValue(isMin ? fo->getDispMin() : fo->getDispMax());
    }

    extr->setToolTip(text);

    //    extr->disconnect();
    if (isMin)
    {
        //connect(extr, SIGNAL(valueChanged(double)), this, SLOT(changeRangeValueMin(double)), Qt::UniqueConnection);
        connect(extr, SIGNAL(editingFinished()), this, SLOT(finishedEditMinValue()), Qt::UniqueConnection);
    }
    else
    {
        //        connect(extr, SIGNAL(valueChanged(double)), this, SLOT(changeRangeValueMax(double)), Qt::UniqueConnection);
        connect(extr, SIGNAL(editingFinished()), this, SLOT(finishedEditMaxValue()), Qt::UniqueConnection);
    }
    return extr;
}



QCheckBox *MainWindow::setupOverlayBox(QCheckBox *box, QString itemName, ImageInfos *ifo, bool reconnect)
{
    //    qDebug() << "Setup Overlay Tile" << ifo;

    if (!reconnect)
    {
        box->setObjectName(QString(itemName));
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->setChecked(ifo->overlayDisplayed(itemName));
    }
    box->setToolTip(QString("Display %1 Overlay").arg(itemName));
    connect(box, SIGNAL(toggled(bool)), this, SLOT(displayTile(bool)), Qt::UniqueConnection);
    //  connect(box, SIGNAL(toggled(bool)), qApp, SLOT(aboutQt()), Qt::UniqueConnection);

    return box;
}



QDoubleSpinBox *MainWindow::setupOverlayWidth(QDoubleSpinBox *box, QString itemName, ImageInfos *ifo, bool reconnect)
{
    if (!reconnect)
    {
        box->setObjectName(QString(itemName));
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->setMinimum(0);
        box->setMaximum(50);
        box->setValue(ifo->getOverlayWidth());
    }
    box->setToolTip(QString("Overlay Width %1 Overlay").arg(itemName));
    connect(box, SIGNAL(valueChanged(double)), this, SLOT(setOverlayWidth(double)), Qt::UniqueConnection);

    return box;
}


QSpinBox *MainWindow::setupTilePosition(QSpinBox *extr, QString itemName, ImageInfos *ifo, bool reconnect)
{
    //    qDebug() << "Setup Tile pos" << ifo;
    if (!reconnect)
    {
        extr->setObjectName(itemName);

        extr->setMinimum(ifo->getOverlayMin(itemName));
        extr->setMaximum(ifo->getOverlayMax(itemName));
    }

    extr->setToolTip(QString("Pick %1 to be overlayed").arg(itemName));
    extr->setValue(ifo->getOverlayId(itemName));
    connect(extr, SIGNAL(valueChanged(int)), this, SLOT(setTile(int)), Qt::UniqueConnection);

    return extr;
}




QComboBox *MainWindow::setupOverlaySelection(QComboBox *box, QString itemName,ImageInfos *ifo, bool reconnect)
{
    if (!reconnect)
    {
        box->setObjectName(itemName);
        box->addItems(ifo->getInteractor()->getMetaOptionsList(itemName));
    }

    box->setToolTip(QString("Select Colorcoding"));
    box->setCurrentText(ifo->getInteractor()->getOverlayCode(itemName));
    connect(box, SIGNAL(currentTextChanged(QString)), this, SLOT(overlayChanged(QString)));

    return box;
}

QComboBox *MainWindow::setupOverlayColor(QComboBox *box, QString itemName,ImageInfos *ifo, bool reconnect)
{
    QStringList colors = QStringList() << "jet" << "inferno" << "random";

    if (!reconnect)
    {
        box->setObjectName(itemName);
        box->addItems(QStringList(colors) << "Color Coding");
    }

    box->setToolTip(QString("Select Color map"));
    QString color = ifo->getInteractor()->getOverlayColor(itemName);

    if (!colors.contains(color))
        color = "Color Coding";

    box->setCurrentText(color);
    connect(box, SIGNAL(currentTextChanged(QString)), this, SLOT(overlayChangedCmap(QString)));

    return box;
}



void MainWindow::updateCurrentSelection()
{
    SequenceInteractor* inter = _sinteractor.current();

    if (!inter->currentChanged())
        return;

    SequenceViewContainer & container = SequenceViewContainer::getHandler();
    container.setCurrent(inter->getSequenceFileModel());
    _channelsIds = inter->getSequenceFileModel()->getChannelsIds();
    /// Change the plate tab widget to focus on the proper plate view

    QList<QWidget*> toDel;

    foreach(QList<QWidget*> widl, _imageControls.values())
        foreach(QWidget* wid, widl)
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

    //    QVBoxLayout* bvl = new QVBoxLayout(wwid);
    QGridLayout* bvl = new QGridLayout(wwid);



    bvl->setSpacing(1);
    bvl->setContentsMargins(0,0,0,0);
    QSize pix;
    //  qDebug() << "Creating Controls" << channels;
    int i = 0;
    std::list<int> chList(_channelsIds.begin(), _channelsIds.end());
    //std::sort(chList.begin(), chList.end()); // sort channel number
    chList.sort();

    for (auto s: shrt_binarize) delete s;
    shrt_binarize.clear();
    int lastPal = 0;

    for (auto trueChan : chList)
    {
        ImageInfos* fo = inter->getChannelImageInfos(trueChan);

        //          qDebug() << "Adding " << i << QString("Channel%1").arg(i);
        pix = fo->imSize();
        if (Q_UNLIKELY(!fo))        return;

        int ncolors = fo->getMax() - fo->getMin();


        if (fo->getMin() >= 0 && fo->getMax() - fo->getMin() < 16)
        {
            bvl->addWidget(setupActiveBox(new QCheckBox(wwid), fo, trueChan), i, 0);
            auto tw = new QWidget(wwid);
            tw->setLayout(new QHBoxLayout());

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
                                         , wwid);
                lbl->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
                connect(lbl, SIGNAL(linkActivated(QString)), this, SLOT(changeColorState(QString)), Qt::UniqueConnection);
                tw->layout()->addWidget(lbl);
            }
            bvl->addWidget(tw, i, 1, 1, -1);
            lastPal += ncolors;
        }
        else
        {
            bvl->addWidget(setupActiveBox(new QCheckBox(wwid), fo, trueChan), i, 0);
            bvl->addWidget(setupMinMaxRanges(new QDoubleSpinBox(wwid), fo, QString("vMin%1").arg(trueChan), true), i, 1);
            bvl->addWidget(RangeWidgetSetup(new ctkDoubleRangeSlider(Qt::Horizontal, wwid), fo, trueChan), i, 2);
            bvl->addWidget(setupMinMaxRanges(new QDoubleSpinBox(wwid), fo, QString("vMax%1").arg(trueChan), false), i, 3);
            bvl->addWidget(colorWidgetSetup(new ctkColorPickerButton(wwid), fo, trueChan), i, 4);

            shrt_binarize.append(new QShortcut(this));
            shrt_binarize.last()->setKey(QKeySequence(Qt::CTRL+Qt::Key_B, Qt::Key_0+i));
            shrt_binarize.last()->setObjectName(QString("%1").arg(trueChan));
            connect(shrt_binarize.last(), &QShortcut::activated, this, [this](){
                int trueChan = this->sender()->objectName().toInt();
                this->_sinteractor.current()->getChannelImageInfos(trueChan)->toggleBinarized();
            });
        }



        _imageControls[inter->getExperimentName()].append(wwid);
        ++i;
    }

    // Add FrameRate control if it makes sense
    if (inter->getTimePointCount() > 1) {
        bvl->addWidget(setupVideoFrameRate(new QDoubleSpinBox(wwid), QString("Video Frame Rate")), channels, 0, 1, -1);
    }

    ui->imageControl->layout()->addWidget(wwid);


    { // Overlay control
        ImageInfos* fo = inter->getChannelImageInfos(*_channelsIds.begin());

        auto wwid = new QWidget;

        QGridLayout* bvl = new QGridLayout(wwid);
        bvl->setSpacing(1);
        bvl->setContentsMargins(0, 0, 0, 0);
        bvl->addWidget(new QLabel("Overlay width", wwid), 0, 1);
        bvl->addWidget(setupOverlayWidth(new QDoubleSpinBox(wwid), "OverlayWidth", fo), 0, 2, 1, -1);


        bvl->addWidget(setupOverlayBox(new QCheckBox(wwid), "Tile", fo), 1, 0);
        bvl->addWidget(new QLabel("Tile: ", wwid), 1, 1);
        bvl->addWidget(setupTilePosition(new QSpinBox(wwid), "Tile", fo), 1, 2, 1, -1);

        ui->overlayControl->layout()->addWidget(wwid);
        _imageControls[inter->getExperimentName()].append(wwid);


        bvl->addWidget(setupOverlayBox(new QCheckBox(wwid), "Scale", fo), 2, 0);
        bvl->addWidget(new QLabel("Scale: ", wwid), 2, 1);
        bvl->addWidget(setupTilePosition(new QSpinBox(wwid), "Scale", fo), 2, 2);


        QStringList overlays = inter->getMetaList();
        int itms = 3;

        ui->pickOverlay->clear();
        ui->pickOverlay->insertItems(0, overlays);


        for (auto & ov : overlays)
        {
            if (ui->overlayInfos->isHidden()) ui->overlayInfos->show();
            bvl->addWidget(setupOverlayBox(new QCheckBox(wwid), ov, fo), itms, 0);
            bvl->addWidget(new QLabel(ov, wwid), itms, 1);
            bvl->addWidget(setupTilePosition(new QSpinBox(wwid), ov, fo), itms, 2);
            bvl->addWidget(setupOverlaySelection(new QComboBox(wwid), ov, fo), itms, 3);
            bvl->addWidget(setupOverlayColor(new QComboBox(wwid), ov, fo), itms, 4);
            auto but = new QPushButton("Export");
            but->setObjectName(ov);
            connect(but, SIGNAL(clicked()), this, SLOT(exportContent()));
            bvl->addWidget(but, itms, 5);
            itms++;
        }


        wwid->show();
    }

    wwid->show();

    ui->treeWidget->clear();
    QList<QTreeWidgetItem*> items;

    QStringList file = inter->getFileName().split("/", Qt::SkipEmptyParts);

    file.pop_back();
    QString exp = file.last(); file.pop_back();
    QString own = file.last();


    items.append(new QTreeWidgetItem(QStringList() << "Project" << QString("%1").arg(inter->getProjectName())));
    items.append(new QTreeWidgetItem(QStringList() << "XP" << QString("%1").arg(inter->getExperimentName())));
    items.append(new QTreeWidgetItem(QStringList() << "File" << QString("%2 %1").arg(exp).arg(own)));
    items.append(new QTreeWidgetItem(QStringList() << "Well" << QString("%1").arg(inter->getSequenceFileModel()->Pos())));
    items.append(new QTreeWidgetItem(QStringList() << "Z" << QString("%2/%1").arg(inter->getZCount()).arg(inter->getZ())));
    items.append(new QTreeWidgetItem(QStringList() << "timePoints" << QString("%2/%1").arg(inter->getTimePointCount()).arg(inter->getTimePoint())));
    items.append(new QTreeWidgetItem(QStringList() << "Fields" << QString("%2/%1").arg(inter->getFieldCount()).arg(inter->getField())));
    items.append(new QTreeWidgetItem(QStringList() << "Channels" << QString("%1").arg(channels)));

    items.append(new QTreeWidgetItem(QStringList() << "Size" << QString("%1x%2").arg(pix.width()).arg(pix.height())));

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

void MainWindow::clearDirectories()
{
    QStandardItem* root = mdl->invisibleRootItem();
    QStringList l;
    for (int i = 0; i < root->rowCount(); ++i)
    {
        QStandardItem* item = root->child(i);
        l << item->data(Qt::ToolTipRole).toString();
    }

    for (auto s: l)  deleteDirPath(s);

}

void MainWindow::conditionChanged(int val)
{
    QWidget* sen = dynamic_cast<QWidget*>(sender());
    conditionChanged(sen, val);


}

//void MainWindow::processStarted(QString hash)
//{
//}


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
    if (_StatusProgress)
    {
        _StatusProgress->setValue(_StatusProgress->value()+1);
    }

    if (CheckoutProcess::handler().numberOfRunningProcess() != 0)
    {
        return;
    }

    QList<QPushButton*> list = ui->processingArea->findChildren<QPushButton*>();
    foreach(QPushButton* b, list)
        b->setEnabled(true);

    // Auto commit delegated to the renew screen
    Screens& screens = ScreensHandler::getHandler().getScreens();
    qDebug() << "Saving data for screens #" << screens.size();
    foreach (ExperimentFileModel* mdl, screens)
    {
        qDebug() <<  mdl->hasComputedDataModel();
        if ( mdl->hasComputedDataModel())
            qDebug() <<  mdl->computedDataModel()->getCommitName();

        if (mdl->hasComputedDataModel() && !mdl->computedDataModel()->getCommitName().isEmpty())
        {
            int vals = mdl->computedDataModel()->commitToDatabase(mdl->hash(), mdl->computedDataModel()->getCommitName());
            Q_UNUSED(vals);

            mdl->reloadDatabaseData();
            mdl->computedDataModel()->setCommitName(QString());
        }
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

int getValInt(QJsonObject& par, QString v)
{
    int low = 0;

    if (par.contains(v))
    {
        if (par[v].isArray())
            low = par[v].toArray()[0].toInt();
        else
            low = par[v].toInt();
    }
    return low;
}

double getValDouble(QJsonObject& par, QString v)
{
    double low = 0;

    if (par.contains(v))
    {
        if (par[v].isArray())
            low = par[v].toArray()[0].toDouble();
        else
            low = par[v].toDouble();
    }
    return low;
}


template <class Widget>
Widget* setupProcessParameterInt(Widget* s, QJsonObject& par, QString def)
{
    s->setRange(getValInt(par, "Range/Low"), getValInt(par,"Range/High"));
    if (par.contains(def))
        s->setValue(getValInt(par, def));
    return s;
}

template <class Widget>
Widget* setupProcessParameterDouble(Widget* s, QJsonObject& par, QString def)
{
    s->setRange(getValDouble(par,"Range/Low"), getValDouble(par,"Range/High"));
    if (par.contains(def))
        s->setValue(getValDouble(par,def));
    return s;
}


QWidget* MainWindow::widgetFromJSON(QJsonObject& par, bool reloaded)
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
        QDoubleSpinBox* t1 = setupProcessParameterDouble(new QDoubleSpinBox(), par, reloaded ? "Value" : "Default" );
        t1->setObjectName(par["Tag"].toString());
        t1->setToolTip(par["Comment"].toString());

        QDoubleSpinBox* t2 = setupProcessParameterDouble(new QDoubleSpinBox(), par, reloaded ? "Value2" : "Default2");
        t2->setObjectName(par["Tag"].toString() + "2");
        t2->setToolTip(par["Comment2"].toString());


        if (par.contains("IsSync") && par["IsSync"].toBool())
        {
            int chan = par["guiChan"].toInt();

            auto miname = QString("vMin%1").arg(chan),
                    maname = QString("vMax%1").arg(chan);



            auto wi = findChild<QDoubleSpinBox*>(miname), wa = findChild<QDoubleSpinBox*>(maname);

            if (!reloaded)
            {
                if (wi)
                    t1->setValue(wi->value());
                if (wa)
                    t2->setValue(wa->value());
            }
            _syncmapper[miname] = t1;
            _syncmapper[maname] = t2;
        }

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
                            (QWidget*)setupProcessParameterInt(new QSlider(Qt::Horizontal), par,  reloaded ? "Value" : "Default")
                          :
                            (QWidget*)setupProcessParameterInt(new QSpinBox(), par,  reloaded ? "Value" : "Default");
            }
            else
            {
                wid = par["isSlider"].toBool() ?
                            (QWidget*)setupProcessParameterDouble(new ctkDoubleSlider(Qt::Horizontal), par,  reloaded ? "Value" : "Default")
                          :
                            (QWidget*)setupProcessParameterDouble(new QDoubleSpinBox(), par,  reloaded ? "Value" : "Default");
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
        if (reloaded && par.contains("Value"))
            box->setCurrentText(par["Value"].toString());
        else
            box->setCurrentIndex(par["Default"].toInt());

        wid = box;
    }
    if (par.contains("Type"))
    {

        if (par["Type"].toString() == "ChannelSelector")

        {
            QComboBox* box = new QComboBox();
            box->setObjectName("Channels");
            QList<int> list = _channelsIds.values();
            std::sort(list.begin(), list.end());

            foreach(int i, list)
            {
                if (_channelsNames.contains(i))
                    box->addItem(_channelsNames[i]);
                else
                    box->addItem(QString("%1").arg(i));
            }
            if (reloaded && par.contains("Value"))
            {
                if (par["Value"].isArray())
                    box->setCurrentText(par["Value"].toArray().at(0).toString());
                else
                    box->setCurrentText(par["Value"].toString());
            }
            else if (par.contains("Default") && par["Default"].isDouble())
                box->setCurrentIndex(par["Default"].toInt() - 1);

            wid = box;
        }
        //        else
        //            qDebug() << "Not handled" << par ;
        /// FIXME: Add GUI for Containers & complex datatypes
    }
    if (par.contains("isString"))
    {
        ///
        /// \brief le
        if (par["isPath"].toBool() )
        {
            auto le = new ctkPathLineEdit();
            if (reloaded && par.contains("Value") && !par["Value"].toString().isEmpty())
                le->setCurrentPath(par["Value"].toString());
            else if (par.contains("Default"))
            {
                le->setCurrentPath(par["Default"].toString());
            }
            QSettings set;
            if (par["isDbPath"].toBool())
            {
                QString v = set.value("databaseDir", par["Default"].toString()).toString();

                // retrieve selection name ?

                ScreensHandler& handler = ScreensHandler::getHandler();
                auto screens = handler.getScreens();
                if (screens.size())
                    v=QString("%1/%2/Checkout_Results/").arg(v).arg(screens[0]->property("project"));
                le->setCurrentPath(v);
            }
            wid = le;
        }
        else
        {
            auto le = new QLineEdit();
            if (reloaded && par.contains("Value"))
                le->setText(par["Value"].toString());
            else if (par.contains("Default"))
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

void constructHistoryComboBox(QComboBox* cb, QString process)
{
    // Seek for processed data
    // Project ?
    // L:/{Project}/Checkout_Results/*/{processpath}_date_time.json :)
    // Order data by date/time ,

    QSet<QString> projects;
    for (auto scr : ScreensHandler::getHandler().getScreens())
        projects.insert(scr->property("project"));

    QSettings set;
    QDir dir(set.value("databaseDir").toString());

    QStringList jsons, commits;
    for (QString proj: projects)
    {

        QString writePath = QString("%1/%2/Checkout_Results/").arg(dir.absolutePath(), proj);
        QDir prdir(writePath);
        QStringList dirs = prdir.entryList(QStringList() << "*", QDir::Dirs | QDir::NoDotAndDotDot);
        for (auto dir : dirs)
        {
            QDir gjsons(writePath+"/"+dir);
            QStringList df = gjsons.entryList(QStringList() << process.replace("/", "_").replace(" ", "_") + "*.json", QDir::Files);
            jsons.append(df);
            for (QString s: df)
                commits << writePath+"/"+dir + "/" + s;
        }
    }
    // sooooo many !!!!
    jsons.sort();
    //    qDebug() << jsons << commits;
    QStringList disp, paths;
    disp << "Default";
    paths << QString();

    for (auto it = jsons.rbegin(), e= jsons.rend(); it != e; ++it)
    {
        for (auto r: commits)
            if (r.contains(*it))
            {
                paths << r;
                QStringList path = r.split("/");
                QString commitName = path.at(path.size() - 2);
                if (commitName == "params")
                {
                    commitName = "";
                    continue;
                }
                QStringList j = (*it).split("_");

                QString hours = j.takeLast();
                hours = hours.mid(0, hours.size()-5);

                QString date = j.takeLast();

                disp << QString("%1 %2 %3:%4:%5").arg(commitName, date, hours.mid(0, 2), hours.mid(2,2), hours.mid(4));
            }
        if (disp.size() > 9)
            break;
    }


    for (int i = 0; i < disp.size(); ++i)
        cb->addItem(disp.at(i), paths.at(i));
}


void MainWindow::setupProcessCall(QJsonObject obj, int idx)
{
    bool reloaded = idx > 0;


    _syncmapper.clear();

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

    auto cb = new QComboBox();
    layo->addRow(cb);
    constructHistoryComboBox(cb, process);
    if (idx > 0) cb->setCurrentIndex(idx);
    connect(cb, SIGNAL(currentTextChanged(QString)), this, SLOT(on_pluginhistory(QString)));

    if (idx < 0 && cb->count() > 1)
    {
        cb->setCurrentIndex(1);
        return;
    }

    // FIXME: Properly handle the "Position" of parameter2
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

        if (par["PerChannelParameter"].toBool() && _channelsIds.size() > 1 && !par.contains("Type") )
        {
            wid = new QWidget();
            QFormLayout* l = new QFormLayout(wid);
            l->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);

            //            for (unsigned i = 0; i < _numberOfChannels; ++i)
            int p = 1;
            QList<int> list = _channelsIds.values();
            std::sort(list.begin(), list.end());
            QJsonArray ar,ar2;
            if (reloaded && par.contains("Value") && par["Value"].isArray())
            {
                ar = par["Value"].toArray();
                if (par.contains("Value2"))
                    ar2 = par["Value2"].toArray();
            }
            foreach(int channels, list)
            {

                par["guiChan"] = channels-1;

                if (ar.size())
                {
                    par["Value"]=ar[channels-1];
                    par["Value2"]=ar2[channels-1];
                }


                QWidget* w = widgetFromJSON(par, reloaded);
                if (w)
                {
                    QString nm;

                    if (_channelsNames.size() == _channelsIds.size())
                        nm = QString(_channelsNames[list.at(channels-1)]);
                    else
                        nm = QString("Channel %1").arg(p++);

                    l->addRow(nm, w);

                    w->setObjectName(QString("%1_%2").arg(par["Tag"].toString()).arg(channels));
                    //                    qDebug() << "Created Widget:" << w << w->objectName();

                }

            }
        }
        else  if (par.contains("Type") && par["Type"].toString() == "Container")
        {
            if (par.contains("PerChannelParameter") && par["PerChannelParameter"].toBool())
            {

                QList<int> list = _channelsIds.values();
                std::sort(list.begin(), list.end());

                int c = 1;
                int start = par["startChannel"].toInt() + 1;
                int end = par["endChannel"].toInt();
                end = end < 0 ? list.size() : end;
                int c_def = par.contains("Default") ? par["Default"].toInt() : start;
                //                qDebug() << "Mapping data Per channel value";
                foreach(int i, list)
                {

                    if (c < start) { c++; continue; }
                    if (c > end) break;

                    QString nm;

                    if (_channelsNames.size() == _channelsIds.size())
                        nm = QString(_channelsNames[list.at(i - 1)]);
                    else
                        nm = QString("Channel %1").arg(i);

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
                        if (reloaded && par.contains("Value"))
                        {
                            if (par["Value"].isArray())
                                par["Default"] = par["Value"].toArray().at(c);
                            else
                                par["Default"] = par["Value"];
                        }
                        else {
                            par["Default"] = c_def; // increment the default value for each channel.
                            c_def++;
                        }
                    }
                    //                    qDebug() << c;

                    QWidget* w = widgetFromJSON(par, reloaded);
                    if (w)
                    {
                        w->setAttribute(Qt::WA_DeleteOnClose, true);
                        w->setObjectName(QString("%1_%2").arg(par["Tag"].toString()).arg(c));
                        lay->addRow(par["Tag"].toString(), w);
                        w->setToolTip(par["Comment"].toString());
                    }
                    c++;

                }

            }

        }
        else
            wid = widgetFromJSON(par, reloaded);


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
                QCheckBox* box = nullptr;
                if (_channelsNames.size() == _channelsIds.size())
                    box = new QCheckBox(_channelsNames[list.at(channels-1)]);
                else
                    box = new QCheckBox(QString("Channel %1").arg(p++));
                box->setObjectName(QString("Channel_%1").arg(channels));
                box->setChecked(true);
                vbox->addWidget(box);
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
            if (!wid->objectName().isEmpty())
                qDebug() << "Created Widget:" << wid << wid->objectName();

            bool show = true;

            if (par.contains("Level"))
            {
                //            qDebug() << par["Level"];
                if (par["Level"] == "Advanced" &&
                        !ui->actionAdvanced_User->isChecked())
                    show=false;

                if (par["Level"] == "VeryAdvanced" &&
                        !ui->actionVery_Advanced->isChecked())
                    show=false;

                if (par["Level"] == "Debug" &&
                        !ui->actionDebug->isChecked())
                    show=false;
            }

            if (par.contains("hidden") && par["hidden"].toBool())
                  show = false;



                wid->setAttribute(Qt::WA_DeleteOnClose, true);
                wid->setObjectName(par["Tag"].toString());
                //            qDebug() << par["Tag"].toString();
                if (par["PerChannelParameter"].toBool() || !show)
                    lay->addRow(wid);
                else
                    lay->addRow(par["Tag"].toString(), wid);
                wid->setToolTip(par["Comment"].toString());

                if (!show) wid->hide();

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
          << "Selected Screens" << "Selected Screens and Filter" << "All Loaded Screens";

    _typeOfprocessing->addItems(names);
    if (_scrollArea->items() == 0)
        _typeOfprocessing->setCurrentText( "Selected Screens");//Index(_typeOfprocessing->setCurrentText())


    if (_shareTags) delete _shareTags;
    _shareTags = new QCheckBox("Share Tags");
    // From now on, we force the transfert of tags to the server & back
    _shareTags->setChecked(true);
    _shareTags->hide();


    if (_commitName) delete _commitName;
    _commitName = new QLineEdit();
    _commitName->setToolTip("If non empty data will be saved to database with table having specified name");

    layo->addRow(_typeOfprocessing);
    //    layo->addRow(_shareTags);

    if (_preparedProcess.endsWith("Generate BirdView"))
    {
        _commitName->setText("BirdView");
        layo->addRow(_commitName);
        _commitName->hide();
    }
    else
        layo->addRow("Commit Name:", _commitName);

    QPushButton* button = new QPushButton("Start");
    button->setObjectName("ProcessStartButton");
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



void lockedChangeValue(QDoubleSpinBox* sp, double val)
{
    sp->blockSignals(true);
    sp->setValue(val);
    sp->blockSignals(false);
}


void MainWindow::rangeChange(double mi, double ma)
{

    SequenceInteractor* inter = _sinteractor.current();

    foreach(QWidget* wwid, _imageControls[inter->getExperimentName()])
    {
        QString name = sender()->objectName().replace("Channel", "");

        if (!wwid) return;

        QList<QDoubleSpinBox*> vmi = wwid->findChildren<QDoubleSpinBox*>(QString("vMin%1").arg(name));

        if (vmi.size()) {
            lockedChangeValue(vmi.first(), mi);
        }
        QList<QDoubleSpinBox*> vma = wwid->findChildren<QDoubleSpinBox*>(QString("vMax%1").arg(name));
        if (vma.size()) lockedChangeValue(vma.first(), ma);


        ImageInfos* fo = inter->getChannelImageInfos(name.toInt() );
        fo->rangeChanged(mi, ma);
    }

    QString pos=sender()->objectName().replace("Channel", "");

    if (_syncmapper.contains(QString("vMin%1").arg(pos)))
    {
        _syncmapper[QString("vMin%1").arg(pos)]->setValue(mi);
    }
    if (_syncmapper.contains(QString("vMax%1").arg(pos)))
    {
        _syncmapper[QString("vMax%1").arg(pos)]->setValue(ma);
    }

}

void MainWindow::udpateRange(double mi, double ma)
{

    SequenceInteractor* inter = _sinteractor.current();

    foreach(QWidget* wwid, _imageControls[inter->getExperimentName()])
    {
        QString name = sender()->objectName().replace("Channel", "");
        //sender()
        QList<ctkDoubleRangeSlider*> ranges = wwid->findChildren<ctkDoubleRangeSlider*>(name);// qobject_cast<ctkDoubleRangeSlider*>(sender());
        //        //qDebug() << "Value Range!!!" << mi << ma << name << range->minimum() << range->maximum();
        for (auto range: ranges)
            if (range)
            {
                int th =0;// abs(ma - mi) > 1000 ? 100 : 10;
                range->setMinimum(mi-th);
                range->setMaximum(ma+th);
            }

    }
    QString pos=sender()->objectName().replace("Channel", "");

    if (_syncmapper.contains(QString("vMin%1").arg(pos)))
    {
        _syncmapper[QString("vMin%1").arg(pos)]->setValue(mi);
    }
    if (_syncmapper.contains(QString("vMax%1").arg(pos)))
    {
        _syncmapper[QString("vMax%1").arg(pos)]->setValue(ma);
    }

}

void MainWindow::finishedEditMaxValue()
{
    auto spin = qobject_cast<QDoubleSpinBox*>(sender());
    changeRangeValueMax(spin->value());
}

void MainWindow::changeRangeValueMax(double val)
{
    SequenceInteractor* inter = _sinteractor.current();

    foreach (QWidget* wwid, _imageControls[inter->getExperimentName()])
    {
        QString name = sender()->objectName().replace("vMax", "");
        //  qDebug() << "Value Max!!!" << val << name;
        //    qDebug() << "Interactor: changeRangeValue " << sender()->objectName();// << fo;

        if (!wwid) return;
        QList<ctkDoubleRangeSlider*> crs = wwid->findChildren<ctkDoubleRangeSlider*>(QString("Channel%1").arg(name));
        if (crs.size()) crs.first()->setMaximumValue(val);
        if (crs.size() && crs.first()->maximum() < val) crs.first()->setMaximum(val);

        ImageInfos* fo = inter->getChannelImageInfos(name.toInt());
        fo->forceMaxValue(val);
    }
    if (_syncmapper.contains(sender()->objectName()))
    {
        _syncmapper[sender()->objectName()]->setValue(val);
    }

}

void MainWindow::finishedEditMinValue()
{
    auto spin = qobject_cast<QDoubleSpinBox*>(sender());
    changeRangeValueMin(spin->value());
}

void MainWindow::changeRangeValueMin(double val)
{
    SequenceInteractor* inter = _sinteractor.current();

    foreach (QWidget* wwid, _imageControls[inter->getExperimentName()])
    {
        QString name = sender()->objectName().replace("vMin", "");
        //  qDebug() << "Value Min !!!" << val << name;

        if (!wwid) return;
        QList<ctkDoubleRangeSlider*> crs = wwid->findChildren<ctkDoubleRangeSlider*>(QString("Channel%1").arg(name));
        if (crs.size()) crs.first()->setMinimumValue(val);
        if (crs.size() && crs.first()->minimum() > val) crs.first()->setMinimum(val);

        ImageInfos* fo = inter->getChannelImageInfos(name.toInt());
        fo->forceMinValue(val);
    }

    if (_syncmapper.contains(sender()->objectName()))
    {
        _syncmapper[sender()->objectName()]->setValue(val);
    }
}

void MainWindow::changeFpsValue(double val)
{
    SequenceInteractor* inter = _sinteractor.current();
    inter->setFps(val);

    //    qDebug() << "Interactor: changeFps " << sender()->objectName();// << fo;

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

    //    qDebug() << "Interactor: changeColorState " << sender()->objectName() << fo;

}



void MainWindow::on_actionPick_Intensities_toggled(bool )
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

void MainWindow::rmDirectory()
{
    QStandardItem* root = mdl->invisibleRootItem();
    //    root->data()
    QString dir = root->child(_icon_model.row())->data(Qt::ToolTipRole).toString();

    QSettings set;
    QStringList l = set.value("ScreensDirectory", QVariant(QStringList())).toStringList();
    l.removeAll(dir);

    set.setValue("ScreensDirectory", l);



    root->removeRow(_icon_model.row());
}

void MainWindow::loadPlate()
{
    // Set selection
    mdl->setData(_icon_model, Qt::Checked, Qt::CheckStateRole);
    on_loadSelection_clicked();
}

void MainWindow::loadPlateFirst()
{
    // Set selection
    mdl->setData(_icon_model, Qt::Checked, Qt::CheckStateRole);
    on_loadSelection_clicked();

    ScreensHandler& h = ScreensHandler::getHandler();
    Screens& s = h.getScreens();
    if (!s.size()) return;
    if (s.front())
    {
        auto l = s.front()->getValidSequenceFiles();
        if (l.size())
            l.front()->setSelectState(true);
    }
    this->_scrollArea->addSelectedWells();

}

void MainWindow::loadPlateDisplay3()
{
    mdl->setData(_icon_model, Qt::Checked, Qt::CheckStateRole);
    on_loadSelection_clicked();

    ScreensHandler& h = ScreensHandler::getHandler();
    Screens& s = h.getScreens();
    if (s.front())
    {
        auto l = s.front()->getValidSequenceFiles();
        if (l.size())
        {
            l.front()->setSelectState(true);
            l.back()->setSelectState(true);
            int ss = l.size();
            if (ss > 3)
            {
                auto it = l.begin();
                std::advance(it, ss/2);
                (*it)->setSelectState(true);
            }
        }
    }
    this->_scrollArea->addSelectedWells();

}

int longestMatch(QString a, QString b)
{
    int i = 0;
    for (; i < a.size() && i < b.size(); ++i)
        if (a[i] != b[i])
            break;
    return i;
}
#include <QInputDialog>

void MainWindow::exportToCellProfiler()
{

    bool ok;

    QString filtertags = QInputDialog::getText(this,
                                               "Set Filter condition (separated by ';' )",
                                               "Part of tag or Well Regexp 'W:' followed by regexp:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QStringList tag_filter = filtertags.isEmpty() ? QStringList() :
                                                    filtertags.split(';');
    QStringList remTags;
    QRegExp wellMatcher, siteMatcher;

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
    }
    for (auto s: remTags) tag_filter.removeAll(s);

    // Check Plate selection Load plate if necessary
    if (mdl->getCheckedDirectories(false).size() == 0)
        mdl->setData(_icon_model, Qt::Checked, Qt::CheckStateRole);

    on_loadSelection_clicked();

    // Get all selected screens
    ScreensHandler& h = ScreensHandler::getHandler();
    Screens& s = h.getScreens();

    QString dir = QFileDialog::getSaveFileName(this, tr("Save File"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/data_cellprofiler.csv", tr("CSV file (excel compatible) (*.csv)"),
                                               0, /*QFileDialog::DontUseNativeDialog
                                                                                                                                                                                                                                                                                                                                                                                                                                                  | */QFileDialog::DontUseCustomDirectoryIcons
                                               );
    if (dir.isEmpty()) return;




    // Now we have all :) Lets set a CSV file with all the metadata for CellProfiler

    // Set up the headers
    std::map<QString, QString> values;
    QString basePath;
    values["Metadata_Plate"]="";
    values["Metadata_Well"]="";
    values["Metadata_Site"]="";
    values["Metadata_Time"]="";
    values["Metadata_Z"]="";
    QSet<QString> tags;
    QSet<QString> chans;
    for (auto xp: s)
    {
        QString plate_name = xp->name();
        QString path = xp->fileName();
        if (basePath.isEmpty())
            basePath = path.left(path.indexOf(plate_name)-1);
        else
            basePath = basePath.left(longestMatch(basePath, path));

        QStringList cname = xp->getChannelNames();
        for (auto a: cname) chans.insert(a.trimmed().replace(" ", "_"));

        for (auto seq: xp->getValidSequenceFiles())
        {
            QStringList t = seq->getTags();
            for (auto a: t)
                tags.insert(a);
        }
    }
    // "DCM-Tum-lines-seeded-for-6k-D9-4X"
    // "C:/Data/DCM/DCM-Tum-lines-seeded-for-6k-D9-4X_20200702_110209/DCM-Tum-lines-seeded-for-6k-D9-4X/MeasurementDetail.mrf"
    //("Hoechst 33342", "2", "3")
    // QSet("D-001-Cb-08", "D-001-Cc-01", "W-004-017", "D-001-035 (3232)", "D-001-Ca-12")
    qDebug() << basePath << chans << tags;
    for (auto c : chans)
    {
        values[QString("Image_FileName_%1").arg(c)]=QString();
        values[QString("Image_PathName_%1").arg(c)]=QString();
    }
    QSet<QString> titration,meta;
    for (auto c : tags)
    {
        if (c.contains('#'))
        {
            c =  c.split('#').front();
            if (!c.isEmpty())
                titration.insert(c);
        }
        else
            meta.insert(c);
    }

    for (auto c : meta) values[QString("Metadata_%1").arg(c.trimmed().replace(" ", "_"))]=QString("0");
    for (auto c : titration) values[QString("Titration_%1").arg(c.trimmed().replace(" ", "_"))]=QString("0");
    values["Metadata_Tags"]="";

    //int slice = 1;

//    QString t = dir;
    QFile file(dir);//t.replace(".csv", QString("_%1.csv").arg(slice++, 4, 10, QChar('0'))));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return ;

    QTextStream resFile(&file);
    auto l = --values.end();
    for (auto c: values )      resFile << c.first << (c.first == l->first ? "" : ",");
    resFile << Qt::endl;

    for (auto xp: s)
    {

        // Empty all the configs
        for (auto c: values )  c.second=QString("0");

        values["Metadata_Plate"] = xp->name();
        // Set the basepath for all files:
        QString path = xp->fileName().split(":").last();
        QStringList p = path.split('/'); p.pop_back();
        path = p.join('\\');
        QStringList cname = xp->getChannelNames();

        for (auto seq: xp->getValidSequenceFiles())
        {
            QStringList t = seq->getTags();

            values["Metadata_Tags"]= t.join(";").replace("%","");

            for (auto a: cname)  values[QString("Image_PathName_%1").arg(a.trimmed().replace(" ", "_"))]=path;
            for (auto c : meta) values[QString("Metadata_%1").arg(c.trimmed().replace(" ", "_"))] = QString("0");
            for (auto c : titration) values[QString("Titration_%1").arg(c.trimmed().replace(" ", "_"))] = QString("0");

            int validated_tags = 0;

            for (auto c : t)
            {


                for (auto mt : tag_filter)
                    validated_tags += c.contains(mt);

                if (c.contains('#'))
                {
                    QStringList sc =  c.split('#');
                    if (!sc.isEmpty())
                        values[QString("Titration_%1").arg(sc.front().replace(" ", "_"))] = sc.back().replace("%","");
                }
                else
                    values[QString("Metadata_%1").arg(c.replace(" ", "_"))] = "1";
            }

            if (validated_tags != tag_filter.size()) continue;

            auto wPos = seq->Pos();
            values["Metadata_Well"] = wPos;

            if (!wellMatcher.isEmpty() && !wellMatcher.exactMatch(wPos))
                continue;


            for (unsigned int t = 0; t < seq->getTimePointCount(); ++t)
                for (unsigned f = 0; f < seq->getFieldCount(); ++f)
                {
                    if (!siteMatcher.isEmpty() && !siteMatcher.exactMatch(QString("%1").arg(f)))
                        continue;

                    for (unsigned z = 0; z < seq->getZCount(); ++z)
                    {
                        for (unsigned c = 0; c < seq->getChannels(); ++c)
                        {
                            QString fi = seq->getFile(t+1,f+1, z+1, c+1);
                            values["Metadata_Site"]=QString("%1").arg(f);
                            values["Metadata_Time"]=QString("%1").arg(t);
                            values["Metadata_Z"]=QString("%1").arg(z);
                            QString fname = fi.split('/').back();

                            if (fname.isEmpty())
                                continue;

                            values[QString("Image_FileName_%1").arg(cname[c].trimmed().replace(" ", "_"))]=fname;
                        }

                        for (auto & c: values )      resFile << c.second << (c.first == l->first ? "" : ",");
                        resFile << Qt::endl;
                    }
                }
        }
    }

}

bool MainWindow::close()
{
    //  CheckoutProcess::handler().exitServer();

    // Save state

    auto imf = ui->tabWidget->findChildren<ImageForm*>();

    QSet<ImageForm*> inters;

    for (auto frm : imf)
        inters.insert(frm);

    QDir dir( QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());

    QFile cbfile(dir.path() + "/context.cbor");
    if (!cbfile.open(QIODevice::ReadWrite)) {
        qWarning("Couldn't open save file.");
        return false;
    }
    QByteArray saveData = cbfile.readAll();

    QJsonObject meta = QCborValue::fromCbor(saveData).toMap().toJsonObject();

    for (auto frm: inters)
    {
        auto xp = frm->getInteractor()->getExperimentName();
        QJsonArray mima;
        for (unsigned i = 0; i < frm->getInteractor()->getChannels(); ++i)
        {
            auto ifo = frm->getInteractor()->getChannelImageInfos(i+1);
            QJsonObject ob;
            ob.insert("min", ifo->getDispMin());
            ob.insert("max", ifo->getDispMax());
            //            qDebug() << xp << i << ifo->getDispMin() << ifo->getDispMax();
            mima.insert(i, ob);
        }
        meta.insert(xp, mima);
    }

    cbfile.seek(0); // rewind file
    cbfile.write(QCborValue::fromJsonValue(meta).toCbor());

    return QMainWindow::close();
}



void MainWindow::on_action_Exit_triggered()
{
    emit close();
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    //    qDebug() << "Close Event !";

    bool res = close();
    if (res)
        ev->accept();
    else
        ev->ignore();
}


void MainWindow::on_treeView_customContextMenuRequested(const QPoint &pos)
{

    QModelIndex idx = ui->treeView->indexAt(pos);

    if (ui->treeView->rootIndex() == idx.parent())
    {
        QMenu menu(this);

        auto *ico = menu.addMenu("Set Icon");
        menu.addAction("Load plate", this, SLOT(loadPlate()));
        auto mm = menu.addMenu("Load && Display");
        mm->addAction("first well", this, SLOT(loadPlateFirst()));
        mm->addAction("3 wells", this, SLOT(loadPlateDisplay3()));
        menu.addSeparator();
        menu.addAction("export for CP", this, SLOT(exportToCellProfiler()));
        menu.addSeparator();
        menu.addAction("add Directory", this, SLOT(addDirectory()));
        menu.addAction("remove Directory", this, SLOT(rmDirectory()));
        menu.addSeparator();
        menu.addAction("clear all Directories", this, SLOT(clearDirectories()));

        for (int i = 0; i < 21; ++i)
        {
            QIcon cmic(QString(":/MicC%1.png").arg(i));
            QAction* nmic = new QAction(cmic, tr("&Set Icon"), this);
            connect(nmic, SIGNAL(triggered()), this, SLOT(setDataIcon()));
            ico->addAction(nmic);
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

void MainWindow::on_sync_fields_toggled(bool arg1)
{
    QSettings set;

    set.setValue("SyncFields", arg1);
}

void MainWindow::on_sync_zstack_toggled(bool arg1)
{
    QSettings set;

    set.setValue("SyncZstack", arg1);
}



void MainWindow::on_start_process_triggered()
{
    // Ctrl + Enter
    startProcess();
}







