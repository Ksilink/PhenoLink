#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "screensmodel.h"

#ifndef _WIN64
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <windows.h>
#include <sys/utime.h>
#include <fileapi.h>
#endif


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
#include <QTreeWidget>
#include <QListWidget>
#include <QtConcurrent/QtConcurrent>

#include <ctkWidgets/ctkDoubleRangeSlider.h>
#include <ctkWidgets/ctkCollapsibleGroupBox.h>
#include <ctkWidgets/ctkRangeSlider.h>
#include <ctkWidgets/ctkPopupWidget.h>
#include <ctkWidgets/ctkDoubleSlider.h>
#include <ctkWidgets/ctkCheckableHeaderView.h>
#include <ctkWidgets/ctkColorPickerButton.h>


#include <QCheckBox>

#include <QTableView>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>

#include <QDesktopServices>

#include <Core/checkoutprocess.h>
#include <Core/imageinfos.h>

#include <ImageDisplay/imageform.h>

#include <ImageDisplay/scrollzone.h>

#include "ScreensDisplay/screensgraphicsview.h"
#include <QInputDialog>
// #include <stdio.h>
// #include <stdlib.h>
#include <sys/types.h>

// #include <time.h>

// #if WIN32
//     #include <QtWebEngineWidgets/QtWebEngineWidgets>
//     #include <QWebEngineView>
// #endif

#include <dashoptions.h>

void MainWindow::on_loadSelection_clicked()
{
    // Retrieve the checked data
    QStringList checked = mdl->getCheckedDirectories(false);

    loadSelection(checked, false);
}


QJsonObject loadAndCleanNotebook(QString file, int& cellid)
{

    cellid = -1;

    QFile loadf(file);
    if (!loadf.open(QIODevice::ReadOnly))
    {
        qDebug() << "Warning cannot open file " << file;
        return QJsonObject();
    }



    QJsonDocument doc = QJsonDocument::fromJson((loadf.readAll()));

    QJsonObject ob = doc.object();

    if (ob.contains("cells"))
    {
        QJsonArray ar = ob["cells"].toArray();

        for (int i = 0; i < ar.count(); ++i) // Clear outputs
        {
            QJsonObject ob = ar[i].toObject();

            if (ob.contains("outputs"))
                ob["outputs"]=QJsonArray();
            if (ob.contains("source"))
            {
                QJsonArray ar = ob["source"].toArray();
                if (ar[0] == "# Parameters:\n")
                    cellid = i;
            }
            ar[i] = ob;
        }

        ob["cells"] = ar;
    }
    else
    {
        qDebug() << "Notebook cells not found !" << ob;
    }

    return ob;
}

bool saveJsonIpynb(QJsonObject ob, QString file)
{
    QFile sf(file);
    if (!sf.open(QIODevice::WriteOnly))
    {
        qDebug() << "Notebook not writable !" << file;
        return false;
    }
    sf.write(QJsonDocument(ob).toJson());

    return true;
}

