#include "tagger.h"
#include "ui_tagger.h"


#include <QThread>

#include <cstdint>
#include <iostream>
#include <vector>

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/stdx/string_view.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>


#include <qhttp/qhttpclient.hpp>
#include "qhttp/qhttpclientrequest.hpp"
#include "qhttp/qhttpclientresponse.hpp"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "Tags/taggerplate.h"

#include <QMenu>
#include <QAction>

#include <QSettings>



mongocxx::instance mongo_instance {}; // This should be done only once.

using namespace qhttp::client;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using namespace bsoncxx::builder::basic;

tagger::tagger(QStringList datas, QWidget *parent) :
    QDialog(parent),
    dataset(datas), http(nullptr),
    ui(new Ui::tagger)
{
    ui->setupUi(this);


    this->_tags_of_tags["DMSO"].insert("Drugs"); // Tags a compound corresponding tag as compound
    this->_grouped_tags["Drugs"].insert("DMSO");


    http = getHttp();
    QString url("http://labcollector.ksilink.int/webservice/index.php?v=2&module=strains"),
            key("74a7d185e35ca33dc082f3e1605b914d1c6fc1c1add3ef4f96e6a284952199f2");

    http->request(qhttp::EHTTP_GET, url,
                  [key, url]( qhttp::client::QHttpRequest* req){
        req->addHeader("X-LC-APP-Auth", key.toLatin1());
        req->addHeader("Accept", "application/json");
        QByteArray body;
        req->addHeaderValue("content-length", body.length());
        req->end(body);
    },

    [this]( qhttp::client::QHttpResponse* res) {
        res->collectData();
        res->onEnd([this, res](){
            auto data = res->collectedData();
            this->labcollector_strains = QJsonDocument::fromJson(data);
            for (auto items: this->labcollector_strains.array())
            {
                auto obj = items.toObject();
                QString pr = obj["project"].toString().simplified();
                if (!pr.isEmpty())
                {
                    _projects.insert(pr);
                    if (!obj["number"].toString().simplified().isEmpty())
                    {
                        this->_well_tags[pr].insert(obj["number"].toString());
                        this->_tags_of_tags[obj["number"].toString()].insert("CellLines"); // Tags the tag as a cell line
                        this->_grouped_tags["CellLines"].insert(obj["number"].toString().simplified());
                    }
                }
            }

            QString cur=this->ui->project->currentText();
            this->ui->project->clear();
            QStringList lst(_projects.begin(), _projects.end()); lst.sort();

            this->ui->project->addItems(lst);
            if (!cur.isEmpty())
                this->ui->project->setCurrentText(cur);


        });
    });


    mongocxx::uri uri("mongodb://192.168.2.127:27017");
    mongocxx::client client(uri);

    try {
        for (auto& dbn: client.list_database_names())
            qDebug() << QString::fromStdString(dbn);

        auto db = client["tags"];

        mongocxx::pipeline pipe{};

        pipe.group(make_document(kvp("_id", "$meta.project"), kvp("count", make_document(kvp("$sum", 1)))));

        auto cursor = db["tags"].aggregate(pipe);
        for (auto & item: cursor)
        {
            bsoncxx::stdx::string_view view = item["_id"].get_string().value;
            QString pr = QString::fromStdString(view.to_string()).simplified();
            if (!pr.isEmpty())
                _projects.insert(pr) ;
        }
        QString cur=this->ui->project->currentText();
        this->ui->project->clear();
        QStringList lst(_projects.begin(), _projects.end()); lst.sort();

        this->ui->project->addItems(lst);
        if (!cur.isEmpty())
            this->ui->project->setCurrentText(cur);

        //        qDebug() << "Mongo" << _projects;

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
                    _well_tags[prj].insert(QString::fromStdString(view.to_string()).simplified());
                }
            }
        }
    }  catch (...){

    }


    url="http://labcollector.ksilink.int/webservice/index.php?v=2&module=chemicals";

    http->request(qhttp::EHTTP_GET, url,
                  [key, url]( qhttp::client::QHttpRequest* req){
        req->addHeader("X-LC-APP-Auth", key.toLatin1());
        req->addHeader("Accept", "application/json");
        QByteArray body;
        req->addHeaderValue("content-length", body.length());
        req->end(body);
    },

    [this]( qhttp::client::QHttpResponse* res) {
        res->collectData();
        res->onEnd([this, res](){
            auto data = res->collectedData();
            QMap<QString, QString> map={ // No API way found as of yet to get this info...
                                         //                                         {"1","Reagents"},
                                         {"3","Drugs"},
                                         //                                         {"4","Cell Culture Room"},// degage
                                         //                                         {"5","Pipettes"},// degage
                                         //                                         {"6","Filtration"},// degage
                                         //                                         {"7","Reservoirs"},// degage
                                         //                                         {"8","Gloves"},// degage
                                         {"9","Antibodies"},
                                         //                                         {"10","Plates"},// degage
                                         //                                         {"11","Sanitisers"},// degage
                                         //                                         {"12","Screening"}, // degage
                                         //                                         {"13","Tips"},// degage
                                         //                                         {"14","Tubes"},// degage
                                         //                                         {"15","Western Blot"}// degage
                                       };
            this->labcollector_chemicals = QJsonDocument::fromJson(data);
            for (auto items: this->labcollector_chemicals.array())
            {
                auto obj = items.toObject();
                if (map.contains(obj["cat_id"].toString().simplified()))
                {
                    this->_tags_of_tags[obj["name"].toString().simplified()].insert(map[obj["cat_id"].toString().simplified()]); // Tags a compound corresponding tag as compound
                    this->_grouped_tags[map[obj["cat_id"].toString().simplified()]].insert(obj["name"].toString().simplified());
                }
            }
        });
    });

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
                data << QString::fromStdString(view.to_string()).simplified();
            }

            ui->global_tags->addItems(data);
        }

        {
            mongocxx::pipeline pipe{};
            pipe.unwind(make_document(kvp("path","$meta.cell_lines"), kvp("preserveNullAndEmptyArrays", false)));
            pipe.group(make_document(kvp("_id", "$meta.cell_lines"), kvp("count", make_document(kvp("$sum", 1)))));

            auto cursor = db["tags"].aggregate(pipe);

            QSet<QString> data;
            for (auto & item: cursor)
            {
                bsoncxx::stdx::string_view view = item["_id"].get_string().value;
                QString t = QString::fromStdString(view.to_string()).simplified();
                if (!t.isEmpty())
                    data.insert(t);
            }
            _grouped_tags["CellLines"] |= data;
            //ui->cell_lines->addItems(data);
        }
    }
    catch(...) {}


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
        QJsonDocument& tags = platet->getTags();

        this->ui->project->clear();
        this->ui->experiment->setText(plate);
        QStringList lst(_projects.begin(), _projects.end()); lst.sort();
        this->ui->project->addItems(lst);


        // Try to find in the mongo the corresponding plate ?
        auto db = client["tags"];
        auto fold = db["tags"].find_one(make_document(kvp("plateAcq",QString("%1/%2").arg(plateDate,plate).toStdString())));
        if (fold)
        {
            QByteArray arr = QString::fromStdString(bsoncxx::to_json(*fold)).toLatin1();
            tags=QJsonDocument::fromJson(arr);
            QJsonObject ob = tags.object();
            if (ob.contains("meta") && ob["meta"].toObject().contains("project"))
            {
                proj = ob["meta"].toObject()["project"].toString(); // mandatoringly :D
                //   qDebug() << proj;
            }

            platet->updatePlate();
        }
        else qDebug() << "Plate Acq not found" << plateDate << "/" << plate;

        ui->Plates->addTab(platet, QString("%1 %2 %3").arg(plate, date.first(), date.at(1)));

        if (!proj.isEmpty())
            this->ui->project->setCurrentText(proj);

        platet->setTags(_grouped_tags, _well_tags[proj]);

    }

    QSettings set;
    this->ui->xp_operator->setCurrentText(set.value("UserName", "").toString());
}

tagger::~tagger()
{
    delete ui;
}

QHttpClient *tagger::getHttp()
{
    if (!http)
        http = new QHttpClient();
    return http;
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




void tagger::on_project_currentIndexChanged(const QString &arg1)
{
    //  qDebug() << "Setting project" << arg1;
    if (arg1.isEmpty())
        return;

    _projects.insert(arg1);
    proj = arg1;
    for (auto w: this->findChildren<TaggerPlate*>())
    {
        //        qDebug() << w->objectName();

        if (qobject_cast<TaggerPlate*>(w))
        {
            auto platet = qobject_cast<TaggerPlate*>(w);
            platet->setTags(_grouped_tags, _well_tags[proj]);
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

}

