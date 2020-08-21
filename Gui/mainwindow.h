#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Core/sequenceinteractor.h>

#include "screensmodel.h"

#include "Core/Dll.h"
#include <QComboBox>

namespace Ui {
class MainWindow;
}

extern DllGuiExport QFile _logFile;

class ImageForm;
class QSqlTableModel;
class ScreensModel;
class QTableWidget;
class QCheckBox;
class ScrollZone;
class QTreeWidget;
class QTableView;
class ctkColorPickerButton;
class ctkDoubleRangeSlider;
class QDoubleSpinBox;

class QHBoxLayout;
class ctkPopupWidget;
class CheckoutCorePythonInterface;
class QWinTaskbarProgress;


#include "experimentworkbenchcontrol.h"

class DllGuiExport MainWindow : public QMainWindow
{
    Q_OBJECT

    QProcess* server;
public:
    explicit MainWindow(QProcess* serverProc, QWidget *parent = 0);
    ~MainWindow();

    void getValue(QWidget *wid, QJsonObject &obj, QString tag, bool list=false);

    void adaptSelection(QTableView* tw, QItemSelectionModel* sm, QSet<QString> &rs);

    void startProcessOtherStates(QList<bool> selectedChanns, QList<SequenceFileModel*> lsfm,
                                 bool started, QMap<QString, QSet<QString> > tags_map);
    void on_actionRe_load_servers_triggered();

protected:
    template <typename WType>
    int getWidgetWidth(QString name)
    {
        int max = 0;
        QList<WType> c = findChildren<WType>(name);
        foreach (WType s, c)
        if (s)
        {
            max = std::max(max, s->minimumWidth());
        }
        return max;

    }
    template <typename WType>
    void setWidgetWidth(QString name, int width)
    {
        QList<WType> c = findChildren<WType>(name);
        foreach (WType s, c)
        if (s)
        {
            s->setMinimumWidth(width);
        }
    }


    virtual void timerEvent(QTimerEvent * event);
    void createWellPlateViews(ExperimentFileModel *data);
//    QTableWidget* getDataTableWidget(QString hash);

    QTableView* getDataTableView(ExperimentFileModel *mdl);
    QTableView* getDataTableView(QString hash);
    QString getDataHash(QJsonObject data);

    QJsonArray startProcess(SequenceFileModel *sfm, QJsonObject obj, QList<bool> selectedChanns);
    void refreshExperimentControl(QTreeWidget *l, ExperimentFileModel *mdl);
    QDoubleSpinBox* constructDoubleSpinbox(QHBoxLayout* popupLayout, QWidget *popup, ImageInfos* fo, QString objName, QString text);

    void setupPython();


    ctkDoubleRangeSlider *RangeWidgetSetup(ctkDoubleRangeSlider *w, ImageInfos *fo, int channel, bool reconnect = false);
    ctkColorPickerButton * colorWidgetSetup(ctkColorPickerButton *w, ImageInfos *fo, int channel, bool reconnect = false);
    QDoubleSpinBox *setupMinMaxRanges(QDoubleSpinBox* extr, ImageInfos* fo, QString text, bool isMin, bool reconnect = false);
    QCheckBox *setupActiveBox(QCheckBox *box, ImageInfos *fo, int channel, bool reconnect = false);
    QWidget *widgetFromJSON(QJsonObject &par);


	void conditionChanged(QWidget* sen, int val);

private slots:
    void deleteDirectoryPath();
    void on_loadSelection_clicked();
    void addDirectory();
    void deleteDirPath(QString dir);
    void loadSelection(QStringList checked);
    void on_actionOpen_Single_Image_triggered();
    void displayWellSelection();
    void addDirectoryName(QString name);

    void on_action_Exit_triggered();
    void on_toolButton_clicked();

    void on_tabWidget_customContextMenuRequested(const QPoint &pos);
    void on_treeView_customContextMenuRequested(const QPoint &pos);
    void on_wellPlateViewTab_tabBarClicked(int index);
    void on_actionPick_Intensities_toggled(bool arg1);

