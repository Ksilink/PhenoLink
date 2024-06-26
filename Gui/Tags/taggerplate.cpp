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


#include <QFileDialog>
#include <QFile>


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
        header << QString(QChar('A'+(char)i));

    ui->plateMaps->setVerticalHeaderLabels(header);

    ui->plateMaps->resizeColumnsToContents();
    ui->plateMaps->resizeRowsToContents();

    for (int r = 0; r < 'Q'; ++r)
        for (int c = 0; c < 24; ++c)
            if (!ui->plateMaps->item(r, c))
            {
                auto item = new QTableWidgetItem(QString());
                ui->plateMaps->setItem(r, c, item);
                auto b = item->background();
                b.setStyle(Qt::SolidPattern);
                b.setColor(Qt::transparent);
                item->setBackground(b);

            }



}

TaggerPlate::~TaggerPlate()
{
    delete ui;
}

QJsonObject &TaggerPlate::getTags() {

    return tagger;
}

void TaggerPlate::setCategories(QMap<QString, QStringList> map)
{
    auto mlcpds = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    if (!mlcpds) return;

    auto ml = qobject_cast<QSortFilterProxyModel*>(mlcpds->sourceModel());
    if (!ml) return;

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ml->sourceModel());
    if (!model) return;

    QStandardItem* root = model->invisibleRootItem();

    for (auto& name: map.keys())
    {
        name=name.simplified();
        QStandardItem* parent = nullptr;
        for (int i = 0; i < root->rowCount(); ++i)
            if (root->child(i)->text() == name)
            {
                parent = root->child(i);
                break;
            }

        if (!parent)
        {
            parent = new QStandardItem(name);
            root->appendRow(parent);
        }

        for (auto& subname: map[name]) parent->appendRow(new QStandardItem(subname));
    }

}

void TaggerPlate::on_setTags_clicked()
{
    auto mlcpds = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    if (!mlcpds) return;

    auto ml = qobject_cast<QSortFilterProxyModel*>(mlcpds->sourceModel());
    if (!ml) return;


    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ml->sourceModel());
    if (!model) return;

    QStandardItem* root = model->invisibleRootItem();


    auto a0 = ui->treeView->selectionModel()->currentIndex();

    auto a2 = mlcpds->mapToSource(a0);

    auto idx = ml->mapToSource(a2);



   // auto idx = ml->mapToSource(mlcpds->mapToSource(ui->treeView->selectionModel()->currentIndex()));

//    qDebug() << idx;// << idx.parent();
    if (idx.isValid() && idx.parent().isValid())
    {
        //        qDebug() << idx << idx.parent();
        //        qDebug() << root;
        QStandardItem* item = root->child(idx.parent().row());
        //        qDebug() << item;
        if (item)
        {
            QString str = item->child(idx.row())->text();
            //      qDebug() << item->text() << str << ui->plateMaps->selectedItems().size();

            for (auto idx: ui->plateMaps->selectionModel()->selectedIndexes())
            {
                setTag(idx.row(), idx.column(), str);
            }
            //            item->text() << str

            auto cato = tagger["Categories"].toObject();
            auto arr = cato[item->text()].toArray();
            if (!arr.contains(str)) arr.append(str);
            cato[item->text()] = arr;
            tagger["Categories"] = cato;
        }
    }
}


void TaggerPlate::on_unsetTags_clicked()
{
    auto mlcpds = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    if (!mlcpds) return;

    auto ml = qobject_cast<QSortFilterProxyModel*>(mlcpds->sourceModel());
    if (!ml) return;


    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ml->sourceModel());
    if (!model) return;

    QStandardItem* root = model->invisibleRootItem();

    auto a0 = ui->treeView->selectionModel()->currentIndex();

    auto a2 = mlcpds->mapToSource(a0);

    auto idx = ml->mapToSource(a2);

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
//void TaggerPlate::on_plates_design_currentIndexChanged(const QString &)
//{
//}


