#ifndef TAGGERPLATE_H
#define TAGGERPLATE_H

#include <QWidget>
#include <QSortFilterProxyModel>
#include <QJsonDocument>

namespace Ui {
class TaggerPlate;
}

class TaggerPlate : public QWidget
{
    Q_OBJECT

public:
    explicit TaggerPlate(QString _plate, QWidget *parent = nullptr);
    ~TaggerPlate();
    QJsonDocument& getTags();

    void setTags(QMap<QString, QSet<QString> >& data, QSet<QString>& othertags);

    void setTag(int r, int c, QString tags);
    void setColor(int r, int c, QString color);

    void updatePlate();


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

    QString plate;
    QSortFilterProxyModel* mdl;

    QJsonDocument tagger;

};



#endif // TAGGERPLATE_H