void MainWindow::on_notebookDisplay_clicked()
{

    QSettings set;


    ScreensHandler& handler = ScreensHandler::getHandler();
    QStringList checked = mdl->getCheckedDirectories(false);
    handler.loadScreens(checked, false);
    Screens data = handler.getScreens();
    // First fire the dashoption object with the Screens :)


    DashOptions opt(data, true, this);
    opt.exec();

    QPair<QStringList, QStringList> datasets = opt.getDatasets();
    QString procs = opt.getProcessing();

// Let's copy the ipynb file to some other location for better integration
// =>

#ifdef _WIN64
    QString username = qgetenv("USERNAME");
#else
    QString username = qgetenv("USER");
#endif

    username =   set.value("UserName", username).toString();

    QString db=set.value("databaseDir", "").toString() + "/Code/KsilinkNotebooks/";

    QString ts = QDateTime::currentDateTime().toString("_yyyyMMdd_hhmmss");

    QString tgt = procs; tgt.replace(".ipynb", "_" + username + ts + ".ipynb");
    //QFile::copy(db + procs , db + "/Run/" + tgt);




    qDebug() << "Copy" << db + procs << db + "/Run/" + tgt;

    QStringList its = tgt.split('/'); its.pop_back();
    QDir dir;  dir.mkpath(db+"/Run/"+its.join("/"));

    int params;
    QJsonObject file = loadAndCleanNotebook(db+procs, params);
    if (params >= 0)
    {
        QJsonArray arc = file["cells"].toArray();
        QJsonObject ob = arc[params].toObject();
        QJsonArray ar =  ob["source"].toArray();

        if (!datasets.first.isEmpty())
            ar.append(QString("dbs=[%1]\n").arg(datasets.first.join(",")));
        if (!datasets.second.isEmpty())
            ar.append(QString("agdbs=[%1]\n").arg(datasets.first.join(",")));

        ob["source"] = ar;
        arc[params] = ob;
        file["cells"] = arc;
    }
    else
    {
        qDebug() << "Cell Parameter not found, aborting the display";
        return;
    }

    saveJsonIpynb(file, db + "/Run/" + tgt);

// Need to load first line of each file & accomodate with plate tags
// Also add a other csv input :) (like cellprofiler or other stuffs;
// allow linking with plates & plates tags then :)





// also we can see if we can introduce some "post processing in python..."
#if WIN32


    // //    int tab = ui->tabWidget->addTab(view, "Dash View");
    // QWebEngineView *view = new QWebEngineView(this);
    QUrl url(QString("http://%1:%2/notebooks/Run/%5?token=%3&autorun=true")
                 .arg(set.value("JupyterNotebook", "127.0.0.1").toString())
                 .arg("8888")
                 .arg(set.value("JupyterToken", "").toString())
                 //             .arg(dbopts)
                 .arg(tgt)
             );
    // qDebug() << url;
    QApplication::clipboard()->setText(url.toString());
#include <QDesktopServices>
    if (!QDesktopServices::openUrl(url))
    {
        QMessageBox::warning(this, "Notebook display", "Notebook cannot be forwarded to you web browser");
    }

    // view->load(url);
    // int tab = ui->tabWidget->addTab(view, "Notebook View");

    // Q_UNUSED(tab);
#endif

}

void MainWindow::on_dashDisplay_clicked()
{
#if WIN32
    ScreensHandler& handler = ScreensHandler::getHandler();
    QStringList checked = mdl->getCheckedDirectories(false);
    handler.loadScreens(checked, false);
    Screens data = handler.getScreens();
    // First fire the dashoption object with the Screens :)


    DashOptions opt(data, false, this);
    opt.exec();
    //    opt->raise();
    //       opt->activateWindow();

    QPair<QStringList, QStringList> datasets = opt.getDatasets();

    QString dbopts = QString("dbs=%1").arg(datasets.first.join(";"))
                     + "&" +
                     QString("agdbs=%1").arg(datasets.second.join(";"));

    dbopts= dbopts.replace("'","");




    // Need to load first line of each file & accomodate with plate tags
    // Also add a other csv input :) (like cellprofiler or other stuffs;
    // allow linking with plates & plates tags then :)

    // also we can see if we can introduce some "post processing in python..."



    //    int tab = ui->tabWidget->addTab(view, "Dash View");
    // QSettings set;
    // QWebEngineView *view = new QWebEngineView(this);
    // QUrl url(QString("http://%1:%2?%3")
    //              .arg(set.value("DashServer", "127.0.0.1").toString())
    //              .arg("8050")
    //              .arg(dbopts)
    //          );
    // qDebug() << url;
    // QApplication::clipboard()->setText(url.toString());
    // view->load(url);
    // int tab = ui->tabWidget->addTab(view, "Dash View");

    // Q_UNUSED(tab);
#endif
}

#include <QProgressDialog>