// Find Template
void TaggerPlate::apply_template(QString script)
{
    if (script.isEmpty()) return;

    QFile of(script);
    if (of.open(QIODevice::ReadOnly))
    {
        QByteArray ar = of.readAll();
        auto tags = QJsonDocument::fromJson(ar);

        auto tg = tags.object();
        tg.remove("_id");
        //            auto ftag = tagger.object();
        for (auto k : tags.object().keys())
            if (k != "map")
            {
                tagger[k] = tg[k];

            }
            else
            {
                QJsonObject maps = tg[k].toObject(), res;
                for (auto& v : maps.keys())
                {
                    //                        qDebug() << v;
                    auto k = v;
                    res[v.replace(".", "::")] = maps[k];
                }
                tagger[k] = res;
            }

        QByteArray bar = QJsonDocument(tagger).toJson();
        updatePlate();
    }
}

void TaggerPlate::on_pushButton_clicked()
{

    QSettings set;

    QString script = QFileDialog::getOpenFileName(this, "Choose Template storage path",
                                                  set.value( "LastTagsJSON", QDir::home().path()).toString(), "Tags json file (*.json)",
                                                  0, /*QFileDialog::DontUseNativeDialog                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              | */QFileDialog::DontUseCustomDirectoryIcons
                                                  );

    if (!script.isEmpty())
    {
        QStringList sc = script.split("/"); sc.pop_back(); set.setValue("LastTagsJSON", sc.join("/"));
        apply_template(script);
    }

}

// Export Template
void TaggerPlate::on_pushButton_2_clicked()
{
    QString script =
        QFileDialog::getSaveFileName(this, "Choose Template storage path",
                                     QDir::home().path(), "Tags json file (*.json)",
                                     0, /*QFileDialog::DontUseNativeDialog                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              | */QFileDialog::DontUseCustomDirectoryIcons
                                     );

    if (!script.isEmpty())
    {
        refreshJson();
        QFile of(script);
        auto tg = tagger;

        if (of.open(QIODevice::WriteOnly))
        {
            tg.remove("_id");
            of.write(QJsonDocument(tg).toJson());
        }
    }
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


    auto mlcpds = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    if (!mlcpds) return;

    auto ml = qobject_cast<QSortFilterProxyModel*>(mlcpds->sourceModel());
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

            for (int i = 0; i < root->rowCount() && parent == nullptr; ++i)
            {
                if (root->child(i)->text() == name)
                    parent = root->child(i);

                if (!parent)
                    for (int j = 0; j < root->child(i)->rowCount() && parent == nullptr; ++j)
                        if (root->child(i)->child(j)->text() == name)
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
        model = new QStandardItemModel(0, 3);

        projmdl = new QSortFilterProxyModel(this);

        projmdl->setRecursiveFilteringEnabled(true);
        projmdl->setFilterCaseSensitivity(Qt::CaseInsensitive);
        projmdl->setFilterKeyColumn(2);
        //        mdl->setSortRole():
        // Cascade with the "project" entry
        projmdl->setSourceModel(model);

        mdl = new QSortFilterProxyModel(this);

        mdl->setRecursiveFilteringEnabled(true);
        mdl->setFilterCaseSensitivity(Qt::CaseInsensitive);

        mdl->setSourceModel(projmdl);



        ui->treeView->setModel(mdl);
        model->setHorizontalHeaderLabels(QStringList() << "Label" << "Amount" << "Project");
        //        model->set
    }
    else
    {

        model = qobject_cast<QStandardItemModel*>(qobject_cast<QSortFilterProxyModel*>(ml->sourceModel())->sourceModel());
    }
    if (!model) // not ready yet
        return;

    ui->projectFilter->setText(project);

    QStandardItem* root = model->invisibleRootItem();
//    root->removeRows(0, root->rowCount());


    QSet<QString> skip;
//    QStringList headers = QStringList() << "" << project;
  //  qDebug() << "Cmpds" << data;
    for (auto& k : data.keys())
        for (auto & key: data[k].keys())
        {

            QStandardItem * item = nullptr;
            for (int i = 0; i < root->rowCount();++i)
                if (root->child(i)->text()==key)
                    item=root->child(i);

            if (!item) {
                item = new QStandardItem(key);
                root->appendRow(item);
            }
            for (auto& prod: data[k][key])
            {
                if (!prod.simplified().isEmpty())
                {

                    QList<QStandardItem *> items;
                    items << new QStandardItem(prod)
                          << new QStandardItem("")
                          << new QStandardItem(k);
                    item->appendRow(items);
                }
                for (const QString& ot: othertags[project])
                {
                    if (ot.startsWith(prod))
                        skip.insert(ot);
                }
            }
        }

    if (tagger.contains("Categories"))
    {
        auto cats = tagger["Categories"].toObject();

        for (const auto & k : cats.keys())
        {
            QStandardItem* r = nullptr;
            for (int i = 0; i < root->rowCount(); ++i)
                if (root->child(i)->text() == k)
                    r = root->child(i);
            if (!r) {

                QList<QStandardItem *> items;
                items << new QStandardItem(k)
                      << new QStandardItem("")
                      << new QStandardItem(project);
                root->appendRow(items);
            }

            auto ar = cats[k].toArray();
            for (const auto &p: ar)
            {
                QString it = p.toString();
                QStandardItem* ap = nullptr;
                for (int i = 0; i < r->rowCount(); ++i)
                    if (r->child(i)->text() == it)
                        ap = r->child(i);
                if (!ap) {
                    QList<QStandardItem *> items;
                    items << new QStandardItem(it)
                          << new QStandardItem("")
                          << new QStandardItem(project);
                    r->appendRow(items);
                }
            }
        }
    }



    QStandardItem* others = nullptr;

    for (int i = 0; i < root->rowCount(); ++i)
        if (root->child(i)->text() == "Others")
        {
            others = root->child(i);
            break;
        }

    if (!others)
    {
        others = new QStandardItem("Others");
        root->appendRow(others);
    }

    for (const QString& ot: othertags[project])
        if (!skip.contains(ot) && !ot.simplified().isEmpty())
        {
            others->appendRow(new QStandardItem(ot));
        }

    updatePlate();
}

