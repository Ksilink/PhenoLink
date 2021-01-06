#ifndef DASHOPTIONS_H
#define DASHOPTIONS_H

#include <QWidget>

#include <QDialog>

#include <Core/wellplatemodel.h>

namespace Ui {
class DashOptions;
}


struct DataHolder
{
    QString name;

    QStringList deactivated_well;

    // Columns & Tags selection + renamming + normalization feature if any
    QStringList columns, mapped_columns, normalize_columns;
    QStringList tags, mapped_tags, normalize_tags;

    // Now let's keep some room for relative normalization assignements

};


class DashOptions : public QDialog
{
    Q_OBJECT

public:
    explicit DashOptions(Screens screens, QWidget *parent = nullptr);
    ~DashOptions();

private slots:
    void on_pushButton_2_clicked();

    void on_next_clicked();

    void on_toolBox_currentChanged(int index);

private:
    QMap<QString, DataHolder> filemapper;

    Ui::DashOptions *ui;
    Screens _screens;
};

#endif // DASHOPTIONS_H
