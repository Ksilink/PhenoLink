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

void MainWindow::on_computeFeatures_currentChanged(int index)
{
    QTabWidget* cf = ui->computeFeatures;
    if (cf->tabText(index) != "Database")
    { // Display the "commit with "prefix" button
        cf->cornerWidget()->show();
    }
    else
    { // hide the commit to database button
        cf->cornerWidget()->hide();
    }
}

void MainWindow::copyDataToClipBoard()
{
    QMap<int, unsigned> cols, rows;

    QTableView* tv = qobject_cast<QTableView*>(ui->computeFeatures->currentWidget());

    if (!tv)
    {
        tv = qobject_cast<QTableView*>(ui->databases->currentWidget());
        if (!tv) return;
    }

    QItemSelectionModel* selections = tv->selectionModel();

    if (!selections->hasSelection()) {
        tv->selectAll();
        selections = tv->selectionModel();
    }

    QModelIndexList list = selections->selectedIndexes();
    foreach (QModelIndex idx, list)
    {
        cols[idx.column()]++;
        rows[idx.row()]++;
    }

    QAbstractItemModel* mdl = tv->model();
    // mdl->headerData(col, Qt::Horizontal).toString()

    //QHeaderView* hv = tv->horizontalHeader();

    QList<int> col  = cols.keys();  std::sort(col.begin(), col.end());
    QList<int> row = rows.keys();  std::sort(row.begin(), row.end());

    QString raw;
    QString str = "<!--StartFragment-->\n<table>\n<tr>\n";

    foreach (int c, col)
    {
        QString d = mdl->headerData(c, Qt::Horizontal).toString();
        raw += d + ",";
        str += QString("<td>%1</td>").arg(d);
    }

    raw+= "\n";
    str += "</tr>\n";

    foreach (int r, row)
    {
        str += "<tr>\n";
        foreach (int c, col)
        {
            QModelIndex idx = mdl->index(r,c);
            if (selections->isSelected(idx))
            {
                QVariant v = mdl->data(idx);
                raw += v.toString() + ",";
                str += QString("<td %1>%2</td>")
                        .arg(v.canConvert(QVariant::Double)  ? "x:num" : "")
                        .arg(v.toString());
            }
            else
            {
                raw += ",";
                str += "<td></td>";
            }
        }
        raw += "\n";
        str += "</tr>\n";
    }
    str += "</table>\n<!--EndFragment-->\n";

    QMimeData* data = new QMimeData;
    data->setHtml(str); // Handle Excel enriched data
    data->setText(raw); // Handle basic Text output

    QClipboard *clip= QApplication::clipboard();
    clip->setMimeData(data);
}


void MainWindow::exportData()
{
    QString dir = QFileDialog::getSaveFileName(this, tr("Save File"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/export.csv", tr("CSV file (excel compatible) (*.csv)"),
                                               0, /*QFileDialog::DontUseNativeDialog
                                               | */QFileDialog::DontUseCustomDirectoryIcons
                                               );
    if (dir.isEmpty()) return;

    QFile data(dir);
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&data);


        QTableView* tv = qobject_cast<QTableView*>(ui->computeFeatures->currentWidget());
        if (!tv)
        {
            tv = qobject_cast<QTableView*>(ui->databases->currentWidget());
            if (!tv) return;
        }

        QAbstractItemModel* mdl = tv->model();

        for (int c = 0; c < mdl->columnCount(); ++c)
        {
            out << mdl->headerData(c, Qt::Horizontal).toString() << ",";
        }
        out << Qt::endl;
        for (int r = 0; r < mdl->rowCount(); ++r)
        {
            for (int c = 0; c < mdl->columnCount(); ++c)
            {
                QModelIndex idx = mdl->index(r,c);
                QVariant v = mdl->data(idx);
                out << v.toString() + ",";
            }
            out << Qt::endl;
        }

    }

}