QMutex mx;

void proc_mapped(QPair<SequenceFileModel*, QString>& pairs)
{
#ifdef _WIN64

    QString str = pairs.second;

    if (str.endsWith(".jxl"))
    {
        str.chop(4); str+=".jxl";

    }
    if (str.endsWith(".czi"))
    {

        str.chop(4);

        str.chop(str.size()-str.lastIndexOf("."));
        str += ".czi";
    }


    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(str.toStdWString().c_str()))
    {
        auto str = pairs.second;

        for (int i = str.size(); i > 0; --i) // Assume it's valid if we have a .tar file
        {
            auto t = str.mid(0, i); t += ".tar";
            if (GetFileAttributesW(t.toStdWString().c_str()) != INVALID_FILE_ATTRIBUTES)
                return;
        }


        mx.lock();
        pairs.first->setInvalid();
        mx.unlock();
    }

#else


    struct stat buf;
    int exist = stat(pairs.second.toStdString().c_str(), &buf);
    if (exist != 0)
    {
        auto str = pairs.second;
        for (int i = str.size(); i > 0; --i)
        {
            auto t = str.right(i); t += ".tar";
            if (stat(pairs.second.toStdString().c_str(), &buf) == 0)
                return;
        }

        mx.lock();
        pairs.first->setInvalid();
        mx.unlock();
    }
#endif

}



