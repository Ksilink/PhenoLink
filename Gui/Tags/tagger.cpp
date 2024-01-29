#include "tagger.h"
#include "ui_tagger.h"


#include <QThread>

#include <cstdint>
#include <iostream>
#include <vector>

/*
#include <qhttp/qhttpclient.hpp>
#include "qhttp/qhttpclientrequest.hpp"
#include "qhttp/qhttpclientresponse.hpp"
*/
#include <QInputDialog>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "Tags/taggerplate.h"

#include <QMenu>
#include <QAction>

#include <QSettings>

#include <QFileDialog>
#include <QDir>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QHBoxLayout>


#include <Core/ck_mongo.h>

//using namespace qhttp::client;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using namespace bsoncxx::builder::basic;

tagger::tagger(QStringList datas, QWidget *parent) :
    QDialog(parent),
    dataset(datas), // http(nullptr),
    ui(new Ui::tagger)
{
    setWindowTitle("Setting Plate Tags");
    setModal(true);
    ui->setupUi(this);
    mongocxx::uri uri("mongodb://192.168.2.127:27017");
    mongocxx::client client(uri);

//    connect(this, SIGNAL(populate()), this, SLOT(on_populate()));

    this->_tags_of_tags["DMSO"].insert("Drugs"); // Tags a compound corresponding tag as compound
    this->_grouped_tags[""]["Drugs"].insert("DMSO");

    try {
        //        for (auto& dbn: client.list_database_names())
        //            qDebug() << QString::fromStdString(dbn);

        auto db = client["tags"];

        mongocxx::pipeline pipe{};

        pipe.group(make_document(kvp("_id", "$meta.project"), kvp("count", make_document(kvp("$sum", 1)))));

        auto cursor = db["tags"].aggregate(pipe);
        for (auto & item: cursor)
        {
            bsoncxx::stdx::string_view view = item["_id"].get_string().value;

            QString pr = QString::fromStdString(std::string{view}).simplified();
            if (!pr.isEmpty())
                _projects.insert(pr) ;
        }
        QString cur=this->ui->project->currentText();
        this->ui->project->clear();
        QStringList lst(_projects.begin(), _projects.end()); lst.sort();
        lst.push_front("");


        this->ui->project->addItems(lst);
        if (!cur.isEmpty())
            this->ui->project->setCurrentText(cur);

                qDebug() << "Mongo" << _projects;

        { // Mongo

            for (QString prj : _projects)
            {
                mongocxx::pipeline pipe{};
                pipe.match(make_document(kvp("meta.project", prj.toStdString())));
                pipe.project(make_document(kvp("data", make_document(kvp("$objectToArray", "$map")))));
                pipe.project(make_document(kvp("data", "$data.k")));
                pipe.unwind(make_document(kvp("path","$data"), kvp("preserveNullAndEmptyArrays", false)));
                pipe.group(make_document(kvp("_id", "$data"), kvp("count", make_document(kvp("$sum", 1)))));

                auto cursor = db["tags"].aggregate(pipe);
                for (auto & item: cursor)
                {
                    bsoncxx::stdx::string_view view = item["_id"].get_string().value;

                    _well_tags[prj].insert(QString::fromStdString(std::string{view}).simplified().replace("::", "."));
                    qDebug() << prj << QString::fromStdString(std::string{view}).simplified().replace("::", ".");
                }
            }
        }
    }  catch (...){

    }
 try
    {
        auto db = client["tags"];

        {
            mongocxx::pipeline pipe{};
            pipe.unwind(make_document(kvp("path","$meta.global_tags"), kvp("preserveNullAndEmptyArrays", false)));
            pipe.group(make_document(kvp("_id", "$meta.global_tags"), kvp("count", make_document(kvp("$sum", 1)))));

            auto cursor = db["tags"].aggregate(pipe);

            QStringList data;
            for (auto & item: cursor)
            {
                bsoncxx::stdx::string_view view = item["_id"].get_string().value;

                data << QString::fromStdString(std::string{view}).simplified().replace("::", ".");
            }

            ui->global_tags->addItems(data);
        }

        {
            mongocxx::pipeline pipe{};
            pipe.unwind(make_document(kvp("path","$meta.cell_lines"), kvp("preserveNullAndEmptyArrays", false)));
            pipe.group(make_document(kvp("_id", make_document(kvp("cell_lines", "$meta.cell_lines"), kvp("project", "$meta.project"))),

                                     kvp("count", make_document(kvp("$sum", 1)))));

            auto cursor = db["tags"].aggregate(pipe);

            for (auto & item: cursor)
            {
                bsoncxx::stdx::string_view prview = item["_id"]["project"].get_string().value;
                bsoncxx::stdx::string_view cellview = item["_id"]["cell_lines"].get_string().value;

                QString t = QString::fromStdString(std::string{cellview}).simplified().replace("::", ".");
                QString prj = QString::fromStdString(std::string{prview}).simplified().replace("::", ".");

                _grouped_tags[prj]["CellLines"].insert(t);
            }
            //ui->cell_lines->addItems(data);
        }

        {
            mongocxx::pipeline pipe{};
            pipe.unwind(make_document(kvp("path","$Categories.CellLines"), kvp("preserveNullAndEmptyArrays", false)));
            pipe.group(make_document(kvp("_id", make_document(kvp("cell_lines", "$Categories.CellLines"), kvp("project", "$meta.project"))),

                                     kvp("count", make_document(kvp("$sum", 1)))));

            auto cursor = db["tags"].aggregate(pipe);

            for (auto & item: cursor)
            {
                bsoncxx::stdx::string_view prview = item["_id"]["project"].get_string().value;
                bsoncxx::stdx::string_view cellview = item["_id"]["cell_lines"].get_string().value;

                QString t = QString::fromStdString(std::string{cellview}).simplified().replace("::", ".");
                QString prj = QString::fromStdString(std::string{prview}).simplified().replace("::", ".");

                _grouped_tags[prj]["CellLines"].insert(t);
            }
            //ui->cell_lines->addItems(data);
        }

    }
    catch(...) {}



    // FIXME add: media, density, treatment, coating, staining, drugs




    QWidget *twobut = new QWidget();
    twobut->setLayout(new QHBoxLayout());

    twobut->layout()->setContentsMargins(0,0,0,0);
    twobut->layout()->setSpacing(1);

    auto button = new QPushButton("Map CSV");
    auto button2 = new QPushButton("Map Template");


    twobut->layout()->addWidget(button);
    twobut->layout()->addWidget(button2);

    ui->Plates->setCornerWidget(twobut);

    connect(button, SIGNAL(clicked()), this, SLOT(on_mapcsv()));
    connect(button2, SIGNAL(clicked()), this, SLOT(on_maptemplate()));


    QSettings set;
    this->ui->xp_operator->setCurrentText(set.value("UserName", "").toString());


    try
    {

        for (auto & d: datas)
        {
            QStringList str = d.split("/");
            str.pop_back(); // remove the reference file name
            QString plate = str.last(); str.pop_back();// should be the plate name
            QString plateDate = str.last();
            QStringList date = str.last().replace(plate+"_", "").split("_"); str.pop_back();
            proj = str.last().simplified();

            if (!proj.isEmpty())
                _projects.insert(proj);

            TaggerPlate* platet = new TaggerPlate(d, this);
            platet->setPlateAcq(plateDate, plate);
            platet->setPath(d);
            QJsonObject& tags = platet->getTags();


            this->ui->experiment->setText(plate);

            this->ui->project->clear();
            QStringList lst(_projects.begin(), _projects.end()); lst.sort();
            lst.push_front("");
            this->ui->project->addItems(lst);


            // Try to find in the mongo the corresponding plate ?
            auto db = client["tags"];

            std::string plt = QString("%1/%2").arg(plateDate, plate).toStdString();

            auto fold = db["tags"].find_one(make_document(kvp("plateAcq",plt)));

            if (fold)
            {
                QByteArray arr = QString::fromStdString(bsoncxx::to_json(*fold)).toUtf8();
                //qDebug() << arr;
                QJsonParseError err;
                tags=QJsonDocument::fromJson(arr, &err).object();
                if (err.error != QJsonParseError::NoError)
                    qDebug() << err.errorString();
                //arr = tags.toJson();
                qDebug() <<"Mongodb info" << QString("%1/%2").arg(plateDate, plate) << tags["plateAcq"].toString() << tags["_id"].toObject()["$oid"].toString();
                //qDebug() << arr;
                if (tags.contains("meta") && tags["meta"].toObject().contains("project"))
                {
                    proj = tags["meta"].toObject()["project"].toString(); // mandatoringly :D
                    //   qDebug() << proj;
                }

            }
            else
            {
                qDebug() << "Plate Acq not found" << plateDate << "/" << plate;

                tags["plateAcq"]   = QString("%1/%2").arg(plateDate,plate);
                tags["serverPath"] = d;
                tags["plate"]      = plate;

                QStringList ops = QStringList() << "map" << "color_map" << "fgcolor_map" << "pattern_map";
                for (auto& p : ops) tags[p] = QJsonObject();

                QJsonObject meta;

                meta["project"]     = proj;
                meta["global_tags"] = QJsonArray();
                meta["operators"]   = QJsonObject();
                meta["XP"]          = plate;
                meta["cell_lines"]  = QJsonArray();



                tags["meta"]=meta;

                platet->updatePlate();
            }

            int p = std::min(1, (int)(date.size()-1));
            ui->Plates->addTab(platet, QString("%1 %2 %3").arg(plate, date.first(), date.at(p)));

            if (!proj.isEmpty())
                this->ui->project->setCurrentText(proj);

            platet->setTags(_grouped_tags, _well_tags, proj);
        }

    }
    catch (...)
    {}
}