void MainWindow::datasetCustomMenu(const QPoint & pos)
{
    QMenu menu(this);

    QAction* nmic = menu.addAction(tr("&Copy"), this, SLOT(copyDataToClipBoard()));
    nmic->setShortcut(Qt::CTRL + Qt::Key_C);

    menu.addAction(tr("&Export Data"), this, SLOT(exportData()));

    menu.exec(ui->computeFeatures->currentWidget()->mapToGlobal(pos));
}

QTableView* MainWindow::getDataTableView(ExperimentFileModel* mdl)
{
    QString hash = mdl->hash();
    QTabWidget* tab = ui->computeFeatures;
    for (int i = 0; i < tab->count(); ++i)
        if (tab->tabText(i) == mdl->name())
            return (QTableView*)tab->widget(i);

    QTableView* table = new QTableView;
    table->setSortingEnabled(true);
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(datasetCustomMenu(QPoint)));



    tab->addTab(table, mdl->name());

    //  tab->setContextMenuPolicy(Qt::CustomContextMenu);
    //  connect(tab, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(datasetCustomMenu(QPoint)));
    table->setObjectName(hash);

    if (!tab->cornerWidget())
    {
        QTabWidget* cf = tab;
        QWidget* thewid = new QWidget();
        thewid->setLayout(new QHBoxLayout());

        thewid->layout()->setContentsMargins(0,0,0,0);
        thewid->layout()->setSpacing(1);

        QPushButton* clear = new QPushButton("clear");
        connect(clear, SIGNAL(clicked()), this, SLOT(clearTable()));
        thewid->layout()->addWidget(clear);

        QPushButton* commit = new QPushButton("commit to db");
        connect(commit, SIGNAL(clicked()), this, SLOT(commitTableToDatabase()));

        thewid->layout()->addWidget(commit);
        QLineEdit* line = new QLineEdit();
        line->setPlaceholderText("column name");
        line->setText(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
        thewid->layout()->addWidget(line);
        cf->setCornerWidget(thewid);
    }


    return table;
}

QTableView *MainWindow::getDataTableView(QString hash)
{
    if (hash.isEmpty()) return 0;

    Screens& screens = ScreensHandler::getHandler().getScreens();
    foreach (ExperimentFileModel* mdl, screens)
        if (mdl->hash() == hash)
        {
            return getDataTableView(mdl);
        }

    return 0;
}


// React on wellPlate tab changes
void MainWindow::on_wellPlateViewTab_tabBarClicked(int index)
{
    Screens& data = ScreensHandler::getHandler().getScreens();
    if (data.size() < 1) return;

    ExperimentFileModel* tmdl = 0;
    foreach (ExperimentFileModel* mdl, data)
        if (ui->wellPlateViewTab->tabText(index) == mdl->name())
            tmdl = mdl;
    if (!tmdl) return;


    QList<QWidget*> l;
    for (int i = 0; i < ui->databases->count(); ++i)  l << ui->databases->widget(i);
    ui->databases->clear();
    foreach (QWidget* w, l)    delete w;


    foreach (QString db, tmdl->getDatabaseNames())
    {
        QTableView* tv = new QTableView(ui->databases);
        tv->setSortingEnabled(true);
        tv->setContextMenuPolicy(Qt::CustomContextMenu);

        ui->databases->addTab(tv, db);

        connect(tv, SIGNAL(customContextMenuRequested(QPoint)),
                this, SLOT(datasetCustomMenu(QPoint)));

        QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel();
        proxyModel->setSourceModel(tmdl->databaseDataModel(db));
        tv->setModel(proxyModel);
    }

}


void MainWindow::adaptSelection(QTableView* tw, QItemSelectionModel* sm, QSet<QString>& rs)
{
    Q_UNUSED(rs);

    if (!sm)
    {
        sm = new QItemSelectionModel(tw->model());
        //      tw->setSelectionModel(sm);
    }
    tw->viewport();
    sm->clear();

    //  for (int r = 0; r < _mdl->rowCount(); ++r)
    //    if (rs.contains(_mdl->data(_mdl->index(r,0), Qt::DisplayRole).toString()))
    //      for (int c = 0; c < _mdl->columnCount(); ++c)
    //        if (_mdl->hasIndex(r,c))
    //          sm->select(_mdl->index(r,c), QItemSelectionModel::Select);
}

