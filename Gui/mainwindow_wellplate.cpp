#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "screensmodel.h"

#ifndef _WIN64
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
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
#include <QtSql>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>

#include <Core/checkoutprocess.h>
#include <Core/imageinfos.h>

#include <ImageDisplay/imageform.h>

#include <ImageDisplay/scrollzone.h>

#include "ScreensDisplay/screensgraphicsview.h"
#include <QtWebView/QtWebView>
#include <QInputDialog>


#include <QtWebEngineWidgets/QWebEngineView>
//#include <QtWebEngineWidgets/QtWebEngineWidgets>

#include <dashoptions.h>

void MainWindow::on_loadSelection_clicked()
{
    // Retrieve the checked data
    QStringList checked = mdl->getCheckedDirectories(false);

    loadSelection(checked);
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

#ifdef WIN32
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



    //    int tab = ui->tabWidget->addTab(view, "Dash View");
    QWebEngineView *view = new QWebEngineView(this);
    QUrl url(QString("http://%1:%2/notebooks/Run/%5?token=%3&autorun=true")
             .arg(set.value("JupyterNotebook", "127.0.0.1").toString())
             .arg("8888")
             .arg(set.value("JupyterToken", "").toString())
//             .arg(dbopts)
             .arg(tgt)
             );
    qDebug() << url;
    QApplication::clipboard()->setText(url.toString());
    view->load(url);
    int tab = ui->tabWidget->addTab(view, "Notebook View");

        Q_UNUSED(tab);
}

void MainWindow::on_dashDisplay_clicked()
{
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
    QSettings set;
    QWebEngineView *view = new QWebEngineView(this);
    QUrl url(QString("http://%1:%2?%3")
             .arg(set.value("DashServer", "127.0.0.1").toString())
             .arg("8050")
             .arg(dbopts)
             );
    qDebug() << url;
    QApplication::clipboard()->setText(url.toString());
    view->load(url);
    int tab = ui->tabWidget->addTab(view, "Dash View");

        Q_UNUSED(tab);
}

#include <QProgressDialog>

QMutex mx;

void proc_mapped(QPair<SequenceFileModel*, QString>& pairs)
{

#ifdef WIN32

    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(pairs.second.toStdWString().c_str()))
    {
        mx.lock();
        pairs.first->setInvalid();
        mx.unlock();
    }
#else
    

    struct stat buf;
    int exist = stat(pairs.second.toStdString().c_str(), &buf);
    if (exist != 0)
    {
        mx.lock();
        pairs.first->setInvalid();
        mx.unlock();
    }
#endif

}