Screens MainWindow::loadSelection(QStringList checked, bool reload)
{
    QSettings set;

    // We have the screens list
    // Need to properly load the files now
    ScreensHandler& handler = ScreensHandler::getHandler();
    Screens data = handler.loadScreens(checked, reload);

    QString err = handler.errorMessage();
    if (!err.isEmpty())
    {
        ui->textLog->append(err);
        ui->textLog->show();

        err.truncate(5000);
        QMessageBox::StandardButton reply;

        reply = QMessageBox::warning(this, "Data Loading",
                                     QString("A problem occured during data loading:  \n'%1...'").arg(err),
                                     QMessageBox::Ignore
                                     );
        Q_UNUSED(reply);
        //        QSettings set;

        //        if (!set.value("UserMode/VeryAdvanced", false).toBool() &&  reply == QMessageBox::Abort)
        //            data.clear();

    }

    //    qDebug() << checked << data.size();

    if (data.size() < 1) return data;
    bool multifield = false;
    int loadCount = 0;

    QList<QPair<SequenceFileModel*, QString> > proc_mapps;

    foreach (ExperimentFileModel* mdl, data)
    {
        // Check for the property "project"
        if (mdl->property("project").isEmpty())
        {
            bool ok;

            QMessageBox::warning(this, QString("Use Plate Tagger!!!"),
                                 "Make use of plate tagger for setting your plate layout and project name ",
                                 QMessageBox::Ok, QMessageBox::Ok);

            QString text = QInputDialog::getText(this,
                                                 QString("Please specify project Name for: %1 (%2)").arg(mdl->name()).arg(mdl->groupName()),
                                                 "Project Name", QLineEdit::Normal,
                                                 mdl->getProjectName(), &ok);
            if (ok && !text.isEmpty())
                mdl->setProperties("project", text);

        }


        for (auto seq : mdl->getAllSequenceFiles())
        {
            QStringList fileList = seq->getAllFiles();
            if (fileList.size())
            {
                //                QString jxl = fileList.at(0); jxl.chop(4);
                //                if (QFile::exists(jxl+".jxl"))
                //                    seq->toJxl();

                for (auto l : seq->getAllFiles() )
                    proc_mapps.append(qMakePair(seq,l));
            }
        }
    }


    int threads = QThreadPool::globalInstance()->maxThreadCount();
    // this this is IO bounded let's ask for more at once !
    QThreadPool::globalInstance()->setMaxThreadCount(10*threads);

    // Now we can map our model with progressbar :)
    QProgressDialog dialog(this);
    dialog.setLabelText("Checking Files");

    QFutureWatcher<void> futureWatcher;
    QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
    QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
    QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
    QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);

    // Start the computation.
    futureWatcher.setFuture(QtConcurrent::map(proc_mapps, proc_mapped));

    // Display the dialog and start the event loop.
    dialog.exec();

    futureWatcher.waitForFinished();

    QThreadPool::globalInstance()->setMaxThreadCount(threads);



    foreach (ExperimentFileModel* mdl, data)
    {

        unsigned fieldc = 0; for (auto sf : mdl->getAllSequenceFiles()) fieldc = (std::max)(fieldc, sf->getFieldCount());

        multifield |= (fieldc > 1);

        if (mdl)
        {
            createWellPlateViews(mdl);
            mdl->setDisplayed(true);
            mdl->setGroupName(this->mdl->getGroup(mdl->fileName()));
            loadCount++;
        }
        else
        {
            ui->textLog->setPlainText(
                ui->textLog->toPlainText()
                +
                "\r\nWarning: corrupted data in loading the file, Wellplate structure could not be reconstructed");
            //            QMessageBox::warning(this, "Loading error", "Warning: corrupted data in loading the file, Wellplate structure could not be reconstructed");
        }

        QList<SequenceFileModel *>  l  = mdl->getAllSequenceFiles();
        foreach(SequenceFileModel* mm, l)
        {
            _channelsIds.unite(mm->getChannelsIds());
        }
        // With the _channelsIds we can

        QStringList ch = mdl->getChannelNames();
        QList<int> list = _channelsIds.values();
        std::sort(list.begin(), list.end());


        qDebug() << list << ch << _channelsNames;
        _channelsNames.clear();
        foreach(int channels, list)
        {
            _channelsNames[channels] = ch.at(channels-1);
        }
        qDebug() << _channelsNames;
    }

    //    if (!lastOk) return;

    statusBar()->showMessage(QString( "Screens loaded: %1").arg(loadCount));


    // Force the display to modify it's information accordingly
    ScreensGraphicsView* view = (ScreensGraphicsView*)ui->wellPlateViewTab->currentWidget();
    view->update();

    if (!set.value("AlwaysUnpack", false).toBool() || !set.value("AlwaysUnpack", false).toBool())
    {
        if (multifield)
        {
            multifield = (QMessageBox::question(this, "Multi Field Detected", "Do you want to automatically unpack wells on display ?") == QMessageBox::Yes);
        }
        else
        {
            if (set.value("AlwaysUnpack", false).toBool())
                multifield = true;
            if (set.value("NeverUnpack", false).toBool())
                multifield = false;
        }
    }


    foreach (ExperimentFileModel* mdl, data)
    {
        if (multifield)
        {
            QList<SequenceFileModel *>  l  = mdl->getAllSequenceFiles();

            foreach(SequenceFileModel* mm, l)
                mm->setProperties("unpack", "yes");
        }
        // mdl->reloadDatabaseData();
    }
    // Load data using factorised function
    on_wellPlateViewTab_tabBarClicked(ui->wellPlateViewTab->currentIndex());

    return data;
}

Screens MainWindow::findPlate(QString plate, QStringList project, QString drive)
{
    ScreensHandler& handler = ScreensHandler::getHandler();

    QString file = handler.findPlate(plate, project, drive);
    if (file.isEmpty())   return Screens();
    return loadSelection(QStringList() << file, false);
}

void MainWindow::clearScreenSelection()
{
    mdl->clearCheckedDirectories();
}




void MainWindow::displayWellSelection()
{
    if (_scrollArea)
        _scrollArea->addSelectedWells();
}

SequenceInteractor *MainWindow::getInteractor(SequenceFileModel *mdl)
{
    return _scrollArea->getInteractor(mdl);
}