tagger::~tagger()
{
    delete ui;
}

QString tagger::getProject() { return proj; }

void tagger::on_add_global_tags_clicked()
{
    QString str = ui->global_tags->currentText();

    for (int i = 0; i<  ui->global_tags_list->count(); ++i)
        if (ui->global_tags_list->item(i)->text()==str)
            return;

    ui->global_tags_list->addItem(str);
}




void tagger::on_global_tags_list_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    QString text = ui->global_tags_list->itemAt(pos)->text();

    QAction* addWb = menu.addAction(tr("Remove tag"), this, SLOT(removeTag()));
    addWb->setData(text);
    menu.exec(ui->global_tags_list->mapToGlobal(pos));
}



void tagger::removeTag()
{
    QAction* s = qobject_cast<QAction*>(sender());

    QStringList items;
    for (int i = 0; i < ui->global_tags_list->count(); ++i)
        if (s->data().toString() != ui->global_tags_list->item(i)->text())
            items << ui->global_tags_list->item(i)->text();

    ui->global_tags_list->clear();
    ui->global_tags_list->addItems(items);
}




void tagger::project_changed(const QString &arg1)
{
    //  qDebug() << "Setting project" << arg1;
    if (arg1.isEmpty())
        return;

    _projects.insert(arg1);

    proj = arg1;

    for (auto & w: this->findChildren<TaggerPlate*>())
    {
        //        qDebug() << w->objectName();

        if (qobject_cast<TaggerPlate*>(w))
        {
            auto platet = qobject_cast<TaggerPlate*>(w);
            platet->setTags(_grouped_tags, _well_tags, proj);
        }
    }
}


