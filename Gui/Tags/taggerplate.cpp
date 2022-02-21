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
#include <QPainter>

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
    QSortFilterProxyModel* ml = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    if (!ml) return;

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ml->sourceModel());
    if (!model) return;

    QStandardItem* root = model->invisibleRootItem();

    auto idx = ml->mapToSource(ui->treeView->selectionModel()->currentIndex());
    qDebug() << idx;// << idx.parent();
    if (idx.isValid() && idx.parent().isValid())
    {
        //        qDebug() << idx << idx.parent();
        //        qDebug() << root;
        QStandardItem* item = root->child(idx.parent().row());
        //        qDebug() << item;
        if (item)
        {
            QString str = item->child(idx.row())->text();
            qDebug() << item->text() << str << ui->plateMaps->selectedItems().size();

            for (auto idx: ui->plateMaps->selectionModel()->selectedIndexes())
            {
                QTableWidgetItem * item = ui->plateMaps->item(idx.row(), idx.column());
                if (!item)
                {
                    item = new QTableWidgetItem(str);
                    ui->plateMaps->setItem(idx.row(), idx.column(), item);

                }
                else
                {
                    QStringList tmp = item->text().split(";", Qt::SkipEmptyParts);
                    QSet<QString> tags(tmp.begin(), tmp.end());
                    tags.insert(str);
                    tmp = QStringList(tags.begin(), tags.end());  tmp.sort(); tmp.removeAll("");
                    item->setText(tmp.join(";"));
                }
            }
        }
        //            currentItem();
    }
}


void TaggerPlate::on_unsetTags_clicked()
{
    QSortFilterProxyModel* ml = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    if (!ml) return;

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ml->sourceModel());
    if (!model) return;

    QStandardItem* root = model->invisibleRootItem();

    auto idx = ml->mapToSource(ui->treeView->selectionModel()->currentIndex());
    if (idx.isValid() && idx.parent().isValid())
    {
        QStandardItem* item = root->child(idx.parent().row());
        QString str = item->child(idx.row())->text();

        //                   .data().toString();
        for (auto item: ui->plateMaps->selectedItems())
        {
            QStringList tags = item->text().split(";", Qt::SkipEmptyParts);
            tags.removeAll(str);
            item->setText(tags.join(";"));
        }
        //            currentItem();
    }
}


// Plate Layout changed (384 / 96 / 1536)
void TaggerPlate::on_plates_design_currentIndexChanged(const QString &)
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

    auto idx = ui->treeView->indexAt(pos);

    QSortFilterProxyModel* ml = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    if (!ml) return;

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ml->sourceModel());
    if (!model) return;


    QStandardItem* root = model->invisibleRootItem();
    //QStandardItem* root = ui->treeView->invisibleRootItem();
    //ui->treeView->roo

    if (res == tag)
    {

        bool ok;
        QString text = QInputDialog::getText(this, tr("New Tag"),
                                             tr("Tag name:"), QLineEdit::Normal,
                                             "", &ok);
        if (ok && !text.isEmpty())
        {
            QStandardItem* parent = nullptr;
            QString name =  ui->treeView->model()->itemData(idx)[Qt::DisplayRole].toString();

            for (int i = 0; i < root->rowCount(); ++i)
            {
                if (root->child(i)->text() == name)
                    parent = root->child(i);
            }

            if (parent)
                parent->appendRow(new QStandardItem(text));
        }
    }
    if (res == cat)
    {

        bool ok;
        QString text = QInputDialog::getText(this, tr("New category"),
                                             tr("Category name:"), QLineEdit::Normal,
                                             "", &ok);
        if (ok && !text.isEmpty())
            root->appendRow(new QStandardItem(text));


    }
}