void MainWindow::addImageFromPython(QStringList files)
{
    ExperimentFileModel* efm = new ExperimentFileModel();
    // Forge an XP name!
    QString d = QString("/Data/DispImage%1/files").arg(_forcedImages.size());
    efm->setName(d);
    efm->setFileName(d);

    _forcedImages.push_back(efm);

    SequenceFileModel& sfm = (*efm)(1,1);
    sfm.setOwner(efm);
    int c = 1;
    foreach(QString f, files)
    {
        sfm.addFile(1,1,1, c++, f);
    }

    ScrollZone* zone = qobject_cast<ScrollZone*>(ui->tabWidget->currentWidget());
    if (!zone) zone = _scrollArea;

    zone->insertImage(&sfm);
    zone->refresh(&sfm);
    sfm.setAsShowed();
}


void MainWindow::on_toolButton_clicked()
{
    QMenu menu(this);

    /// FIXME: Need to construct some plugin based solution to add query data
    menu.addAction("&Add data directory", this, SLOT(addDirectory()));
    menu.addAction("Delete All dirs", this, SLOT(clearDirectories()));


    QStandardItem* root = mdl->invisibleRootItem();
    QMenu *del = menu.addMenu(tr("&Delete"));

    for (int i = 0; i < root->rowCount(); ++i)
    {
        QStandardItem* item = root->child(i);

        QAction* dd = del->addAction(QString(tr("Remove '%1'")).arg(item->text()), this, SLOT(deleteDirectoryPath()));
        dd->setData(item->data(Qt::ToolTipRole));
    }

    menu.exec(ui->toolButton->mapToGlobal(ui->toolButton->rect().bottomLeft()));
}


void MainWindow::deleteDirectoryPath()
{
    QAction* s = dynamic_cast<QAction*>(sender());
    if (!s) { qDebug() << "Aie aie no action"; return ; }

    deleteDirPath(s->data().toString());

}

void MainWindow::deleteDirPath(QString dir)
{
    QSettings set;
    QStringList l = set.value("ScreensDirectory", QVariant(QStringList())).toStringList();
    l.removeAll(dir);

    set.setValue("ScreensDirectory", l);

    QStandardItem* root = mdl->invisibleRootItem();

    for (int i = 0; i < root->rowCount(); ++i)
    {
        QStandardItem* item = root->child(i);
        if (dir == item->data(Qt::ToolTipRole))
        {
            root->removeRow(i);
        }
    }
}


void MainWindow::addDirectory()
{
    QSettings set;
    // For multiple file selection we shall not use native dialog to allow multiple value selection

    QFileDialog* _f_dlg = new QFileDialog(this);
    _f_dlg->setDirectory(set.value("LastAddedDir","c:/").toString());
    _f_dlg->setOption(QFileDialog::ShowDirsOnly, true);
    _f_dlg->setFileMode(QFileDialog::Directory);

    int nMode = _f_dlg->exec();
    if (nMode == QDialog::Rejected)
        return;//


    QStringList dirs = _f_dlg->selectedFiles();

    if (dirs.isEmpty()) // Just to skip the case when no directory is selected...
        return;

    foreach(QString d, dirs)
        addDirectoryName(d);

    set.setValue("LastAddedDir", dirs.last());
}

void MainWindow::addDirectoryName(QString dir)
{
    QSettings set;
    QStringList l = set.value("ScreensDirectory", QVariant(QStringList())).toStringList();
    if (!l.contains(dir))
        l << dir;
    set.setValue("ScreensDirectory", l);

    auto fut = QtConcurrent::run(&ScreensModel::addDirectoryTh, mdl,  dir);
}

