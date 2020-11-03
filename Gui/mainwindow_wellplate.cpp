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

void MainWindow::on_loadSelection_clicked()
{
    // Retrieve the checked data
    QStringList checked = mdl->getCheckedDirectories(false);

    loadSelection(checked);
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
    foreach (ExperimentFileModel* mdl, data)
    {
        multifield |= (mdl->getAllSequenceFiles().front()->getFieldCount() > 1);

        if (mdl)
        {
            //mdl->addToDatabase();

          /*  if (!QSqlDatabase::contains(mdl->hash()))
            {
                QSettings set;
                QDir dir(set.value("databaseDir").toString());
                QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", mdl->hash());
                db.setDatabaseName(dir.absolutePath() + "/" + mdl->hash() + ".db");
                db.open();
            }
*/
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
        QStringList ch =         mdl->getChannelNames();
        if (_channelsNames.size() != ch.size())
        {
            for (int i = 0; i < ch.size(); ++i)
            {
                _channelsNames[i+1] = ch.at(i);
            }
        }
    }

    if (!lastOk) return;

    statusBar()->showMessage(QString( "Screens loaded: %1").arg(loadCount));

    // Load data using factorised function
    on_wellPlateViewTab_tabBarClicked(ui->wellPlateViewTab->currentIndex());

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
        }
    }

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
    _f_dlg->setFileMode(QFileDialog::DirectoryOnly);
    _f_dlg->setOption(QFileDialog::DontUseNativeDialog, true);

    QListView* l = _f_dlg->findChild<QListView*>("listView");

    if (l) l->setSelectionMode(QListView::MultiSelection);
    QTreeView* t = _f_dlg->findChild<QTreeView*>();
    if (t) t->setSelectionMode(QTreeView::MultiSelection);

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


