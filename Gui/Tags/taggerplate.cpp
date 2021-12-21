#include "taggerplate.h"
#include "ui_taggerplate.h"

#include <QStandardItemModel>
#include <QStandardItem>
#include <QSortFilterProxyModel>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#include "Tags/tagger.h"
#include <QSettings>
#include <QMenu>
#include <QColorDialog>
#include <QInputDialog>

TaggerPlate::TaggerPlate(QString _plate,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TaggerPlate),
    plate(_plate)
{
    ui->setupUi(this);

    //    tagger* ttags = qobject_cast< tagger* >(parent);
    //    qDebug() << (ttags != nullptr);


    QStringList header;

    for (int i = 0; i < 'Q'; ++i)
        header << QString('A'+i);

    ui->plateMaps->setVerticalHeaderLabels(header);

    ui->plateMaps->resizeColumnsToContents();
    ui->plateMaps->resizeRowsToContents();

}

TaggerPlate::~TaggerPlate()
{
    delete ui;
}

QJsonDocument &TaggerPlate::getTags() { return tagger; }

void TaggerPlate::on_setTags_clicked()
{
    auto model = ui->treeView->model();
    auto idx = ui->treeView->selectionModel()->currentIndex();
        if (idx.isValid())
        {
           QString str = model->data(idx).toString();
                   //                   .data().toString();
            for (auto item: ui->plateMaps->selectedItems())
            {
                QStringList tmp = item->text().split(";");
                QSet<QString> tags(tmp.begin(), tmp.end());
                tags.insert(str);
                tmp = QStringList(tags.begin(), tags.end());  tmp.sort();
                item->setText(tmp.join(";"));
            }
    //            currentItem();
    }



}


void TaggerPlate::on_unsetTags_clicked()
{

    auto model = ui->treeView->model();
    auto idx = ui->treeView->selectionModel()->currentIndex();
        if (idx.isValid())
        {
           QString str = model->data(idx).toString();
                   //                   .data().toString();
            for (auto item: ui->plateMaps->selectedItems())
            {
                QStringList tags = item->text().split(";");
                tags.removeAll(str);
                item->setText(tags.join(";"));
            }
    //            currentItem();
    }

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


void TaggerPlate::on_treeView_customContextMenuRequested(const QPoint &pos)
{
    // Tag context menu for addind new tags
  //  qDebug() << "Tag edition";
    QMenu menu(this);

    auto *tag = menu.addAction("Add Tag");
    auto *cat = menu.addAction("Add Category");
    menu.addSeparator();

    auto res = menu.exec(ui->treeView->mapToGlobal(pos));

    if (res == tag)
    {

    }
    if (res == cat)
    {

        bool ok;
        QString text = QInputDialog::getText(this, tr("New category"),
                                             tr("Category name:"), QLineEdit::Normal,
                                             "", &ok);
        if (ok && !text.isEmpty())
            ui->treeView->model();
//            textLabel->setText(text);


    }


}

void TaggerPlate::setTags(QMap<QString, QSet<QString> > &data, QSet<QString> &othertags)
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->treeView->model());

    if (!model)
    {
        model = new QStandardItemModel(0, 2);
        mdl = new QSortFilterProxyModel(this);
        mdl->setSourceModel(model);
        ui->treeView->setModel(mdl);
        model->setHorizontalHeaderLabels(QStringList() << "Label" << "Amount");
    }

    QStandardItem* root = model->invisibleRootItem();
    root->removeRows(0, root->rowCount());


    QSet<QString> skip;

    //    QList<QTreeWidgetItem *> items;
    for (auto & key: data.keys())
    {
        QStandardItem * item = new QStandardItem(key);
        root->appendRow(item);
        for (auto& prod: data[key])
        {
            item->appendRow(new QStandardItem(prod));
            for (const QString& ot: othertags)
            {
                if (ot.startsWith(prod))
                    skip.insert(ot);
            }
        }
    }

    auto others = new QStandardItem("Others");
    root->appendRow(others);

    for (const QString& ot: othertags)
        if (!skip.contains(ot))
        {
            others->appendRow(new QStandardItem(ot));
        }

    updatePlate();
}