void tagger::on_pushButton_2_clicked()
{

    this->close();
}


// Ok button clicked save to Mongo !!!
void tagger::on_pushButton_clicked()
{
    qDebug() << "Shall be Saving to mongodb";
    // Tell each taggerplate to gather the associated infos in internal json representation
    // Retrieve the jsons
    // push to mongodb :p

    if (proj.isEmpty())
    {
        bool ok;
        proj = QInputDialog::getText(this, tr("Please set Project name"),
                                     "", QLineEdit::Normal,
                                     proj, &ok);
        if (!ok) return;
    }

    mongocxx::uri uri("mongodb://192.168.2.127:27017");
    mongocxx::client client(uri);

    auto db = client["tags"];
    auto coll = db["tags"];

    for (auto w: this->findChildren<TaggerPlate*>())
    {
        if (qobject_cast<TaggerPlate*>(w))
        {
            auto platet = qobject_cast<TaggerPlate*>(w);
            auto json = platet->refreshJson();

            QJsonObject meta = json["meta"].toObject();

            meta["project"] = proj;

            auto ops = meta["operators"].toObject();
            ops["XP"] = ui->xp_operator->currentText();
            meta["operators"] = ops;

            meta["measurement"] = ui->experiment->text();
            auto arr = meta["global_tags"].toArray();

            for (int i = 0; i < ui->global_tags_list->count(); ++i)
                arr.append(ui->global_tags_list->item(i)->text());

            meta["global_tags"] = arr;

            json["meta"] = meta;

            std::string qdoc = QJsonDocument(json).toJson().toStdString(),
                plt = json["plateAcq"].toString().toStdString();

            auto doc = bsoncxx::from_json(qdoc);

            if (json.contains("_id"))
                coll.replace_one(make_document(kvp("plateAcq", plt)),  doc.view());
            else
                coll.insert_one(doc.view());

        }
    }

    this->close();
}

