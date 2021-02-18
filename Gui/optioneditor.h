#ifndef OPTIONEDITOR_H
#define OPTIONEDITOR_H

#include <QDialog>
#include "Core/Dll.h"
#include <QLineEdit>
#include <QSpinBox>

class ctkPathListWidget;
class ctkPathLineEdit;
class QGridLayout;

class DllGuiExport GlobalOptions: public QWidget
{
    Q_OBJECT
public:
    GlobalOptions(QWidget* parent = 0);
    void ServGui(QStringList var, int i, QList<QVariant> ports, QGridLayout* serv_layout, bool isServ=false);

protected:
    QWidget *features();
    QWidget* networkOptions();
    QWidget* screensPaths();
    QWidget *dashOptions();
    QWidget *notebooksOptions();

    QWidget* appDirectory();
    QLayout *buildPaths(QString fname);
    QPushButton *button(QString fname, const QIcon &ico, bool open);
    void showInGraphicalShell(QWidget *parent, const QString &pathIn);



public slots:

    void updatePaths();

    void openDirectory();
    void copyDirectory();

    void addServer();
    void delServer();

protected:
    QLineEdit* username;
    QLineEdit* serverhost;

    QLineEdit* dashhost;
    QLineEdit* jupyterhost;
    QLineEdit* jupyterToken;


    QSpinBox* refreshRate;
    QSpinBox* maxRefreshQuery;
    QSpinBox* minServerProcs;
    QDoubleSpinBox* unpackScaling;

    QSpinBox* serverPort;
    ctkPathListWidget* screensPath;
    ctkPathLineEdit* databasePath;


};




class DllGuiExport PythonOptionEditor : public QWidget
{
    Q_OBJECT
public:
    explicit PythonOptionEditor(QWidget *parent = 0);

protected:
    QWidget *pythonCorePath();
    QWidget *pythonPluginsPath();
signals:

public slots:

    void updatePaths();


protected:
    ctkPathListWidget* pythonPath;
    ctkPathListWidget* pythonPluginPath;
    ctkPathLineEdit* init_script;
};


class QListWidget;
class QListWidgetItem;
class QStackedWidget;

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog();

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void createIcons();

    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;
};

#endif // OPTIONEDITOR_H
