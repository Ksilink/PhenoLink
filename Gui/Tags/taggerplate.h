#ifndef TAGGERPLATE_H
#define TAGGERPLATE_H

#include <QWidget>
#include <QSortFilterProxyModel>
#include <QJsonObject>

namespace Ui {
class TaggerPlate;
}

class TaggerPlate : public QWidget
{
    Q_OBJECT

public:
    explicit TaggerPlate(QString _plate, QWidget *parent = nullptr);
    ~TaggerPlate();
    QJsonObject &getTags();

    void setPlateAcq(QString pla, QString pl)
    {
        plate = pl;
        plateDate = pla;
    }

    void setPath(QString p)
    {
        path = p;
    }

    void setCategories(QMap<QString, QStringList> map);


    void setTags(
            QMap<QString, QMap<QString, QSet<QString > > > &data,
            QMap<QString, QSet<QString> >  &othertags,
            QString project);



    void setTag(int r, int c, QString tags);
    void setColor(int r, int c, QString color);
    void setPattern(int r, int c, int patt);
    void setColorFG(int r, int c, QString color);
    void updatePlate();
    QJsonObject refreshJson();
    QString getPlate()  { return plate; }


    void apply_template(QString script);

private slots:
    void on_checkBox_toggled(bool checked);

private slots:
    void on_setTags_clicked();
    void on_unsetTags_clicked();

    void on_plates_design_currentIndexChanged(const QString &arg1);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_treeView_customContextMenuRequested(const QPoint &pos);


    void on_lineEdit_textChanged(const QString &arg1);

    void on_plateMaps_customContextMenuRequested(const QPoint &pos);

private:
    Ui::TaggerPlate *ui;


    QString plateDate, plate, path;
    QSortFilterProxyModel* mdl, *projmdl;

    QJsonObject tagger;

};



#endif // TAGGERPLATE_H
