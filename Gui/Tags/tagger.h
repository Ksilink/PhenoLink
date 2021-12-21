#ifndef TAGGER_H
#define TAGGER_H

#include <QDialog>

//#include <cstdint>
//#include <iostream>
//#include <vector>

//#include <bsoncxx/json.hpp>
//#include <mongocxx/client.hpp>
//#include <mongocxx/stdx.hpp>
//#include <mongocxx/uri.hpp>
//#include <mongocxx/instance.hpp>
//#include <bsoncxx/builder/stream/helpers.hpp>
//#include <bsoncxx/builder/stream/document.hpp>
//#include <bsoncxx/builder/stream/array.hpp>

#include <QJsonDocument>

namespace Ui {
class tagger;
}

namespace qhttp { namespace client { class QHttpClient; }  }

class tagger : public QDialog
{
    Q_OBJECT

public:
    explicit tagger(QStringList data, QWidget *parent = nullptr);
    ~tagger();


    int exec()
    {
        int res = QDialog::exec();
        if (res==1)
        {
            qDebug() << res << "Accepted dialog need to store the plate tags";
        }
        return res;
    }

    qhttp::client::QHttpClient* getHttp();

    QString getProject();

private slots:
    void on_add_global_tags_clicked();

    void on_global_tags_list_customContextMenuRequested(const QPoint &pos);

    void removeTag();
    void on_project_currentIndexChanged(const QString &arg1);

    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    QStringList dataset;

    QString proj; // for validation & or suggestion

    // For labcollector connection

    qhttp::client::QHttpClient* http;

    QSet<QString> _projects;
    QMap<QString, QSet<QString> > _well_tags; // Per project tags
    QMap<QString, QSet<QString> > _grouped_tags;

    QMap<QString, QSet<QString > > _tags_of_tags;


private:
    QJsonDocument labcollector_strains, labcollector_chemicals;

    Ui::tagger *ui;
};

#endif // TAGGER_H