QWidget* createPair(QComboBox* left, QComboBox* right)
{
    QWidget* res = new QWidget(left->parentWidget());
    auto lay = new QHBoxLayout();
    res->setLayout(lay);
    lay->addWidget(left);
    lay->addWidget(right);

    return res;
}

void tagger::on_mapcsv()
{

    QSettings set;


    qDebug() << "Query for CSV & Map CSV file to the plate names";
    QString script = QFileDialog::getOpenFileName(this, "Choose Template storage path",
                                                  set.value("LastTagsCSV", QDir::home().path()).toString(), "CSV file (*.csv)",
                                                  0, /*QFileDialog::DontUseNativeDialog | */
                                                  QFileDialog::DontUseCustomDirectoryIcons
                                                  );

    if (!script.isEmpty())
    {
        QStringList sc = script.split("/"); sc.pop_back();   set.setValue("LastTagsCSV", sc.join("/"));

        QFile io(script);
        if (io.open(QFile::ReadOnly))
        {
            QString hea= io.readLine();
            QStringList header = hea.contains(',') ? hea.split(',') : (hea.contains(";") ? hea.split(';') : (hea.contains('\t') ? hea.split('\t') : hea.split(",")));
            QStringList h2 = header; h2.prepend("None/Skip");
            int wc = 0, pc = 0, c=0;

            for (auto& str: header)
            {
                if (str.contains("well", Qt::CaseInsensitive))
                    wc = c;
                if (str.contains("plate", Qt::CaseInsensitive))
                    pc = c;
                c++;
            }

            QDialog box(this);

            auto layout = new QFormLayout();
            box.setLayout(layout);
            auto wells = new QComboBox(&box); wells->addItems(header);
            wells->setCurrentIndex(wc);


            auto plates = new QComboBox(&box); plates->addItems(h2);
            plates->setCurrentIndex(pc+1);

            layout->addRow("Pick Well Columns", wells);
            layout->addRow("Pick Plate Columns", plates);

            QList<QComboBox*> features,subfeatures;
            for (int i = 2; i < header.size(); ++i)
            {
                features.push_back(new QComboBox(&box));
                features.back()->addItems(h2);

                subfeatures.push_back(new QComboBox(&box));
                subfeatures.back()->addItems(h2);


                layout->addRow(QString("Pick Feature %1").arg(i),
                               createPair(features.back(), subfeatures.back()));
            }
            auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                  | QDialogButtonBox::Cancel);

            connect(buttonBox, &QDialogButtonBox::accepted, &box, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, &box, &QDialog::reject);
            layout->addRow(buttonBox);

            QMap<QString, QStringList> categories;

            if (box.exec()==QDialog::Accepted)
            {
                while(true)
                {
                    hea= io.readLine();
                    if (hea.isEmpty())
                        break;
                    header = hea.contains(',') ? hea.split(',') : (hea.contains(";") ? hea.split(';') : (hea.contains('\t') ? hea.split('\t') : hea.split(",")));

                    QStringView well = header.at(wells->currentIndex()).toUpper();
                    int r = (char)(well[0].toLatin1())-'A', c = well.sliced(1).toInt()-1;

                    for (auto w: this->findChildren<TaggerPlate*>())
                    {
                        if (qobject_cast<TaggerPlate*>(w))
                        {
                            auto platet = qobject_cast<TaggerPlate*>(w);
                            if (plates->currentIndex() != 0)
                            {
                                if (platet->getPlate() == header.at(plates->currentIndex()-1))
                                {
                                    int i = 0;
                                    for (auto& col : features)
                                    {
                                        if (col->currentIndex()!=0)
                                        {
                                            auto t = header.at(col->currentIndex()-1);
                                            if (subfeatures.at(i)->currentIndex()!=0)
                                                t += "#" + header.at(subfeatures.at(i)->currentIndex()-1);

                                            if (!categories[col->currentText()].contains(t))
                                                categories[col->currentText()]<<t;

                                            platet->setTag(r,c,t);
                                        }
                                        i++;
                                    }
                                }
                            }
                            else
                            {
                                int i = 0;
                                for (auto& col : features)
                                {
                                    if (col->currentIndex()!=0)
                                    {
                                        auto t = header.at(col->currentIndex()-1);

                                        if (subfeatures.at(i)->currentIndex()!=0)
                                            t += "#" + header.at(subfeatures.at(i)->currentIndex()-1);


                                        if (!categories[col->currentText()].contains(t))
                                            categories[col->currentText()]<<t;
                                        platet->setTag(r,c,header.at(col->currentIndex()-1));
                                    }
                                    i++;
                                }
                            }
                        }
                    }
                }
            }


            for (auto w: this->findChildren<TaggerPlate*>())
            {
                if (qobject_cast<TaggerPlate*>(w))
                {
                    auto platet = qobject_cast<TaggerPlate*>(w);
                    platet->setCategories(categories);
                }
            }

            delete layout;
            delete plates;
            delete wells;
            for (auto& c: features)
                delete c;
            features.clear();

        }
    }
}