#include <QHBoxLayout>
void MainWindow::createWellPlateViews(ExperimentFileModel* data)
{

    if (ui->wellPlateViewTab->count() == 0)
    {
        auto w = new QWidget;
        auto hl = new QHBoxLayout();

        w->setLayout(hl);

        QPushButton* button = new QPushButton("bird view");
        connect(button, SIGNAL(clicked()), this, SLOT(createBirdView()));
        hl->addWidget(button);

        button = new QPushButton("to wkb");
        connect(button, SIGNAL(clicked()), this, SLOT(addExperimentWorkbench()));
        hl->addWidget(button);

        hl->setContentsMargins(0, 0, 0, 0);


        ui->wellPlateViewTab->setCornerWidget(w);
        w->show();
    }


    QString name = data->name();
    int occ = 0;
    // This is needed if the screen is not already loaded
    for (int i = 0; i < ui->wellPlateViewTab->count(); ++i) // Do not display multiple time the same tab
        if (ui->wellPlateViewTab->tabText(i) == name)
            name = QString("%1 (%2)").arg(data->name()).arg(++occ);

    ScreensGraphicsView* view = new ScreensGraphicsView();
    if (!view->scene()) view->setScene(new QGraphicsScene());

    assoc_WellPlateWidget[view] = data;

    int idx = ui->wellPlateViewTab->addTab(view, name);

    ui->wellPlateViewTab->setTabToolTip(idx,   data->groupName() + "/" + name);


    GraphicsScreensItem* gfx = new GraphicsScreensItem;
    // QRectF r =
    gfx->constructWellRepresentation(data, view);
    connect(gfx, SIGNAL(selectionChanged()), this, SLOT(on_wellSelectionChanged()));
}

void MainWindow::wellplateClose(int tabId)
{

    //    Screens& data = ScreensHandler::getHandler().getScreens();
    ExperimentFileModel* tmdl = assoc_WellPlateWidget[ui->wellPlateViewTab->widget(tabId)];
    if (!tmdl) return;

    assoc_WellPlateWidget.remove(ui->wellPlateViewTab->widget(tabId));

    ScreensHandler::getHandler().removeScreen(tmdl);

    ui->wellPlateViewTab->removeTab(tabId);


    SequenceViewContainer & container = SequenceViewContainer::getHandler();
    QList<SequenceFileModel *>& lsfm = container.getSequences();
    QList<SequenceFileModel *> erase;

    foreach (SequenceFileModel* sfm, lsfm)
        if (sfm->getOwner() == tmdl)
            erase << sfm;

    _scrollArea->removeSequences(erase);
    delete tmdl;
}

void MainWindow::refreshExperimentControl(QTreeWidget* l,ExperimentFileModel* mdl)
{

    l->clear();
    // Handle the data with a tree view
    // Iterates through the Database table data to Retrieve column names
    QTreeWidgetItem* db = new QTreeWidgetItem(QStringList() << "Database");
    // Iterates through the computed features table to Retrieve column names
    QTreeWidgetItem* dt = new QTreeWidgetItem(QStringList() << "Computed");
    QStringList cols = mdl->getDatabaseNames();

    // If non empty add the column names to the tree
    if (cols.size())
    {
        foreach (QString name, cols)
        {
            //          QTreeWidgetItem* it = new QTreeWidgetItem(db, QStringList() << name);
            foreach (QString nn, mdl->databaseDataModel(name)->getExperiments())
                new QTreeWidgetItem(db, QStringList() << name + "/" + nn);
        }
        l->insertTopLevelItem(0, db);
    }
    else delete db;

    cols.clear();
    cols = mdl->computedDataModel()->getExperiments();

    // Search the corresponding datatable & perform similar insertion
    if (cols.size())
    {
        foreach (QString name, cols)
            new QTreeWidgetItem(dt, QStringList() << name);
        l->insertTopLevelItem(1, dt);
    }
    l->expandItem(dt);

}