    void setDataIcon();
    void databaseModified();
    void on_actionStandard_toggled(bool arg1);
    void on_actionAdvanced_User_toggled(bool arg1);
    void on_actionDebug_toggled(bool arg1);
    void on_actionVery_Advanced_toggled(bool arg1);
    void on_actionPython_Core_triggered();
    void on_actionOptions_triggered();
    void exec_pythonCommand();
    void addExperimentWorkbench();
    void on_tabWidget_currentChanged(int index);

    void on_wellSelectionChanged();

    bool close();

    void on_actionRe_start_Server_triggered();

    void changeColorState(QString link);

	QJsonArray sortParameters(QJsonArray& arr);

    void startProcessRun();

public slots:

    void updateCurrentSelection();
    void prepareProcessCall();
    void setupProcessCall(QJsonObject obj);
    void startProcess();
    void changeLayout(int value);

    void refreshProcessMenu();

    void conditionChanged(int val);


    void processStarted(QString hash);

    void server_processError(QProcess::ProcessError error);
    void server_processFinished(int exitCode, QProcess::ExitStatus status);

    void processFinished();
    void on_computeFeatures_currentChanged(int index);

    void wellplateClose(int tabId);
    void removeWorkbench(int idx);

    void networkProcessFinished(QJsonObject ob);
    void future_networkRetrieveImageFinished();

    void networkRetrievedImage(QList<SequenceFileModel*> lsfm);


    void rangeChange(double mi, double ma);
    void changeRangeValueMax(double val);
    void changeRangeValueMin(double val);

    void commitTableToDatabase();
    void clearTable();

    void setProgressBar();
    void on_actionNo_network_toggled(bool arg1);

protected slots:
    void updateProcessStatusMessage(QJsonArray ob);

    void addImageWorkbench();

    void datasetCustomMenu(const QPoint &pos);
    void copyDataToClipBoard();
    void exportData();
    void selectDataInTables(); // According to selected wells in different views highlight lines accordingly in the table view

    void addImageFromPython(QStringList);

    void retrievedPayload(QString hash);
    void setColor(QColor c);
    void active_Channel(bool c);
private:

    bool networking;

    QString _logData; // holds data for text logs

    QMap<QWidget*, QMap<int, QList< QWidget*> > > _enableIf;
    QMap<QWidget*, QList<QWidget*> > _disable;

    QMap<QWidget*, ExperimentFileModel* > assoc_WellPlateWidget;

    QMap<QString, QWidget*> _imageControls;

    QMap<QString, int> mapValues;

    Ui::MainWindow *ui;

    ScrollZone* _scrollArea;

//    QSqlTableModel* _mdl;             // Object for representation of the stored data

    ScreensModel *mdl;               // Object for Data representation
    SequenceInteractor _sinteractor; // variable keeping track of the interaction with sequences
    QString    _preparedProcess; // Name of the current process
    QComboBox* _typeOfprocessing; // Keep tracks of the user selected type of processing (current image, current well, all screens, etc...)
    QLineEdit* _commitName;       // If non empty shall be used to commit the data to a database

    QCheckBox* _shareTags;

    QModelIndex _icon_model;    // Variable used to keep track of the model for which we want to change the icon

//    ImageForm* _images;       // Previ

//    unsigned _numberOfChannels;
//    bool _startingProcesses; // Flag to keep track of process that are awaiting a start

    QList<QCheckBox*> _ChannelPicker;  // keep track of visual selection of channels
    QSet<int> _channelsIds; // To keep track of the number of channels loaded & their respective true "value" i.e. C1/C2, etc...

    CheckoutCorePythonInterface* _python_interface;

    QList<ExperimentFileModel*> _forcedImages;

    QMap<QString, QJsonObject> _waitingForImages;

//    QWinTaskbarProgress *_progress;
    QProgressBar* _StatusProgress;
    int StartId;

    QSet<QFutureWatcher<void>*> _watchers;

};



DllGuiExport QWidget*   widgetFromJSON(QJsonObject& par);

#endif // MAINWINDOW_H