void TaggerPlate::setTag(int r, int c, QString tags)
{

    if (!ui->plateMaps->item(r,c))
        ui->plateMaps->setItem(r,c, new QTableWidgetItem(tags));
    else
    {
        QStringList lst = ui->plateMaps->item(r,c)->text().split(';');
        for (auto &t: tags.split(';')) lst.append(t);

        lst.sort();
        ui->plateMaps->item(r,c)->setText(lst.join(";"));
    }

    ui->plateMaps->resizeColumnsToContents();
    ui->plateMaps->resizeRowsToContents();
}

void TaggerPlate::setColor(int r, int c, QString color)
{
    if (!ui->plateMaps->item(r,c))
        ui->plateMaps->setItem(r,c, new QTableWidgetItem());

    ui->plateMaps->item(r,c)->setBackground(QColor(color));
}





void TaggerPlate::updatePlate()
{
    QStringList tags, gtags;

    if (tagger.isObject())
    {
        QJsonObject json = tagger.object();
        QJsonObject& ob = json;
        if (json.contains("meta"))
        {
            auto meta = json["meta"].toObject();

            if (meta.contains("global_tags"))
            {
                auto ar = meta["global_tags"].toArray();

                for (auto i : (ar)) gtags << i.toString();

                //                mdl->setProperties("global_tags", gtags.join(";"));
            }
            if (meta.contains("cell_lines"))
            {
                auto ar = meta["cell_lines"].toArray();
                QStringList ll;
                for (auto i : (ar)) ll << i.toString();
                //                mdl->setProperties("cell_lines", ll.join(";"));
            }
        }
        if (ob.contains("map"))
        { // Unroll the tags
            auto map = ob["map"].toObject();
            for (auto it = map.begin(), e = map.end(); it != e; ++it)
            {
                QString ctag = it.key();
                ctag = ctag.replace(';', ' ');
                auto wells = it.value().toObject();
                for (auto k = wells.begin(), ee = wells.end(); k != ee; ++k)
                {
                    auto arr = k.value().toArray();
                    int r = k.key().toUtf8().at(0) - 'A';
                    for (int i = 0; i < arr.size(); ++i)
                    {
                        int c = arr[i].toInt();
                        setTag(r,c, ctag);
                    }
                }
            }
        }
        if (ob.contains("color_map"))
        { // Unroll the colors
            auto map = ob["color_map"].toObject();
            for (auto it = map.begin(), e = map.end(); it != e; ++it)
            {
                QString ctag = it.key();
                auto wells = it.value().toObject();
                for (auto k = wells.begin(), ee = wells.end(); k != ee; ++k)
                {
                    auto arr = k.value().toArray();
                    int r = k.key().toUtf8().at(0) - 'A';
                    for (int i = 0; i < arr.size(); ++i)
                    {
                        int c = arr[i].toInt();

                        //qDebug() << "set Color" << ctag << "to" << QString("%1%2").arg(k.key()).arg(c+1,2, 10, QLatin1Char('0'));
                        setColor(r,c, ctag);



                    }
                }
            }
        }

    }

}


void TaggerPlate::on_lineEdit_textChanged(const QString &arg1)
{

    if (arg1.isEmpty())
        ui->treeView->setSortingEnabled(false);
    else
        ui->treeView->setSortingEnabled(true);

    // Adjust the filter of the treeview
    if (mdl)
    {
        mdl->setFilterFixedString(arg1);
        mdl->setRecursiveFilteringEnabled(true);
        mdl->setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

}


void TaggerPlate::on_plateMaps_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    auto *ico = menu.addAction("Set Color");
    auto *cc = menu.addAction("Clear Color");
    menu.addSeparator();
    auto *ct = menu.addAction("Clear Tags");



    auto res = menu.exec(ui->treeView->mapToGlobal(pos));

    if (res == ico)
    {

        QColor col = QColorDialog::getColor(Qt::white, this);
        if (col.isValid())
        {
            for (auto item: ui->plateMaps->selectedItems())
                if (item)
                    item->setBackground(col);
        }
        return;
    }

    if (res == cc)
    {
        for (auto item: ui->plateMaps->selectedItems())
            if (item)
                item->setBackground(QColor(QColor::Invalid));

        return;
    }

    if (res == ct)
    {
        for (auto item: ui->plateMaps->selectedItems())
            if (item)
                item->setText(QString());
        return;
    }
}