QString generatePlate(QFile& file, ExperimentFileModel* mdl)
{


    QString res;

    QSettings set;
    QString dbP=set.value("databaseDir").toString();

    bool hasBirdview2 = false;
    for (unsigned r = 0; r < mdl->getRowCount() && !hasBirdview2; ++r)
    {
        for (unsigned c = 0; c < mdl->getColCount(); ++c)
        {
            auto colname =  (QString("%1").arg(((int)c+1), 2, 10,QChar('0')));
            if (mdl->hasMeasurements(QPoint(r, c)))
            {

                QString img2Path = dbP + "/PROJECTS/" + mdl->getProjectName() + "/Checkout_Results/BirdView/" + mdl->name() + "/bv2" + mdl->name() + "_" + QString('A'+QChar(r))
                                   + colname + ".jpg";
                if (QFile::exists(img2Path))
                    hasBirdview2 = true;
                break;
            }
        }
    }



    // Let's create the HTML file !
    if (file.open(QFile::WriteOnly))
    {


        QTextStream out(&file);

        QString chanChange("<select name='channel' onchange='chanChange(this.value);'>");

        if (hasBirdview2)
        {
            chanChange+="<option value='bv2composite' >Adjusted Composite</option>";
            for (auto chan : mdl->getChannelNames())
                chanChange += QString("<option value='bv2%1'>Adjusted %1</option>").arg(chan.replace(" ", "_"));

        }
        chanChange+="<option value='composite' >Composite</option>";
        for (auto chan : mdl->getChannelNames())
            chanChange += QString("<option value='%1'>%1</option>").arg(chan.replace(" ", "_"));
        chanChange += "</select>";


        out << "<html>"
            << "<head>"
            <<"<link rel='stylesheet' href='" << dbP << "/Code/HTML/birdview.css'>"
            <<"<script src='"<< dbP << "/Code/HTML/jquery.js'></script>"
            <<"<script src='"<< dbP << "/Code/HTML/ksilink.js'></script>"
            << "</head>"
            << "<body><h1>" << mdl->name() << " " << chanChange << "<label class='switch'><input type='checkbox' id='togBtn' onchange='toBW();'><div class='slider round' id='bw'></div></label></h1>"
            << "<div id='largeImg' class='Center' hidden=True></div>"
            <<"<table width='100%' >";

        out  << "<thead>" << "<tr><th></th>"; // Empty col for row name

        //<<    "<th colspan='2'>The table header</th>"
        // Table header
        for (unsigned  i = 0; i < mdl->getColCount(); ++i) out << "<th>" <<QString("%1").arg(i+1,2) << "</th>";
        out <<  "</tr>" <<"</thead>";
        out   <<"<tbody>";
        for (unsigned r = 0; r < mdl->getRowCount(); ++r)
        {
            out << "<tr><td>" << QString((QChar('A'+r))) << "</td>";
            for (unsigned c = 0; c < mdl->getColCount(); ++c)
            {
                auto colname =  (QString("%1").arg(((int)c+1), 2, 10,QChar('0')));
                if (mdl->hasMeasurements(QPoint(r, c)))
                {

                    QString imgPath = dbP + "/PROJECTS/" + mdl->getProjectName() + "/Checkout_Results/BirdView/" + mdl->name() + "/" + mdl->name() + "_" + QString(QChar('A'+r))
                                      + colname + ".jpg";
                    QString img2Path = dbP + "/PROJECTS/" + mdl->getProjectName() + "/Checkout_Results/BirdView/" + mdl->name() + "/bv2" + mdl->name() + "_" + QString(QChar('A'+r))
                                       + colname + ".jpg";


                    out <<    "<td><img width='100%' src='file://"
                        <<  (hasBirdview2 ? img2Path : imgPath) << "' onclick='imgEnlarge(this);' title='"<< (*mdl)(r,c).getTags().join(',') << "' id='" << QString(QChar('A'+r)) << colname << "' checkout='http://localhost:8020/Load?project=" << mdl->getProjectName() << "&plate=" << mdl->name() << "&wells=" << QString(QChar('A'+r)) << colname << "&json'" <<"/></td>";
                    if (res.isEmpty())
                        res = imgPath;
                }
                else
                    out << "<td>-</td>";
            }
            out <<      "</tr>";
        }
        out  <<"</tbody>";
        out         <<"</table>"
            <<"</body>"
            << "</html>";
    }
    return res;
}