void tagger::on_maptemplate()
{
    QSettings set;

    QString script = QFileDialog::getOpenFileName(this, "Choose Template storage path",
                                                  set.value( "LastTagsJSON", QDir::home().path()).toString(), "Tags json file (*.json)",
                                                  0, /*QFileDialog::DontUseNativeDialog                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              | */QFileDialog::DontUseCustomDirectoryIcons
                                                  );

    if (!script.isEmpty())
    {
        QStringList sc = script.split("/"); sc.pop_back(); set.setValue("LastTagsJSON", sc.join("/"));

        for (auto w: this->findChildren<TaggerPlate*>())
        {
            if (qobject_cast<TaggerPlate*>(w))
            {
                auto platet = qobject_cast<TaggerPlate*>(w);
                platet->apply_template(script);
            }
        }
    }
}



//void tagger::on_populate()
//{
//    if (!proj.isEmpty())
//        for (auto w: this->findChildren<TaggerPlate*>())
//        {
//            if (qobject_cast<TaggerPlate*>(w))
//            {
//                auto platet = qobject_cast<TaggerPlate*>(w);
//                platet->setTags(_grouped_tags, _well_tags, proj);
//            }
//        }
//}


void tagger::on_Plates_currentChanged(int index)
{
    // When changing plate fuse the inputs with next one
}


void tagger::on_project_currentIndexChanged(int index)
{
    project_changed(ui->project->itemText(index));
}

