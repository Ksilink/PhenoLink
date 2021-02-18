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
    explicit DashOptions(Screens screens, bool notebook, QWidget *parent = nullptr);
    ~DashOptions();

private:
    void populateDataset();
    void notebookAdapt();

private slots:

    void on_pushButton_2_clicked();

    void on_next_clicked();

    void on_toolBox_currentChanged(int index);

private: // result data
    QMap<QString, DataHolder> filemapper;
    QStringList projects; // List of project for post-processing selection

private: // class init options

    Ui::DashOptions *ui;
    Screens _screens;
    bool _notebook;
};

#endif // DASHOPTIONS_H