void TaggerPlate::setTags(QMap<QString, QMap<QString, QSet<QString > > > &data,
                          QMap<QString, QSet<QString> >  &othertags,
                          QString project)
{
    QSortFilterProxyModel* ml = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    QStandardItemModel* model = nullptr;

    if (!ml)
    {
        model = new QStandardItemModel(0, 2);
        mdl = new QSortFilterProxyModel(this);

        mdl->setRecursiveFilteringEnabled(true);
        mdl->setFilterCaseSensitivity(Qt::CaseInsensitive);
        //        mdl->setSortRole():

        mdl->setSourceModel(model);
        ui->treeView->setModel(mdl);
        model->setHorizontalHeaderLabels(QStringList() << "Label" << "Amount");
    }
    else
        model = qobject_cast<QStandardItemModel*>(ml->sourceModel());


    QStandardItem* root = model->invisibleRootItem();
    root->removeRows(0, root->rowCount());


    QSet<QString> skip;
    QStringList headers = QStringList() << "" << project;
    for (auto& k : headers)
        for (auto & key: data[k].keys())
        {
            QStandardItem * item = new QStandardItem(key);
            root->appendRow(item);
            for (auto& prod: data[k][key])
            {
                item->appendRow(new QStandardItem(prod));
                for (const QString& ot: othertags[project])
                {
                    if (ot.startsWith(prod))
                        skip.insert(ot);
                }
            }
        }

    auto others = new QStandardItem("Others");
    root->appendRow(others);

    for (const QString& ot: othertags[project])
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

        QSet<QString> st = QSet<QString>(lst.begin(), lst.end());

        lst.clear();

        for (auto& t: st) lst << t;

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

void TaggerPlate::setPattern(int r, int c, int patt)
{
    if (!ui->plateMaps->item(r,c))
        ui->plateMaps->setItem(r,c, new QTableWidgetItem());


    auto b = ui->plateMaps->item(r,c)->background();
    b.setStyle((Qt::BrushStyle)patt);
    ui->plateMaps->item(r,c)->setBackground(b);
}

void TaggerPlate::setColorFG(int r, int c, QString color)
{
    if (!ui->plateMaps->item(r,c))
        ui->plateMaps->setItem(r,c, new QTableWidgetItem());

    ui->plateMaps->item(r,c)->setForeground(QColor(color));

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
        if (ob.contains("fgcolor_map"))
        { // Unroll the colors
            auto map = ob["fgcolor_map"].toObject();
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
                        setColorFG(r,c, ctag);
                    }
                }
            }
        }
        if (ob.contains("pattern_map"))
        { // Unroll the patterns
            auto map = ob["pattern_map"].toObject();
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
                        setPattern(r,c, ctag.toInt());
                    }
                }
            }
        }

    }

}

QJsonDocument TaggerPlate::refreshJson()
{
    // We shall update the tags with respect to the content of the GUI
    //




    return tagger;
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
    auto *pco = menu.addMenu("Set Pattern");
    QStringList opts = QStringList() << "SolidPattern" <<
                                        "Dense1Pattern" <<
                                        "Dense2Pattern" <<
                                        "Dense3Pattern" <<
                                        "Dense4Pattern" <<
                                        "Dense5Pattern" <<
                                        "Dense6Pattern" <<
                                        "Dense7Pattern" <<
                                        "HorPattern" <<
                                        "VerPattern" <<
                                        "CrossPattern" <<
                                        "BDiagPattern" <<
                                        "FDiagPattern" <<
                                        "DiagCrossPattern" <<
                                        "LinearGradientPattern"
                                     << "RadialGradientPattern"
                                     << "ConicalGradientPattern";


    QList<QAction*> br;
    for (int i = Qt::SolidPattern; i < Qt::LinearGradientPattern; ++i)
    {
        QPixmap px(16,16);
        QPainter p(&px);
        QBrush b;
        b.setColor(Qt::black);
        p.setBackground(Qt::white);
        b.setStyle((Qt::BrushStyle)i);
        p.setBrush(b);
        p.drawRect(0,0,16,16);
        p.end();
        QIcon ico(px);
//        ico.actualSize(QSize(16,16));
  //      ico.paint(&p, 0,0,16,16);

        br << pco->addAction(ico,opts[i-Qt::SolidPattern]);
    }
    auto *pcc = menu.addAction("Clear Pattern");
    menu.addSeparator();
    auto *fco = menu.addAction("Set Foreground Color");
    auto *fcc = menu.addAction("Clear Foreground Color");
    menu.addSeparator();
    auto *ct = menu.addAction("Clear Tags");



    auto res = menu.exec(ui->plateMaps->mapToGlobal(pos));

    if (res == ico)
    {

        QColor col = QColorDialog::getColor(Qt::white, this);
        if (col.isValid())
        {
            for (auto& item: ui->plateMaps->selectedItems())
                if (item)
                {
                    auto c = item->background();
                    c.setColor(col);
                    item->setBackground(c);
                }
        }
        return;
    }
    if (res == fco)
    {
        QColor col = QColorDialog::getColor(Qt::white, this);
        if (col.isValid())
        {
            for (auto& item: ui->plateMaps->selectedItems())
                if (item)
                    item->setForeground(col);
        }
        return;
    }

    if (res == cc)
    {
        for (auto& item: ui->plateMaps->selectedItems())
            if (item)
                item->setBackground(QColor(QColorConstants::White));

        return;
    }
    if (res == fcc)
    {
        for (auto & item: ui->plateMaps->selectedItems())
            if (item)
            {
                item->setForeground(QColor(QColorConstants::White));
            }
        return;
    }
    //    if (res == pco)
    if (br.contains(res))
    {
        // Pick pattern

        for (auto& item: ui->plateMaps->selectedItems())
            if (item)
            {
                auto b = item->background();
                b.setStyle((Qt::BrushStyle)(Qt::SolidPattern+br.indexOf(res)));
                item->setBackground(b);
            }

        return;
    }

    if (res == pcc)
    {
        for (auto& item: ui->plateMaps->selectedItems())
            if (item)
            {
                auto b = item->background();
                b.setStyle(Qt::SolidPattern);
                item->setBackground(b);
            }

        return;
    }
    if (res == ct)
    {
        for (auto& item: ui->plateMaps->selectedItems())
            if (item)
                item->setText(QString());
        return;
    }
}

