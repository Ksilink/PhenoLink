#ifndef TAGGERPLATE_H
#define TAGGERPLATE_H

#include <QWidget>

namespace Ui {
class TaggerPlate;
}

class TaggerPlate : public QWidget
{
    Q_OBJECT

public:
    explicit TaggerPlate(QString _plate, QWidget *parent = nullptr);
    ~TaggerPlate();

    void setTags(QMap<QString, QSet<QString> >& data, QSet<QString>& othertags);

private slots:
    void on_setTags_clicked();
    void on_unsetTags_clicked();

    void on_plates_design_currentIndexChanged(const QString &arg1);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_treeWidget_customContextMenuRequested(const QPoint &pos);


private:
    Ui::TaggerPlate *ui;

    QString plate;

};

#endif // TAGGERPLATE_H