void MainWindow::loadSelection(QStringList checked)
{
    // We have the screens list
    // Need to properly load the files now
    ScreensHandler& handler = ScreensHandler::getHandler();
    Screens data = handler.loadScreens(checked, true);

    QString err = handler.errorMessage();
    if (!err.isEmpty())
    {
        ui->textLog->append(err);
        err.truncate(80);
        QMessageBox::StandardButton reply;

        reply = QMessageBox::warning(this, "Data Loading",
                                     QString("A problem occured during data loading:  \n'%1...'").arg(err),
                                     QMessageBox::Abort | QMessageBox::Ignore
                                     );
        QSettings set;

        if (!set.value("UserMode/VeryAdvanced", false).toBool() &&  reply == QMessageBox::Abort)
            data.clear();

    }

    //    qDebug() << checked << data.size();

    if (data.size() < 1) return;

    bool multifield = false;

    int loadCount = 0;
    ExperimentFileModel* lastOk = 0;

    QList<QPair<SequenceFileModel*, QString> > proc_mapps;

    foreach (ExperimentFileModel* mdl, data)
    {
        // Check for the property "project"
        if (mdl->property("project").isEmpty())
        {
            bool ok;
            QStringList sug=mdl->fileName().replace("\\", "/").split("/");
            if (sug.size() > 4)
                sug[0] = sug[4];
            else
                sug[0] = sug[sug.size()-2];

            QMessageBox::warning(this, QString("Use Plate Tagger!!!"),
                                 "Make use of plate tagger for setting your plate layout and project name ",
                                 QMessageBox::Ok, QMessageBox::Ok);

            QString text = QInputDialog::getText(this,
                                                 QString("Please specify project Name for: %1 (%2)").arg(mdl->name()).arg(mdl->groupName()),
                                                 "Project Name", QLineEdit::Normal,
                                                 sug.at(0), &ok);
            if (ok && !text.isEmpty())
                mdl->setProperties("project", text);

        }


        for (auto seq : mdl->getAllSequenceFiles())
        {
            for (auto l :seq->getAllFiles())
                proc_mapps.append(qMakePair(seq,l));
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
        multifield |= (mdl->getAllSequenceFiles().front()->getFieldCount() > 1);

        if (mdl)
        {
            createWellPlateViews(mdl);
            mdl->setDisplayed(true);
            mdl->setGroupName(this->mdl->getGroup(mdl->fileName()));
            loadCount++;
            lastOk = mdl;
        }
        else
            QMessageBox::warning(this, "Loading error", "Warning: corrupted data in loading the file, Wellplate structure could not be reconstructed");

        QList<SequenceFileModel *>  l  = mdl->getAllSequenceFiles();
        foreach(SequenceFileModel* mm, l)
        {
            _channelsIds.unite(mm->getChannelsIds());
        }
        // With the _channelsIds we can

        QStringList ch = mdl->getChannelNames();
        if (_channelsNames.size() != ch.size())
        {
            for (int i = 0; i < ch.size(); ++i)
            {
                _channelsNames[i+1] = ch.at(i);
            }
        }
    }

//    if (!lastOk) return;

    statusBar()->showMessage(QString( "Screens loaded: %1").arg(loadCount));


    // Force the display to modify it's information accordingly
    ScreensGraphicsView* view = (ScreensGraphicsView*)ui->wellPlateViewTab->currentWidget();
    view->update();

    if (multifield)
        multifield = (QMessageBox::question(this, "Multi Field Detected", "Do you want to automatically unpack wells on display ?") == QMessageBox::Yes);


    if (multifield)
    {
        foreach (ExperimentFileModel* mdl, data)
        {
            QList<SequenceFileModel *>  l  = mdl->getAllSequenceFiles();
            foreach(SequenceFileModel* mm, l)
                mm->setProperties("unpack", "yes");
            mdl->reloadDatabaseData();
        }
    }
    // Load data using factorised function
    on_wellPlateViewTab_tabBarClicked(ui->wellPlateViewTab->currentIndex());
}



void MainWindow::displayWellSelection()
{
    if (_scrollArea)
        _scrollArea->addSelectedWells();
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
    //    fileDialog.setOption(QFileDialog::ShowDirsOnly);

    //    _f_dlg->setFileMode(QFileDialog::DirectoryOnly);
    //    _f_dlg->setOption(QFileDialog::DontUseNativeDialog, true);

    //    QListView* l = _f_dlg->findChild<QListView*>("listView");

    //    if (l) l->setSelectionMode(QListView::MultiSelection);
    //    QTreeView* t = _f_dlg->findChild<QTreeView*>();
    //    if (t) t->setSelectionMode(QTreeView::MultiSelection);

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

    QtConcurrent::run(mdl, &ScreensModel::addDirectoryTh,  dir);
}


void MainWindow::createWellPlateViews(ExperimentFileModel* data)
{

    if (ui->wellPlateViewTab->count() == 0)
    {
        QPushButton* button = new QPushButton("to wkb");
        connect(button, SIGNAL(clicked()), this, SLOT(addExperimentWorkbench()));
        ui->wellPlateViewTab->setCornerWidget(button);
        button->show();
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

    Screens& data = ScreensHandler::getHandler().getScreens();
    ExperimentFileModel* tmdl = assoc_WellPlateWidget[ui->wellPlateViewTab->widget(tabId)];
    if (!tmdl) return;

    assoc_WellPlateWidget.remove(ui->wellPlateViewTab->widget(tabId));

    data.removeAll(tmdl);

    ui->wellPlateViewTab->removeTab(tabId);


    SequenceViewContainer & container = SequenceViewContainer::getHandler();
    QList<SequenceFileModel *>& lsfm = container.getSequences();
    QList<SequenceFileModel *> erase;
    foreach (SequenceFileModel* sfm, lsfm)
        if (sfm->getOwner() == tmdl)
            erase << sfm;

    _scrollArea->removeSequences(erase);

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


