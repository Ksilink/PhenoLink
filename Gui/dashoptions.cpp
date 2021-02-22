#include "dashoptions.h"
#include "ui_dashoptions.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <Core/wellplatemodel.h>
#include <QFileInfo>
#include <QDateTime>

#include <QSettings>
#include <QDir>
#include <QComboBox>


DashOptions::DashOptions(Screens screens, bool notebook, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DashOptions),
    qtw(nullptr),
    _screens(screens),
    _notebook(notebook)
{
    ui->setupUi(this);
//    for (int i = ui->toolBox->count()-1; i > 1 ; --i)
//    {
//        ui->toolBox->removeItem(i);
//    }


    populateDataset();
    notebookAdapt();


}

DashOptions::~DashOptions()
{
    delete ui;
}

typedef enum
{
    Unset,
    Raw,
    Aggregated
} eRecState;

QPair<QStringList, QStringList> recurseTree(QTreeWidgetItem* item, eRecState state = Unset)
{
    QPair<QStringList, QStringList> res;

    for (int i = 0; i < item->childCount(); ++i)
    {
        QTreeWidgetItem *ch = item->child(i);
        qDebug() << ch->text(0) << ch->checkState(0) << state;
        if (state==eRecState::Unset)
        {
            if (ch->text(0)=="Raw")
                state = eRecState::Raw;
            else if (ch->text(0)=="Aggregated")
                state = eRecState::Aggregated;
        }
        else
        {
            if (ch->checkState(0)==Qt::Checked)
            {
                QVariant v = ch->data(0, Qt::UserRole);
                if (v.isValid())
                {
                    QString file = "'" + v.toString() + "'";
                    if (state==eRecState::Raw)
                        res.first += file;
                    else
                        res.second += file;
                    continue;
                }
            }
        }
        QPair<QStringList, QStringList> t = recurseTree(ch, state);
        res.first += t.first;
        res.second += t.second;
    }
    return res;
}

QPair<QStringList, QStringList> DashOptions::getDatasets()
{
    // Iterates through the dataset to get the checked data
    QPair<QStringList, QStringList> res;


    for (int i = 0; i < ui->datasets->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* it = ui->datasets->topLevelItem(i);
        QPair<QStringList, QStringList> t= recurseTree(it);
        res.first += t.first;
        res.second += t.second;
    }
    return res;
}

QString DashOptions::getProcessing()
{
    if (qtw)
        return qtw->currentData().toString();

    return QString();
}

void DashOptions::populateDataset()
{

    ui->datasets->setColumnCount(3);
    ui->datasets->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QList<QTreeWidgetItem *> items;

    QSet<QString> projs;

    for (auto it : _screens)
    {
        projs.insert(it->property("project"));
        auto dbs = it->databases();

        auto parent = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList() << it->name() << "" << "");
        items << parent;
        auto rw = new QTreeWidgetItem(parent, QStringList()<<"Raw"<<"" <<"");
        auto ag = new QTreeWidgetItem(parent, QStringList()<<"Aggregated"<<"" <<"");
        //      qDebug() << dbs.first << dbs.second;
        for (auto l : dbs.first)
        {
            QStringList split = l.split("/");
            QString t = split[split.count()-2];
            QFileInfo info(l);

            QStringList opts;
            opts  << t << (info.birthTime().toString("yyyyMMdd hh:mm:ss")) << (QString("%1 kB").arg(info.size()/1024));
            auto ti = new QTreeWidgetItem(rw, opts);
            ti->setData(0, Qt::UserRole, l);
            //             ti->setData(0, Qt::UserRole+1, QVariant::fromValue(it));
            ti->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            ti->setCheckState(0, Qt::Unchecked);
            //          ti->setCheckState(0, Qt::CheckState);
        }
        for (auto l : dbs.second)
        {
            QStringList split = l.split("/");
            QString t = split[split.count()-2];
            QFileInfo info(l);

            QStringList opts;
            opts  << t << (info.birthTime().toString("yyyyMMdd hh:mm:ss")) << (QString("%1 kB").arg(info.size()/1024));
            auto ti = new QTreeWidgetItem(ag, opts);

            ti->setData(0, Qt::UserRole, l);
            //            ti->setData(0, Qt::UserRole+1, QVariant::fromValue(it));

            ti->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            ti->setCheckState(0, Qt::Unchecked);

        }

        //      QStringList ag = dbs.second;
        //      for (int i = 0; i < 10; ++i)
        //         items.append(new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList(QString("item: %1").arg(i))));

        //      ;
    }

    ui->datasets->insertTopLevelItems(0, items);
    this->projects = QStringList(projs.begin(), projs.end());

}