void MainWindow::selectDataInTables()
{
    //  qDebug() << "Select Data In table";
    return;  /// Functionnality removed due to the latency induced see "20160530 - Notes de synthèse livraison.docx"
}



QString MainWindow::getDataHash(QJsonObject data)
{
    QJsonArray ar = data["Data"].toArray();

    for (int i = 0; i < ar.size(); ++i)
    {
        QJsonObject ob = ar.at(i).toObject();
        if (ob.contains("Meta"))
            return ob["Meta"].toArray().at(0).toObject()["DataHash"].toString();
    }
    return QString();
}


QList<SequenceFileModel*> addResultImages(QJsonObject ob)
{
    QList<SequenceFileModel*> lsfm = ScreensHandler::getHandler().addProcessResultImage(ob);

    return lsfm;
}


QList<SequenceFileModel*> addResultImage(QJsonObject ob)
{
    QList<SequenceFileModel*> lsfm;
    /// FIXME !!!!
    /// Need to properly add the resulting image
    ///
    lsfm << ScreensHandler::getHandler().addProcessResultSingleImage(ob);
    return lsfm;
}


void MainWindow::retrievedPayload(QString hash)
{
    qDebug() << "Retrieving payload data";
    if (!_waitingForImages.contains(hash))
    {
        QMessageBox::warning(this, "Server data", "The server sent unrequested data");
        return;
    }

    QString msg = QString("Received data %1 processing to display").arg(hash);
    //    qDebug() << msg;
    //ui->statusBar->showMessage(msg);

    // This is needed to be pushed inside a QtConcurrent::run function
    QFutureWatcher<QList<SequenceFileModel*> >* watcher = new QFutureWatcher<QList<SequenceFileModel*> >();
    connect(watcher,  SIGNAL(finished()),
            this,     SLOT(future_networkRetrieveImageFinished()));

    QFuture<QList<SequenceFileModel*> > future = QtConcurrent::run(addResultImage, _waitingForImages[hash]);
    watcher->setFuture(future);
    _waitingForImages.remove(hash);
   // msg = QString("Finished with image %1 / remaining %2").arg(hash).arg(CheckoutProcess::handler().numberOfRunningProcess());
  //  ui->statusBar->showMessage(msg);
}


void MainWindow::networkProcessFinished(QJsonObject data)
{
    //QCoreApplication::processEvents();

    QStringList vals = QStringList() << "Hash" << "ElapsedTime" << "ProcessStartId" << "Data";
    foreach (QString v, vals)
        if (!data.contains(v))
            return;

    QString msg = QString("Process Finished %1 in %2 ms, retrieving network data").arg(data["Hash"].toString()).arg(data["ElapsedTime"].toString());
   // ui->statusBar->showMessage(msg);


    if (_StatusProgress)
    {
        _StatusProgress->setValue(_StatusProgress->value()+1);
//        if (CheckoutProcess::handler().numberOfRunningProcess() == 0)
//        {
//            _StatusProgress->hide();
//            _StatusProgress->setValue(0);
//            _StatusProgress->setMaximum(0);
//        }
    }

    //    if (_progress)
    //    {
    //        _progress->setValue(_progress->value()+1);
    //        if (CheckoutProcess::handler().numberOfRunningProcess() == 0)
    //        {
    //            _progress->hide();
    //            _progress->setValue(0);
    //            _progress->setMaximum(0);
    //        }
    //    }

    int procId = data["ProcessStartId"].toInt();
    QString processHash = data["Hash"].toString();

    //qDebug() << data;

    QJsonArray ar = data["Data"].toArray();
    for (int i = 0; i < ar.size(); ++i)
    {
        //        QCoreApplication::processEvents();

        QJsonObject ob = ar.at(i).toObject();

        if (ob["isImage"].toBool() && ob.contains("DataHash") && !ob["DataHash"].toString().isEmpty())
        {
            QString dhash = ob["DataHash"].toString();
            // Store the current object for further usage
            ob["ProcessHash"] = processHash;
            ob["ProcessStartId"] = procId;
            ob["shallDisplay"] = data["shallDisplay"].toBool();


            // Query the server for the data if available
            bool should_delete = ob.contains("isOptional") && !ob["optionalState"].toBool();

            //                                qDebug() << "Query payload" << dhash;
            QList<QString> hashes; hashes << dhash;
            if (!should_delete) _waitingForImages[dhash] = ob;
            if (ob.contains("Payload"))
            {
                QJsonArray ar = ob["Payload"].toArray();
                for (size_t i = 1; i < (size_t)ar.size(); ++i)
                 {

                    QString hh = QString("%1%2").arg(dhash).arg(i);

//                    qDebug() << hh;
                    if (!should_delete)  _waitingForImages[hh] = ob;

                    hashes << hh;
                }
            }

            foreach (QString h, hashes)
                if (should_delete)
                    CheckoutProcess::handler().deletePayload(h);
                else
                    CheckoutProcess::handler().queryPayload(h);

        }
    }


    foreach (ExperimentFileModel* mdl, ScreensHandler::getHandler().getScreens())
        if (ui->wellPlateViewTab->tabText(ui->wellPlateViewTab->currentIndex()) == mdl->name())
        {
            QTableView* v = getDataTableView(mdl);

            QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel();
            proxyModel->setSourceModel(mdl->computedDataModel());

            v->setModel(proxyModel);
        }

    qDebug() << msg;
}

