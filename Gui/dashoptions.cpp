#include "dashoptions.h"
#include "ui_dashoptions.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <Core/wellplatemodel.h>
#include <QFileInfo>
#include <QDateTime>

#include <QSettings>
#include <QDir>


DashOptions::DashOptions(Screens screens, bool notebook, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DashOptions),
    _screens(screens),
    _notebook(notebook)
{
    ui->setupUi(this);

    notebookAdapt();
    populateDataset();


}

DashOptions::~DashOptions()
{
    delete ui;
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
    QDir iter(notebooksPaths);

    QStringList files = iter.entryList(QStringList() << "*.ipynb", QDir::Files | QDir::NoSymLinks);
    QStringList subd = iter.entryList(QDir::Dirs | QDir::NoSymLinks);
    // recurse


    // project name & plugin name as well !




    // Need to add a Pick Processing panel

    ui->toolBox->insertItem(1, new QWidget(), "Select Post Processing");




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
        QStringList csv_files;

        for (int ch = 0; ch < ui->datasets->topLevelItemCount() ;++ch)
        {
            auto plate = ui->datasets->topLevelItem(ch);

            for (int t = 0; t < plate->childCount(); ++t)
            {
                auto ag_rw = plate->child(t);

                for (int dsi = 0; dsi < ag_rw->childCount(); ++dsi)
                {
                    auto ds = ag_rw->child(dsi);

                    if (ds->checkState(0) == Qt::Checked)
                    {
                        qDebug() << ds->text(0) << ds->data(0, Qt::UserRole);
                        csv_files << ds->data(0, Qt::UserRole).toString();
                    }
                }

            }
        // Load or unload files


        }



    }

}
