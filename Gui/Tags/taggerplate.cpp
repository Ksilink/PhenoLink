#include "taggerplate.h"
#include "ui_taggerplate.h"
#include "tagger.h"


TaggerPlate::TaggerPlate(QString _plate,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TaggerPlate),
    plate(_plate)
{
    ui->setupUi(this);

    qDebug() << (qobject_cast<tagger*>(parent) != nullptr);

}

TaggerPlate::~TaggerPlate()
{
    delete ui;
}

void TaggerPlate::on_setTags_clicked()
{

}


void TaggerPlate::on_unsetTags_clicked()
{

}


// Plate Layout changed (384 / 96 / 1536)
void TaggerPlate::on_plates_design_currentIndexChanged(const QString &arg1)
{
}


// Find Template
void TaggerPlate::on_pushButton_clicked()
{

}

// Export Template
void TaggerPlate::on_pushButton_2_clicked()
{

}


void TaggerPlate::on_treeWidget_customContextMenuRequested(const QPoint &pos)
{
    // Tag context menu for addind new tags
}

void TaggerPlate::setTags(QMap<QString, QSet<QString> > &data, QSet<QString> &othertags)
{

    ui->treeWidget->clear();
    ui->treeWidget->setColumnCount(2);
    ui->treeWidget->setHeaderLabels(QStringList() << "Label" << "Amount");

    QSet<QString> skip;

    QList<QTreeWidgetItem *> items;
    for (auto & key: data.keys())
    {
        QTreeWidgetItem * item = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList(key));
        items.append(item);
        for (auto& prod: data[key])
        {
            new QTreeWidgetItem(item, QStringList(prod));
            for (const QString& ot: othertags)
            {
                if (ot.startsWith(prod))
                    skip.insert(ot);
            }
        }
    }

    items <<  new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), QStringList("Others"));

    ui->treeWidget->insertTopLevelItems(0, items);
    for (const QString& ot: othertags)
        if (!skip.contains(ot))
        {
            new QTreeWidgetItem(items.last(), QStringList(ot));
        }
}