void MainWindow::createBirdView()
{
#if WIN32
    // Check if the plate's birdview HTML exists or write it
    QString xpName = ui->wellPlateViewTab->tabText(ui->wellPlateViewTab->currentIndex());
    if (xpName.isEmpty())
    {
        QMessageBox::warning(this, "Warning: adding Workbench", "Experiment Workbenches can only be added after being loaded!");
        return;
    }

    ScreensGraphicsView* owner = qobject_cast<ScreensGraphicsView*>(ui->wellPlateViewTab->currentWidget());
    ExperimentFileModel* mdl = 0x0;
    QList<QGraphicsItem *> items = owner->items();
    foreach (QGraphicsItem * it, items)
    {
        GraphicsScreensItem* gsi = dynamic_cast<GraphicsScreensItem*>(it);
        if (gsi)
        {
            mdl = gsi->currentRepresentation();
        }
    }
    if (!mdl) { qDebug() << "No model found" ; return ; }


    QSettings set;
    QString dbP=set.value("databaseDir").toString();

    QString fpath = QString("%1/PROJECTS/%2/Checkout_Results/BirdView/birdview_%3.html").arg(dbP,mdl->getProjectName(), mdl->name());

    QFile file(fpath);
    QString imgName = generatePlate(file, mdl);
    file.close();

    // // Launch the HTML viewer on the birdview file
    // QWebEngineView *view = new QWebEngineView(this);
    QUrl url(fpath);

    if (!QDesktopServices::openUrl(url))
    {

        QMessageBox::warning(this, "Warning: Failed to load Birdview", QString("File %1 was not found").arg(fpath));
            return;
    }

    // view->load(url);
    // ui->tabWidget->addTab(view, "Birdview");


#endif
}


void MainWindow::addExperimentWorkbench()
{
    // First get the current workbench from tab

    QString xpName = ui->wellPlateViewTab->tabText(ui->wellPlateViewTab->currentIndex());
    if (xpName.isEmpty())
    {
        QMessageBox::warning(this, "Warning: adding Workbench", "Experiment Workbenches can only be added after being loaded!");
        return;
    }
    ScreensGraphicsView* owner = qobject_cast<ScreensGraphicsView*>(ui->wellPlateViewTab->currentWidget());

    int count = 1;
    for (int i = 0; i < ui->tabWidget->count(); ++i)
        if (ui->tabWidget->tabText(i).startsWith(xpName)) count++;

    xpName = QString("%1 %2").arg(xpName).arg(count);

    ScreensGraphicsView* view = new ScreensGraphicsView();
    if (!view->scene()) view->setScene(new QGraphicsScene());

    int tab = ui->tabWidget->addTab(view, xpName);


    ExperimentFileModel* mdl = 0x0;
    QList<QGraphicsItem *> items = owner->items();
    foreach (QGraphicsItem * it, items)
    {
        GraphicsScreensItem* gsi = dynamic_cast<GraphicsScreensItem*>(it);
        if (gsi)
        {
            mdl = gsi->currentRepresentation();
        }
    }
    if (!mdl) { qDebug() << "No model found" ; return ; }
    GraphicsScreensItem* gfx = new GraphicsScreensItem;
    connect(gfx, SIGNAL(selectionChanged()), this, SLOT(on_wellSelectionChanged()));

    gfx->constructWellRepresentation(mdl, view);

    // Construct the GUI for the
    ExperimentWorkbenchControl* wid = new ExperimentWorkbenchControl(mdl, gfx, ui->dockExperimentControl);

    ui->dockExperimentControl->setWidget(wid);

    // Connect the interactions
    ui->tabWidget->setCurrentIndex(tab);//Widget(view);

    ui->tabWidget->setTabToolTip(tab, mdl->groupName());
}


