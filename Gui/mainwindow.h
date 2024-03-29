#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Core/sequenceinteractor.h>

#include "screensmodel.h"

#include "Core/Dll.h"
#include <QComboBox>
#include <guiserver.h>

namespace Ui {
class PhenoLink;
}

extern DllGuiExport QFile _logFile;

class ImageForm;
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
class QCloseEvent;
class QShortcut;
class QPushButton;
class QListWidget;
class OverlayFilter;


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
                                 bool started, QRegularExpression siteMatcher, QString exports);

    void on_actionRe_load_servers_triggered();


    QString workbenchKey();


    void updateTableView(ExperimentFileModel *mdl);
    void displayWellSelection();

    SequenceInteractor* getInteractor(SequenceFileModel* mdl);
    Screens loadSelection(QStringList checked, bool reload=true);

    Screens findPlate(QString plate, QStringList project, QString drive);

    void clearScreenSelection();

    void resetSelection();
    void graySelection();
    void ungraySelection();

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

    QJsonArray startProcess(SequenceFileModel *sfm, QJsonObject obj, QList<bool> selectedChanns, QRegularExpression siteMatcher);
    void refreshExperimentControl(QTreeWidget *l, ExperimentFileModel *mdl);
    QDoubleSpinBox* constructDoubleSpinbox(QHBoxLayout* popupLayout, QWidget *popup, ImageInfos* fo, QString objName, QString text);

    void setupPython();

    QDoubleSpinBox *setupOverlayWidth(QDoubleSpinBox *box, QString itemName, ImageInfos *ifo, bool reconnect = false);
    ctkDoubleRangeSlider *RangeWidgetSetup(ctkDoubleRangeSlider *w, ImageInfos *fo, int channel, bool reconnect = false);
    ctkColorPickerButton * colorWidgetSetup(ctkColorPickerButton *w, ImageInfos *fo, int channel, bool reconnect = false);
    QDoubleSpinBox *setupMinMaxRanges(QDoubleSpinBox* extr, ImageInfos* fo, QString text, bool isMin, bool reconnect = false);
    QCheckBox *setupActiveBox(QCheckBox *box, ImageInfos *fo, int channel, bool reconnect = false);
    QDoubleSpinBox *setupVideoFrameRate(QDoubleSpinBox* extr, QString text);
    QWidget *widgetFromJSON(QJsonObject &par, bool reloaded = false);


    QCheckBox *setupOverlayBox(QCheckBox* box, QString itemName, ImageInfos* inter, bool reconnect = false);
    QSpinBox *setupTilePosition(QSpinBox* box, QString itemName, ImageInfos* inter, bool reconnect = false);

    QComboBox *setupOverlaySelection(QComboBox* box, QString itemName, ImageInfos *ifo, bool reconnect = false);
    QComboBox *setupOverlayColor(QComboBox *box, QString itemName, ImageInfos *ifo, bool reconnect = false);


    void conditionChanged(QWidget* sen, int val);
    void closeEvent(QCloseEvent *ev);

    void overlayClearTags();

    QJsonArray& adjustParameterFromWidget(SequenceFileModel *sfm, QJsonObject& im, QJsonArray& params, QJsonArray& bias);
private slots:
    void deleteDirectoryPath();
    void on_loadSelection_clicked();

    void on_dashDisplay_clicked();
    void on_notebookDisplay_clicked();


    void addDirectory();
    void clearDirectories();
    void deleteDirPath(QString dir);
    void on_actionOpen_Single_Image_triggered();
    void addDirectoryName(QString name);

    void cloudUpload();


    void on_action_Exit_triggered();
    void on_toolButton_clicked();

    void on_tabWidget_customContextMenuRequested(const QPoint &pos);
    void on_treeView_customContextMenuRequested(const QPoint &pos);
    void on_actionPick_Intensities_toggled(bool arg1);

    void setDataIcon();
    void rmDirectory();
    void loadPlate();
    void loadPlateFirst();
    void loadPlateDisplay3();
    void loadPlateDisplaySample();
    void exportToCellProfiler();
    void exportForAWS();

    void databaseModified();
    void on_actionStandard_toggled(bool arg1);
    void on_actionAdvanced_User_toggled(bool arg1);
    void on_actionDebug_toggled(bool arg1);
    void on_actionVery_Advanced_toggled(bool arg1);
    void on_actionPython_Core_triggered();
    void on_actionOptions_triggered();
    void exec_pythonCommand();

    void createBirdView();
    void addExperimentWorkbench();

    void on_tabWidget_currentChanged(int index);

    void on_wellSelectionChanged();

    bool close();

    void on_actionRe_start_Server_triggered();

    void changeColorState(QString link);

    QJsonArray sortParameters(QJsonArray& arr);

    void startProcessRun(QString exp=QString());
    void channelCheckboxMenu(const QPoint&);

    void on_sync_fields_toggled(bool arg1);

    void on_sync_zstack_toggled(bool arg1);

    void on_start_process_triggered();

    void on_prevOverlay_clicked();
    void on_nextOverlay_clicked();

    void on_addTag_clicked();
    void on_delTag_clicked();
    void on_actionDisplay_Remaining_Processes_triggered();


    void on_actionDisplay_Channel_Names_toggled(bool arg1);

    void on_filterOverlay_clicked();

    void on_actionAlways_triggered(bool checked);

    void on_actionNever_triggered(bool checked);

    void on_actionShare_Intensity_Controls_toggled(bool arg1);

    void on_dockWidget_visibilityChanged(bool visible);

    void on_actionOverlay_controls_triggered(bool checked);

    void on_actionDisplay_Field_Infos_toggled(bool arg1);

