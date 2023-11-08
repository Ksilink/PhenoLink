#ifndef OPTIONEDITOR_H
#define OPTIONEDITOR_H

#include <QDialog>
#include "Core/Dll.h"
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>

class ctkPathListWidget;
class ctkPathLineEdit;
class QGridLayout;
class QFormLayout;

class DllGuiExport GlobalOptions: public QWidget
{
    Q_OBJECT
public:
    GlobalOptions(QWidget* parent = 0);
    void ServGui(QString var, int ports, QGridLayout* serv_layout);

protected:
    QWidget *features();
    QWidget *networkOptions();
    QWidget *screensPaths();
    QWidget *dashOptions();
    QWidget *notebooksOptions();
    QWidget *searchOptions();

    QWidget* appDirectory();
    QLayout *buildPaths(QString fname);
    QPushButton *button(QString fname, const QIcon &ico, bool open);
    void showInGraphicalShell(QWidget *parent, const QString &pathIn);



public slots:

    void updatePaths();

    void openDirectory();
    void copyDirectory();

protected:
    QLineEdit* username;
    QLineEdit* serverhost;

    QLineEdit* dashhost;
    QLineEdit* jupyterhost;
    QLineEdit* jupyterToken;


    QSpinBox*    intensity_waitRate;
    QDoubleSpinBox* intensity_refreshRatio;


//    QSpinBox* refreshRate;
//    QSpinBox* maxRefreshQuery;
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


class DllGuiExport SearchOptionEditor : public QWidget
{
    Q_OBJECT
public:
    explicit SearchOptionEditor(QWidget *parent = 0);

protected:
    QWidget *searchPath();
signals:

public slots:

    void updatePaths();

protected:
    ctkPathListWidget* _searchPath;
};



class DllGuiExport CloudOptionEditor : public QWidget
{
    Q_OBJECT
public:
    explicit CloudOptionEditor(QWidget *parent = 0);

protected:
    QWidget *searchPath();
public slots:
    void loadSettings();
    void saveSettings();
signals:


public slots:

    void updateFormFields();
public:

//    QString getProvider() const {
//        return providerComboBox_->currentText();
//    }

//    QString getAccessKey() const {
//        return accessKeyEdit_->text();
//    }

//    QString getSecretKey() const {
//        return secretKeyEdit_->text();
//    }

//    QString getBucket() const {
//        return bucketEdit_->text();
//    }

//    QString getKey() const {
//        return keyEdit_->text();
//    }

//    QString getConnectionString() const {
//        return connectionStringEdit_->text();
//    }

//    QString getContainer() const {
//        return containerEdit_->text();
//    }

//    QString getBlob() const {
//            return blobEdit_->text();
//        }

protected:
    QFormLayout *formLayout;
    QComboBox *providerComboBox_;
    QLineEdit *accessKeyEdit_;
    QLineEdit *secretKeyEdit_;
    QLineEdit *bucketEdit_;
    QLineEdit *keyEdit_;
    QLineEdit *connectionStringEdit_;
    QLineEdit *containerEdit_;
    QLineEdit *blobEdit_;
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