void TaggerPlate::setTag(int r, int c, QString tags)
{

    //    qDebug() << "Set Tag" << r << c << tags;
    tags = tags.replace("::", ".");
    if (!ui->plateMaps->item(r,c))
    {
        auto item = new QTableWidgetItem(tags);
        item->setToolTip(tags.replace(";","\r\n"));
        ui->plateMaps->setItem(r,c, item);

    }
    else
    {
        QStringList lst = ui->plateMaps->item(r,c)->text().split(';');
        for (auto &t: tags.split(';')) lst.append(t);

        QSet<QString> st = QSet<QString>(lst.begin(), lst.end());

        lst.clear();

        for (auto& t: st) if (!t.simplified().isEmpty()) lst << t.simplified();

        lst.sort();

        ui->plateMaps->item(r,c)->setText(lst.join(";"));
        ui->plateMaps->item(r,c)->setToolTip(lst.join("\r\n"));
    }


    ui->plateMaps->resizeColumnsToContents();
    ui->plateMaps->resizeRowsToContents();
}

void TaggerPlate::setColor(int r, int c, QString color)
{
    if (!ui->plateMaps->item(r,c))
        ui->plateMaps->setItem(r,c, new QTableWidgetItem());

    auto item = ui->plateMaps->item(r, c);
    auto b = item->background();
    b.setStyle(Qt::SolidPattern);
    b.setColor(QColor(color));
    item->setBackground(b);

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

    if (tagger.contains("meta"))
    {
        auto meta = tagger["meta"].toObject();

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
    if (tagger.contains("map"))
    { // Unroll the tags
        auto map = tagger["map"].toObject();
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
    if (tagger.contains("color_map"))
    { // Unroll the colors
        auto map = tagger["color_map"].toObject();
        for (auto it = map.begin(), e = map.end(); it != e; ++it)
        {
            QString ctag = it.key();
            if (ctag == "#ffffff" || ctag == "#000000") continue;
            auto wells = it.value().toObject();
            for (auto k = wells.begin(), ee = wells.end(); k != ee; ++k)
            {
                auto arr = k.value().toArray();
                int r = k.key().toUtf8().at(0) - 'A';
                for (int i = 0; i < arr.size(); ++i)
                {
                    int c = arr[i].toInt();

                    //                qDebug() << "set Color" << ctag << "to" << QString("%1%2").arg(k.key()).arg(c+1,2, 10, QLatin1Char('0'));
                    setColor(r,c, ctag);
                }
            }
        }
    }
    if (tagger.contains("fgcolor_map"))
    { // Unroll the colors
        auto map = tagger["fgcolor_map"].toObject();
        for (auto it = map.begin(), e = map.end(); it != e; ++it)
        {
            QString ctag = it.key();
            if (ctag == "#ffffff" || ctag == "#000000") continue;
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
    if (tagger.contains("pattern_map"))
    { // Unroll the patterns
        auto map = tagger["pattern_map"].toObject();
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

    tagger["serverPath"] = path;
    tagger["plate"] = plate;
    tagger["plateAcq"] = QString("%1/%2").arg(plateDate, plate);


//    QStringList cats;
//    for (int i = 0; i < root->rowCount(); ++i)
//        cats.append(root->child(i)->text());

}




QJsonObject aC(QJsonObject& ob, QString p, int v)
{
    if (ob.contains(p))
    {
        auto ar = ob[p].toArray();
        ar.append(v);
        ob[p] = ar;
        return ob;
    }
    else
    {

        QJsonArray ar;
        ar.append(v);
        ob[p] = ar;
        return ob;
    }
}


void addInfo(QJsonObject& ob, QString map, QString gl, QString row, int col)
{
    QJsonObject obj, token;

    gl = gl.simplified().replace(".","::");
    if (gl.isEmpty())
        return;

    obj = ob[map].toObject();
    token = obj[gl].toObject();

    aC(token, row, col);

    obj[gl] = token;
    ob[map] = obj;

}


QJsonObject TaggerPlate::refreshJson()
{
    // We shall update the tags with respect to the content of the GUI

    QMap<QString, QMap<QString, QList<int> > > tags, color, fgcolor, pattern;

    QColor transparent(Qt::transparent);

    QStringList ops =  QStringList() << "map" << "color_map" << "fgcolor_map" << "pattern_map";
    for (auto& p : ops) tagger[p] = QJsonObject(); // Clear the mapping prior to applying the view

    QStringList used_tags;

    for (int c = 0; c < ui->plateMaps->columnCount(); ++c)
        for (unsigned char r = 0; r < (unsigned char)std::min(255, ui->plateMaps->rowCount()); ++r)
            if (ui->plateMaps->item(r,c))
            {

                auto item = ui->plateMaps->item(r,c);

                QStringList stags = item->text().split(";");

                for (QString& t: stags)
                {
                    if (!used_tags.contains(t)) used_tags << t;

                    addInfo(tagger, "map", t, QString("%1").arg(QLatin1Char('A' + r)), c);
                }

                if (item->foreground().color().isValid() && item->foreground().color().name() != "#000000" && item->foreground().color().name() != "#ffffff" && item->background().color() != transparent)
                    addInfo(tagger, "fgcolor_map", item->foreground().color().name(), QString("%1").arg(QLatin1Char('A'+r)), c);

                if (item->background().color().isValid() && item->background().color().name() != "#000000" && item->background().color().name() != "#ffffff" && item->background().color() != transparent)
                    addInfo(tagger, "color_map", item->background().color().name(), QString("%1").arg(QLatin1Char('A'+r)), c);

                if (item->background().style() != Qt::SolidPattern && item->background().style() != Qt::NoBrush)
                    addInfo(tagger, "pattern_map", QString("%1").arg((int)item->background().style()),QString("%1").arg(QLatin1Char('A'+r)), c);
            }


    QJsonObject categories;
    auto mlcpds = qobject_cast<QSortFilterProxyModel*>(ui->treeView->model());
    if (mlcpds)
    {

    auto ml = qobject_cast<QSortFilterProxyModel*>(mlcpds->sourceModel());

    if (ml)
    {

        QSet<QString> seen;


        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ml->sourceModel());
        if (model) {

            QStandardItem* root = model->invisibleRootItem();
            for (int i = 0; i < root->rowCount(); ++i)
                {
                    auto ch = root->child(i);//->text();
                    for (int j = 0; j < ch->rowCount(); ++j)
                    if (used_tags.contains(ch->child(j)->text()))
                    {
                        QJsonArray ar = categories.contains(ch->text()) ?
                                            categories[ch->text()].toArray()
                                                                        : QJsonArray();
                        QString s = ch->child(j)->text().trimmed();
                        if (ar.contains(s) || seen.contains(s))
                            continue;

                        seen.insert(s);

                        ar.append(s);

                        categories[ch->text()]=ar;
                    }
                }
        }
    }

    }

    tagger.insert("plateAcq",QJsonValue(QString("%1/%2").arg(plateDate, plate)));
    tagger.insert("plate",QJsonValue(plate));
    tagger.insert("serverPath", QJsonValue(path));
    tagger.insert("Categories", categories);

    if (!tagger.contains("acquisitionDate"))
    {
        QStringList dateList=plateDate.split("_");
        if (dateList.size() > 2)
        {
        auto date=QDateTime::fromString(dateList[dateList.size()-2]+"-"+dateList[dateList.size()-1], "yyyyMMdd-HHmmss");

        tagger.insert("acquisitionDate", date.toString(Qt::ISODateWithMs));
        }
    }

    if (!tagger.contains("size"))
    {
        QDir folder(path);
        folder.cdUp();

        auto flist = folder.entryList(QStringList() << "*.jxl" << "*.tif" );
        if (flist.size() > 0)
        {
        QFile img(flist.first());
        float im_size = img.size();

        tagger.insert("size", (flist.size() * im_size ) / 1024 / 1024 / 1024);
        }
        else
            qDebug() << "Not able to compute folder size due to empty results from folder.entryList"
                     << folder << path;
    }

    return tagger;
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

        QColor col = QColorDialog::getColor(Qt::transparent, this);
        if (col.isValid())
        {
            for (auto& item: ui->plateMaps->selectedItems())
                if (item)
                {
                    auto c = item->background();
                    c.setColor(col);
                    c.setStyle(Qt::SolidPattern);

                    item->setBackground(c);
                }
        }
        return;
    }
    if (res == fco)
    {
        QColor col = QColorDialog::getColor(Qt::transparent, this);
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
                item->setBackground(QColor(Qt::transparent));

        return;
    }
    if (res == fcc)
    {
        for (auto & item: ui->plateMaps->selectedItems())
            if (item)
            {
                item->setForeground(QColor(Qt::transparent));
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

        tagger["map"]=QJsonObject();

        return;
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

void TaggerPlate::on_projectFilter_textChanged(const QString &arg1)
{
    if (arg1.isEmpty())
        ui->treeView->setSortingEnabled(false);
    else
        ui->treeView->setSortingEnabled(true);

    // Adjust the filter of the treeview
    if (projmdl)
    {
        projmdl->setFilterFixedString(arg1);
        projmdl->setRecursiveFilteringEnabled(true);
        projmdl->setFilterCaseSensitivity(Qt::CaseInsensitive);
    }
}