void MainWindow::future_networkRetrieveImageFinished()
{
    QFutureWatcher<QList<SequenceFileModel*> >* watcher = dynamic_cast<QFutureWatcher<QList<SequenceFileModel*> >* >(sender());

    if (!watcher) {
        qDebug() << "Error retreiving watchers finished state";
        return;
    }


    QList<SequenceFileModel*> l = watcher->future().result();
    //    qDebug() << "image retreived" << l.size();

    if (!l.isEmpty())
        networkRetrievedImage(l);

    QSet<ExperimentFileModel*> mdls;
    foreach (SequenceFileModel* ml, l)   {
        if (ml)
        {
            ExperimentFileModel* efm = ml->getOwner();
            if (efm->getOwner()) efm = efm->getOwner();
            mdls.insert(efm);
        }
    }


    watcher->deleteLater();

    foreach (ExperimentFileModel* mdl, mdls)
    {
        QTableView* v = getDataTableView(mdl);

        QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel();
        proxyModel->setSourceModel(mdl->computedDataModel());

        v->setModel(proxyModel);
    }



}

void MainWindow::networkRetrievedImage(QList<SequenceFileModel *> lsfm)
{
    foreach (SequenceFileModel* m, lsfm)
    {
        if (!m) continue;
        if (!m->toDisplay()) continue;


        ScrollZone* zone = qobject_cast<ScrollZone*>(ui->tabWidget->currentWidget());
        if (!zone) zone = _scrollArea;
        if (!m->isAlreadyShowed())
        {
            m->setAsShowed();
            zone->insertImage(m);
        }
        zone->refresh(m);
    }
}

void MainWindow::commitTableToDatabase()
{
    QTableView* wid = qobject_cast<QTableView*>(ui->computeFeatures->currentWidget());
    QString hash = wid->objectName();

    ExperimentFileModel* mdl = ScreensHandler::getHandler().getScreenFromHash(hash);
    if (!mdl) return;

    QTabWidget* tab = ui->computeFeatures;
    QLineEdit* txt = tab->cornerWidget()->findChild<QLineEdit*>();
    if (!txt) {
        qDebug() << "Erroneous search of widget... impossible, should not happen" ;
        return;
    }
    if (txt->text().isEmpty())
    {
        QMessageBox::warning(this, "Commit data to Database", "Please specify a table name to push into the database");
        return;
    }


    ExperimentDataModel* datamdl = mdl->computedDataModel();

    QtConcurrent::run(datamdl, &ExperimentDataModel::commitToDatabase, hash, txt->text());

    // Update Text for next call
    txt->setText(   QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));


    // Refresh
    on_wellPlateViewTab_tabBarClicked(ui->wellPlateViewTab->currentIndex());
}