public slots:

    void updateCurrentSelection();
    void prepareProcessCall();
    void setupProcessCall(QJsonObject obj, int idx = -1);
    void startProcess();
    void changeLayout(int value);

    void refreshProcessMenu();

    void conditionChanged(int val);


//    void processStarted(QString hash);
    void on_wellPlateViewTab_tabBarClicked(int index);

    void server_processError(QProcess::ProcessError error);
    void server_processFinished(int exitCode, QProcess::ExitStatus status);

    void processFinished();
    void on_computeFeatures_currentChanged(int index);

    void wellplateClose(int tabId);
    void removeWorkbench(int idx);

    void networkProcessFinished(QJsonArray ob);
    void future_networkRetrieveImageFinished();

    void networkRetrievedImage(QList<SequenceFileModel*> lsfm);

    void on_pluginhistory(QString selection);
    void pluginHistory(QComboBox*);

    void rangeChange(double mi, double ma);

    void udpateRange(double mi, double ma);
    void changeRangeValueMax(double val);
    void changeRangeValueMin(double val);
    void changeFpsValue(double val);

    void commitTableToDatabase();
    void clearTable();

    void setProgressBar();
    void on_actionNo_network_toggled(bool arg1);

    void overlayChanged(QString id);
    void overlayChangedCmap(QString id);

    void finishedJob(int nb);

    void setOverlayWidth(double w);
    void renameWorkbench();
    void exportContent();
    void importOverlay();
    void exportOverlay();

    void finishedEditMinValue();
    void finishedEditMaxValue();

    void overlay_selection(const QString &text);

    void change_overlay_details(QString, QString, int);

    void on_clearTags_clicked();
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


    void displayTile(bool disp);
    void setTile( int tile);


    void plateMap();

    void button_load_database();

private:

    bool networking;

    QString _logData; // holds data for text logs

    QMap<QWidget*, QMap<int, QList< QWidget*> > > _enableIf;
    QMap<QWidget*, QList<QWidget*> > _disable;

    QMap<QWidget*, ExperimentFileModel* > assoc_WellPlateWidget;

    QMap<QString, QList<QWidget*> > _imageControls;

    QMap<QString, int> mapValues;

    Ui::PhenoLink *ui;

    ScrollZone* _scrollArea;


    ScreensModel *mdl;               // Object for Data representation
    SequenceInteractor _sinteractor; // variable keeping track of the interaction with sequences
    QString    _preparedProcess; // Name of the current process
    QJsonObject _processParams;


    QComboBox* _typeOfprocessing, *_history; // Keep tracks of the user selected type of processing (current image, current well, all screens, etc...)
    QLineEdit* _commitName;       // If non empty shall be used to commit the data to a database


    QModelIndex _icon_model;    // Variable used to keep track of the model for which we want to change the icon

    QList<QCheckBox*> _ChannelPicker;  // keep track of visual selection of channels
    QSet<int> _channelsIds; // To keep track of the number of channels loaded & their respective true "value" i.e. C1/C2, etc...
    QMap<int, QString> _channelsNames;

    CheckoutCorePythonInterface* _python_interface;

    QList<ExperimentFileModel*> _forcedImages;

    QMap<QString, QJsonObject> _waitingForImages;

    QProgressBar* _StatusProgress;
    QPushButton* _cancelation;

    int StartId;

    QSet<QFutureWatcher<void>*> _watchers;

    GuiServer _gui_server;

    QElapsedTimer run_time;

    QMap<QString , QDoubleSpinBox*> _syncmapper;


    QShortcut *shrt_startR, *shrt_startEnt;
    QVector<QShortcut *> shrt_binarize;


    QDateTime process_starttime;

    QPushButton* load_data;


    QDockWidget* overlay_filter_dock;
    OverlayFilter* overlay_filter;
//    QStringList overlay_filter_list;//, overlay_filter_and, overlay_filter_not;

    int overlay_t, overlay_f, overlay_z, overlay_c;

};



DllGuiExport QWidget*   widgetFromJSON(QJsonObject& par);

#endif // MAINWINDOW_H