void DashOptions::notebookAdapt()
{ // Adapt the Gui in notebook call mode

    if (!_notebook) return;

    QSettings set;

    QString db=set.value("databaseDir", "").toString();

    if (db.isEmpty())
    {
        //FIXME: !!!!!
        qDebug() << "Gently tell the user to set the database dir in the options !";
    }

    // Let's seek for ipnb files lying along side the

    // Notebook shall be here:
    QString notebooksPaths = db +  "/Code/KsilinkNotebooks/";

    // now let's got for


    // project name & plugin name  !

    QStringList nbs = recurseNotebooks(notebooksPaths, projects );
//    print(nbs);
    //qDebug() << nbs;
    // Need to add a Pick Processing panel

    qtw = new QComboBox();

    for (auto item : nbs)
    {
        item = item.replace("//", "/").replace(notebooksPaths, "");
        qtw->addItem(item.replace(".ipynb",""), item);
    }


    ui->toolBox->insertItem(1, qtw, "Select Post Processing");
}

QStringList DashOptions::recurseNotebooks(QString path, QStringList projects)
{

    QDir iter(path);

    QStringList files = iter.entryList(QStringList() << "*.ipynb", QDir::Files | QDir::NoSymLinks );
    // recurse

    if (projects.isEmpty())
    { // simply recurse all subdirs
        for (int i = 0; i < files.size(); ++i)  files[i] = path + "/"  + files[i];

        QStringList subd = iter.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        for (auto p: subd)
        {
            QStringList tmps = recurseNotebooks(path + "/" + p);
            //for (int i = 0; i < tmps.size(); ++i)  tmps[i] = path + "/" + tmps[i];
            files += tmps;
        }
    }
    else
    {
        for (auto p: projects)
        {
            QStringList tmps = recurseNotebooks(path + "/" + p);
            //for (int i = 0; i < tmps.size(); ++i)  tmps[i] = path + "/" + tmps[i];
            files += tmps;
        }
    }


    return files;
}

void DashOptions::on_pushButton_2_clicked()
{
    this->close();
}

void DashOptions::on_next_clicked()
{
    if (ui->toolBox->currentIndex() == ui->toolBox->count()-1)
        // Finished continue
        accept();
    else
    {
        ui->toolBox->setCurrentIndex(ui->toolBox->currentIndex()+1);
        //        if (ui->toolBox->currentIndex() == ui->toolBox->count()-2)
    }
}

void DashOptions::on_toolBox_currentChanged(int index)
{
    if (index != 0)
    { // Check for data to be loaded ?
//        QStringList csv_files;

//        for (int ch = 0; ch < ui->datasets->topLevelItemCount() ;++ch)
//        {
//            auto plate = ui->datasets->topLevelItem(ch);

//            for (int t = 0; t < plate->childCount(); ++t)
//            {
//                auto ag_rw = plate->child(t);

//                for (int dsi = 0; dsi < ag_rw->childCount(); ++dsi)
//                {
//                    auto ds = ag_rw->child(dsi);

//                    if (ds->checkState(0) == Qt::Checked)
//                    {
//                        qDebug() << ds->text(0) << ds->data(0, Qt::UserRole);
//                        csv_files << ds->data(0, Qt::UserRole).toString();
//                    }
//                }

//            }
//        // Load or unload files

//        }



    }

}