void MainWindow::clearTable()
{
    QTableView* wid = qobject_cast<QTableView*>(ui->computeFeatures->currentWidget());
    if (!wid) return;
    QString hash = wid->objectName();

    ExperimentFileModel* mdl = ScreensHandler::getHandler().getScreenFromHash(hash);
    if (!mdl) return;

    mdl->computedDataModel()->clearAll();
}

void MainWindow::setProgressBar()
{
//    qDebug() << "Threaded finished";
    CheckoutProcess& handler = CheckoutProcess::handler();

    if (handler.errors() > 0)
    {
        qDebug() << "Network starting errors!" << handler.errors();
       // QMessageBox::warning(this, "Network error", QString("%1 processes where not properly started, check network config").arg(handler.errors()));
		ui->statusBar->showMessage(QString("Network error: %1 processes where not properly started, check network config").arg(handler.errors()));
    }


}



void MainWindow::updateProcessStatusMessage(QJsonArray oa)
{
    QTextStream stream(&_logFile);

    for (int i = 0; i < oa.size(); ++i)
    {
        QJsonObject ob = oa[i].toObject();
        if (ob["State"] == "Running")
        {
            //  stream << ob["Pos"].toString() << ": " <<  rint(100.*ob["Evolution"].toDouble()) << "% ;";
        }
    }
}



void MainWindow::addImageWorkbench()
{
    int count = 1;
    for (int i = 0; i < ui->tabWidget->count(); ++i)
        if (ui->tabWidget->tabText(i).startsWith("Image Workbench")) count++;

    ScrollZone* s = new ScrollZone(ui->tabWidget);

    s->setMainWindow(this);
    //  s->show();
    QString name = QString("Image Workbench %1").arg(count);
    s->setObjectName(name);
    //QPushButton* button = new QPushButton("Yiha");
    ui->tabWidget->addTab(s, name);
    ui->tabWidget->setCurrentWidget(s);
    _scrollArea = s;
    //  ui->tabWidget->addTab(wb, "Workbench")
}

void MainWindow::removeWorkbench(int idx)
{

    //ui->tabWidget->

    ui->tabWidget->removeTab(idx);
}




void MainWindow::on_tabWidget_currentChanged(int index)
{
    QWidget* w = ui->tabWidget->widget(index);

    if (qobject_cast<ScreensGraphicsView*>(w))
    {
        ui->dockImageControl->hide();
        ui->dockExperimentControl->show();
        refreshExperimentControl(ui->dockExperimentControl->widget()->findChildren<QTreeWidget*>().front(),
                                 qobject_cast<ScreensGraphicsView*>(w)->getExperimentModel() );
    }

    if (qobject_cast<ScrollZone*>(w))
    {
        ui->dockImageControl->show();
        ui->dockExperimentControl->hide();
    }
}




void MainWindow::on_wellSelectionChanged()
{
    // Update the well related objects to be highlighted

    int cur = ui->wellPlateViewTab->currentIndex();
    for (int i = 0; i < ui->wellPlateViewTab->count(); ++i)
        if (i != cur)
        {
            ScreensGraphicsView* view = (ScreensGraphicsView*)ui->wellPlateViewTab->widget(i);
            view->getExperimentModel()->clearState(ExperimentFileModel::IsSelected);
            view->update();
        }
    //    qDebug() << "Well Selection Changed";

    // Update the Workbenches selection if required
    ScrollZone* sz = qobject_cast<ScrollZone*>(ui->tabWidget->currentWidget());
    if (sz) sz->updateSelection();

    ScreensGraphicsView* sgv = qobject_cast<ScreensGraphicsView*>(ui->tabWidget->currentWidget());
    if (sgv)
        sgv->update();

    // Update the well plate view
    ScreensGraphicsView* view = (ScreensGraphicsView*)ui->wellPlateViewTab->currentWidget();
    view->update();

    // Update table views
    selectDataInTables();
}

