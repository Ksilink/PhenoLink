#include "optioneditor.h"

#include <QSettings>
#include <QPushButton>
#include <QToolButton>
#include <QStandardPaths>
#include <QDebug>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <ctkWidgets/ctkPathListWidget.h>
#include <ctkWidgets/ctkPathListButtonsWidget.h>
#include <ctkWidgets/ctkCollapsibleGroupBox.h>
#include <ctkWidgets/ctkPathLineEdit.h>

#include <QFileIconProvider>

#include <QApplication>

PythonOptionEditor::PythonOptionEditor(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;

    //    QPushButton *closeButton = new QPushButton(tr("Close"));
    //    connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);

    mainLayout->addWidget(pythonCorePath());
    mainLayout->addWidget(pythonPluginsPath());

    QFormLayout* lay = new QFormLayout();
    QSettings set;
    init_script = new ctkPathLineEdit(
                "Python init script"               ,
                QStringList() << "Python (*.py)",
                ctkPathLineEdit::Files | ctkPathLineEdit::Readable);
    init_script->setCurrentPath(set.value("Python/InitScript", QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first() + "/checkout_init.py").toString() );
    lay->addRow("Python Init script", init_script);

    connect(init_script, SIGNAL(currentPathChanged(QString)), this, SLOT(updatePaths()));


    mainLayout->addLayout(lay);
    //    mainLayout->addWidget(closeButton);

    setLayout(mainLayout);
}

QWidget* PythonOptionEditor::pythonCorePath()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Python Path");

    pythonPath = new ctkPathListWidget;
    pythonPath->setMode(ctkPathListWidget::DirectoriesOnly);
    pythonPath->setDirectoryIcon(icp.icon(QFileIconProvider::Folder));


    QStringList data = set.value("Python/PythonPath",
                                 QStringList() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\Lib"
                                 <<   QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\DLLs"
                                 <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "\\Anaconda3\\Lib\\site-packages"
                                 ).toStringList();
    qDebug() << data;

    foreach (QString d, data)
    {
        QString absolutePath = QFileInfo(d).absoluteFilePath();
        qDebug() << QFileInfo(d).isDir() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
        qDebug() << d << absolutePath;
        if (!pythonPath->addPath(d))
            qDebug() << "Warning path not added";
    }
    QHBoxLayout *mainLayout = new QHBoxLayout;


    connect(pythonPath, SIGNAL(pathsChanged(QStringList,QStringList)), this, SLOT(updatePaths()));
    connect(pythonPath, SIGNAL(currentPathChanged(QString,QString)), this, SLOT(updatePaths()));


    ctkPathListButtonsWidget* pathEdit = new ctkPathListButtonsWidget();
    pathEdit->init(pythonPath);
    pathEdit->setOrientation(Qt::Vertical);

    pathEdit->setIconAddDirectoryButton(QIcon(":/plus.png"));

    pathEdit->setIconRemoveButton(QIcon(":/minus.png"));
    pathEdit->setIconEditButton(QIcon(":/edit.png"));

    mainLayout->addWidget(pythonPath);
    mainLayout->addWidget(pathEdit);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);

    ppython->setLayout(mainLayout);

    return ppython;
}

QWidget *PythonOptionEditor::pythonPluginsPath()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Python Plugins Path");


    pythonPluginPath = new ctkPathListWidget;
    pythonPluginPath->setMode(ctkPathListWidget::DirectoriesOnly);
    pythonPluginPath->setDirectoryIcon(icp.icon(QFileIconProvider::Folder));


    QStringList data = set.value("Python/PluginPath",
                                 QStringList() << qApp->applicationDirPath() + "/Python/")
            .toStringList();

    foreach (QString d, data)
    {
        QString absolutePath = QFileInfo(d).absoluteFilePath();
        //        qDebug() << QFileInfo(d).isDir() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
        //        qDebug() << d << absolutePath;
        if (!pythonPluginPath->addPath(d))
            qDebug() << "Warning path not added";
    }
    QHBoxLayout *mainLayout = new QHBoxLayout;


    connect(pythonPluginPath, SIGNAL(pathsChanged(QStringList,QStringList)), this, SLOT(updatePaths()));
    connect(pythonPluginPath, SIGNAL(currentPathChanged(QString,QString)), this, SLOT(updatePaths()));


    ctkPathListButtonsWidget* pathEdit = new ctkPathListButtonsWidget();
    pathEdit->init(pythonPluginPath);
    pathEdit->setOrientation(Qt::Vertical);

    pathEdit->setIconAddDirectoryButton(QIcon(":/plus.png"));

    pathEdit->setIconRemoveButton(QIcon(":/minus.png"));
    pathEdit->setIconEditButton(QIcon(":/edit.png"));

    mainLayout->addWidget(pythonPluginPath);
    mainLayout->addWidget(pathEdit);

    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);

    ppython->setLayout(mainLayout);

    return ppython;
}

void PythonOptionEditor::updatePaths()
{
    QStringList lcp =  pythonPath->paths();
    QStringList lpp = pythonPluginPath->paths();

    QSettings s;
    s.setValue("Python/PythonPath", lcp);
    s.setValue("Python/PluginPath", lpp);
    s.setValue("Python/InitScript", init_script->currentPath());
    init_script->addCurrentPathToHistory();

    //    qDebug() << "Options recorded";
}



#include <QtWidgets>


ConfigDialog::ConfigDialog()
{
    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(96, 84));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMaximumWidth(128);
    contentsWidget->setSpacing(12);

    pagesWidget = new QStackedWidget;

    pagesWidget->addWidget(new GlobalOptions);
    pagesWidget->addWidget(new SearchOptionEditor);
    pagesWidget->addWidget(new PythonOptionEditor);
    pagesWidget->addWidget(new CloudOptionEditor);

    QPushButton *closeButton = new QPushButton(tr("Close"));

    createIcons();
    contentsWidget->setCurrentRow(0);

    connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("PhenoLink Configuration options"));
}

void ConfigDialog::createIcons()
{
    QListWidgetItem *configButton = new QListWidgetItem(contentsWidget);
    configButton->setIcon(QIcon(":/configuration.png"));
    configButton->setText(tr("Configuration"));
    configButton->setTextAlignment(Qt::AlignHCenter);
    configButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *searchButton = new QListWidgetItem(contentsWidget);
    searchButton->setIcon(QIcon(":/search.png"));
    searchButton->setText(tr("Search Path"));
    searchButton->setTextAlignment(Qt::AlignHCenter);
    searchButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);


    QListWidgetItem *updateButton = new QListWidgetItem(contentsWidget);
    updateButton->setIcon(QIcon(":/python.png"));
    updateButton->setText(tr("Python Options"));
    updateButton->setTextAlignment(Qt::AlignHCenter);
    updateButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);


    QListWidgetItem *cloud = new QListWidgetItem(contentsWidget);
    cloud->setIcon(QIcon(":/upload_cloud.png"));
    cloud->setText(tr("Cloud Options"));
    cloud->setTextAlignment(Qt::AlignHCenter);
    cloud->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);


    //    QListWidgetItem *queryButton = new QListWidgetItem(contentsWidget);
    //    queryButton->setIcon(QIcon(":/images/query.png"));
    //    queryButton->setText(tr("Query"));
    //    queryButton->setTextAlignment(Qt::AlignHCenter);
    //    queryButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(contentsWidget, &QListWidget::currentItemChanged, this, &ConfigDialog::changePage);
}

void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    pagesWidget->setCurrentIndex(contentsWidget->row(current));
}


GlobalOptions::GlobalOptions(QWidget *parent): QWidget(parent)
{

    QVBoxLayout *mainLayout = new QVBoxLayout;

    //    QPushButton *closeButton = new QPushButton(tr("Close"));
    //    connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);

    mainLayout->addWidget(networkOptions());
    mainLayout->addWidget(dashOptions());
    mainLayout->addWidget(notebooksOptions());
    mainLayout->addWidget(features());
    mainLayout->addWidget(screensPaths());
    mainLayout->addWidget(appDirectory());

    setLayout(mainLayout);



}


//q.value("Image Flow/Count", _ncols).toInt();
//q.value("ImageWidget/Scale", .5)
//set.value("ScreensDirectory", QVariant(QStringList())).toStringList();

//   (set.value("UserMode/Advanced", false).toBool());
//    (set.value("UserMode/VeryAdvanced", false).toBool());
//    (set.value("UserMode/Debug", false).toBool())

//    QString pythonPath = sets.value("Python/PythonPath",
//                                    QString("C:\\Users\\admin\\Anaconda3\\Lib;C:\\Users\\admin\\Anaconda3\\DLLs;C:\\Users\\admin\\Anaconda3\\Lib\\site-package")).toString();

//    QStringList paths = sets.value("Python/PluginPath", QStringList() << qApp->applicationDirPath() + "/Python/").toStringList();




void GlobalOptions::ServGui(QString serv,  int port, QGridLayout* serv_layout)
{

    serverhost = new QLineEdit(serv);
    serverhost->setToolTip("Default: 127.0.0.1");
    connect(serverhost, SIGNAL(textChanged(QString)), this, SLOT(updatePaths()));

    serverPort = new QSpinBox();
    serverPort->setMinimum(0);
    serverPort->setMaximum(std::numeric_limits<unsigned short>::max());
    serverPort->setValue(port);
    serverPort->setToolTip("Default: 13555");
    connect(serverPort, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    serv_layout->addWidget(new QLabel("Server Host"), 0, 0);
    serv_layout->addWidget(serverhost, 0, 1);
    serv_layout->addWidget(new QLabel("Server Port"), 0, 2);
    serv_layout->addWidget(serverPort, 0, 3);
}

QWidget *GlobalOptions::features()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Global Options");

    ppython->setObjectName("Globalopts");


    QFormLayout* mainLayout = new QFormLayout;

    mainLayout->setObjectName("Global Option Layout");



    intensity_waitRate = new QSpinBox();
    intensity_waitRate->setMinimum(200);
    intensity_waitRate->setMaximum(10000);
    intensity_waitRate->setValue(set.value("RefreshSliderRate", 2000).toInt());
    intensity_waitRate->setToolTip("Default: 2000 (ms)");
    connect(intensity_waitRate, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    mainLayout->addRow("Intensity Rescale wait duration ", intensity_waitRate);



    intensity_refreshRatio = new QDoubleSpinBox();
    intensity_refreshRatio->setDecimals(2);
    intensity_refreshRatio->setMinimum(0.0);
    intensity_refreshRatio->setMaximum(0.5);
    intensity_refreshRatio->setValue(set.value("RefreshSliderRatio", 0.05).toDouble());
    intensity_refreshRatio->setToolTip("Default: 0.05 (* 100 %))");
    connect(intensity_refreshRatio, SIGNAL(valueChanged(double)), this, SLOT(updatePaths()));

    mainLayout->addRow("Intensity Margin (% of range intensity) ", intensity_refreshRatio);


    //    refreshRate = new QSpinBox();
    //    refreshRate->setMinimum(0);
    //    refreshRate->setMaximum(60000);
    //    refreshRate->setValue(set.value("RefreshRate", 300).toInt());
    //    refreshRate->setToolTip("Default: 300");
    //    connect(refreshRate, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    //    mainLayout->addRow("Process Query Refressh Rate ", refreshRate);


    minServerProcs = new QSpinBox();
    minServerProcs->setMinimum(0);
    minServerProcs->setMaximum(100000);
    minServerProcs->setValue(set.value("MinProcs", 2000).toInt());
    minServerProcs->setToolTip("Default: 2000");
    connect(minServerProcs, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    mainLayout->addRow("Minimum Process List Size ", minServerProcs);

    //    maxRefreshQuery = new QSpinBox();
    //    maxRefreshQuery->setMinimum(0);
    //    maxRefreshQuery->setMaximum(1000);
    //    maxRefreshQuery->setValue(set.value("maxRefreshQuery", 100).toInt());
    //    maxRefreshQuery->setToolTip("Default: 100");
    //    connect(maxRefreshQuery, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    //    mainLayout->addRow("Maximum states query", maxRefreshQuery);


    unpackScaling = new QDoubleSpinBox();
    unpackScaling->setMinimum(0.0001);
    unpackScaling->setMaximum(1);
    unpackScaling->setValue(set.value("unpackScaling", 1).toDouble());
    unpackScaling->setToolTip("Default: 1");
    connect(unpackScaling, SIGNAL(valueChanged(double)), this, SLOT(updatePaths()));

    mainLayout->addRow("Unpack Scaling factor", unpackScaling);



    ppython->setLayout(mainLayout);

    return ppython;
}


QWidget *GlobalOptions::networkOptions()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Networking options");

    ppython->setObjectName("NetworkOpts");

#ifdef WIN32
    QString usern = qgetenv("USERNAME");
#else
    QString usern = qgetenv("USER");
#endif

    QFormLayout* mainLayout = new QFormLayout;

    mainLayout->setObjectName("Option Main Layout");

    username = new QLineEdit(set.value("UserName", usern).toString());
    connect(username, SIGNAL(textChanged(QString)), this, SLOT(updatePaths()));
    mainLayout->addRow("User Name", username);

    QGridLayout* serv_layout = new QGridLayout;
    serv_layout->setObjectName("ServLayout");

    QString var = set.value("ZMQServer",  "127.0.0.1").toString();
    int ports = set.value("ZMQServerPort", 13555).toInt();

    ServGui(var, ports, serv_layout);

    //    QWidget* wid = new QWidget;
    //    wid->setLayout(serv_layout);

    //    mainLayout->addWidget(wid);
    mainLayout->addRow(serv_layout);

    ppython->setLayout(mainLayout);

    return ppython;
}


QWidget *GlobalOptions::dashOptions()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Graphics Synthesis (Dash) options");

    ppython->setObjectName("DashOpts");

    QGridLayout* serv_layout = new QGridLayout;
    serv_layout->setObjectName("DashLayout");
    //mainLayout->addRow(serv_layout);



    dashhost = new QLineEdit(set.value("DashServer","192.168.2.127").toString());
    dashhost->setObjectName("DashServer");
    dashhost->setToolTip("Address of the dash server to render the graphics (192.168.2.127)");
    connect(dashhost, SIGNAL(textChanged(QString)), this, SLOT(updatePaths()));
    serv_layout->addWidget(new QLabel("Dash Server Host"), 0, 0);
    serv_layout->addWidget(dashhost, 0, 1);
    ppython->setLayout(serv_layout);

    return ppython;
}

QWidget *GlobalOptions::notebooksOptions()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Jupyter Notebooks options");

    ppython->setObjectName("notebookOps");

    QGridLayout* serv_layout = new QGridLayout;
    serv_layout->setObjectName("nbksLayout");
    //mainLayout->addRow(serv_layout);



    jupyterhost = new QLineEdit(set.value("JupyterNotebook","192.168.2.127").toString());
    jupyterhost->setObjectName("JupyterNotebookServer");
    jupyterhost->setToolTip("Address of the Jupyter Notebook server to post process the data (192.168.2.127)");
    connect(jupyterhost, SIGNAL(textChanged(QString)), this, SLOT(updatePaths()));

    serv_layout->addWidget(new QLabel("Jupyter Notebook Server Host"), 0, 0);
    serv_layout->addWidget(jupyterhost, 0, 1);

    jupyterToken = new QLineEdit(set.value("JupyterToken","").toString());
    jupyterToken->setObjectName("JupyterNotebookToken");
    jupyterToken->setToolTip("Token to access the jupyter notebook");
    connect(jupyterToken, SIGNAL(textChanged(QString)), this, SLOT(updatePaths()));

    serv_layout->addWidget(new QLabel("Jupyter's Notebook Token"), 1, 0);
    serv_layout->addWidget(jupyterToken, 1, 1);

    ppython->setLayout(serv_layout);

    return ppython;
}

QWidget *GlobalOptions::searchOptions()
{
    return NULL;
}



QWidget *GlobalOptions::screensPaths()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("List of Screens Directory ");


    screensPath = new ctkPathListWidget;
    screensPath->setMode(ctkPathListWidget::DirectoriesOnly);
    screensPath->setDirectoryIcon(icp.icon(QFileIconProvider::Folder));


    QStringList data = set.value("ScreensDirectory",
                                 QStringList() << "")
            .toStringList();

    foreach (QString d, data)
    {
        QString absolutePath = QFileInfo(d).absoluteFilePath();
        //        qDebug() << QFileInfo(d).isDir() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
        //        qDebug() << d << absolutePath;
        if (!screensPath->addPath(d))
            qDebug() << "Warning path not added";
    }
    QHBoxLayout *mainLayout = new QHBoxLayout;


    (connect)(screensPath, SIGNAL(pathsChanged(QStringList,QStringList)), this, SLOT(updatePaths()));
    connect(screensPath, SIGNAL(currentPathChanged(QString,QString)), this, SLOT(updatePaths()));


    ctkPathListButtonsWidget* pathEdit = new ctkPathListButtonsWidget();
    pathEdit->init(screensPath);
    pathEdit->setOrientation(Qt::Vertical);

    pathEdit->setIconAddDirectoryButton(QIcon(":/plus.png"));

    pathEdit->setIconRemoveButton(QIcon(":/minus.png"));
    pathEdit->setIconEditButton(QIcon(":/edit.png"));

    mainLayout->addWidget(screensPath);
    mainLayout->addWidget(pathEdit);

    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);

    ppython->setLayout(mainLayout);

    return ppython;
}

QPushButton* GlobalOptions::button(QString fname, const QIcon& ico, bool open)
{
    QPushButton* but = new QPushButton();
    but->setIcon(ico);
    but->setToolTip(fname);
    but->setProperty("path", fname);

    if (!QFile(fname).exists() && open)
        but->setDisabled(true);


    if (open)
        connect(but, SIGNAL(clicked()), this, SLOT(openDirectory()));
    else
        connect(but, SIGNAL(clicked()), this, SLOT(copyDirectory()));

    return but;

}

QLayout* GlobalOptions::buildPaths(QString fname)
{
    QHBoxLayout* hl = new QHBoxLayout;

    QFileIconProvider icp;

    hl->addWidget(button(fname, icp.icon(QFileIconProvider::Folder), true));
    hl->addWidget(button(fname, QIcon(":/copy.png"), false));

    return hl;
}

QWidget *GlobalOptions::appDirectory()
{
    QSettings set;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Applications Installation paths");
    QFormLayout* mainLayout = new QFormLayout;

    mainLayout->addRow("Startup Python script",
                       buildPaths( set.value("Python/InitScript",
                                             QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first() + "/checkout_init.py").toString()));
    mainLayout->addRow("Core Application Log file",
                       buildPaths( QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first() + "/WD_CheckoutLog.txt"));
    mainLayout->addRow("Server Application Log file",
                       buildPaths( "c:/temp/CheckoutServer_log.txt"));


    databasePath = new ctkPathLineEdit(
                "Database Path"               ,
                QStringList() << "Dirs (*)",
                ctkPathLineEdit::Dirs | ctkPathLineEdit::Readable );
    databasePath->setCurrentPath(set.value("databaseDir").toString());
    mainLayout->addRow("Database Path", databasePath);

    connect(databasePath, SIGNAL(currentPathChanged(QString)), this, SLOT(updatePaths()));



    ppython->setLayout(mainLayout);

    return ppython;
}

void GlobalOptions::updatePaths()
{

    QSettings set;

    set.setValue("UserName", username->text());

    set.setValue("MinProcs", minServerProcs->value());
    //    set.setValue("RefreshRate", refreshRate->value());
    //    set.setValue("maxRefreshQuery", maxRefreshQuery->value());
    set.setValue("unpackScaling", unpackScaling->value());


    set.setValue("RefreshSliderRate", intensity_waitRate->value());
    set.setValue("RefreshSliderRatio", intensity_refreshRatio->value());


    //    this->parentWidget()->startTimer(refreshRate->value());


    QString var;
    int ports;
    QGridLayout* lay = findChild<QGridLayout*>("ServLayout");
    for (int i = 0; i < lay->rowCount(); ++i)
    {
        QLineEdit* ed = qobject_cast<QLineEdit*>(lay->itemAtPosition(i, 1)->widget());
        QSpinBox* sp = qobject_cast<QSpinBox*>(lay->itemAtPosition(i, 3)->widget());

        if (ed) var = ed->text();
        if (sp) ports = sp->value();
    }
    set.setValue("ZMQServer", var);
    set.setValue("ZMQServerPort", ports);


    if (dashhost)
        set.setValue("DashServer", dashhost->text());


    if (jupyterhost)
        set.setValue("JupyterNotebook", jupyterhost->text());

    if (jupyterToken)
        set.setValue("JupyterToken", jupyterToken->text());



    set.setValue("ScreensDirectory", screensPath->paths());
    set.setValue("databaseDir", databasePath->currentPath());
    //    qDebug() << "Saved options";
}


void GlobalOptions::openDirectory()
{
    QVariant var = sender()->property("path");

    if (var.isNull() || !var.canConvert<QString>()) return;


    showInGraphicalShell(this, var.toString());
}

void GlobalOptions::copyDirectory()
{
    QVariant var = sender()->property("path");

    if (var.isNull() || !var.canConvert<QString>()) return;

    QApplication::clipboard()->setText(var.toString());

}

void printParent(QObject* ob, int pos = 0)
{

    //    qDebug() << pos << ob->objectName();
    if (ob->parent())
        printParent(ob->parent(), pos + 1);
}


void GlobalOptions::showInGraphicalShell(QWidget *parent, const QString &pathIn)
{
    // Mac, Windows support folder or file.
#if defined(Q_OS_WIN)
    const QString explorer("explorer.exe");
    if (explorer.isEmpty()) {
        QMessageBox::warning(parent,
                             tr("Launching Windows Explorer failed"),
                             tr("Could not find explorer.exe in path to launch Windows Explorer."));
        return;
    }
    QStringList param;
    if (!QFileInfo(pathIn).isDir())
        param << QLatin1String("/select,");
    param << QDir::toNativeSeparators(pathIn);
    //    QString command = explorer + " " + param;
    QProcess::startDetached(explorer, param);
#elif defined(Q_OS_MAC)
    Q_UNUSED(parent)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
               << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                  .arg(pathIn);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
               << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
#else
#if FALSE
    // we cannot select a file here, because no file browser really supports it...
    const QFileInfo fileInfo(pathIn);
    const QString folder = fileInfo.absoluteFilePath();
    const QString app = Utils::UnixUtils::fileBrowser(Core::ICore::instance()->settings());
    QProcess browserProc;
    const QString browserArgs = Utils::UnixUtils::substituteFileBrowserParameters(app, folder);
    if (debug)
        qDebug() <<  browserArgs;
    bool success = browserProc.startDetached(browserArgs);
    const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
    success = success && error.isEmpty();
    if (!success)
        showGraphicalShellError(parent, app, error);
#endif
#endif
}

SearchOptionEditor::SearchOptionEditor(QWidget *parent)
{


    QVBoxLayout *mainLayout = new QVBoxLayout;

    mainLayout->addWidget(searchPath());

    setLayout(mainLayout);

}

QWidget *SearchOptionEditor::searchPath()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Plate Search Path");


    _searchPath = new ctkPathListWidget;
    _searchPath->setMode(ctkPathListWidget::DirectoriesOnly);
    _searchPath->setDirectoryIcon(icp.icon(QFileIconProvider::Folder));

    ctkPathListButtonsWidget* pathEdit = new ctkPathListButtonsWidget();
    pathEdit->init(_searchPath);
    pathEdit->setOrientation(Qt::Vertical);

    pathEdit->setIconAddDirectoryButton(QIcon(":/plus.png"));

    pathEdit->setIconRemoveButton(QIcon(":/minus.png"));
    pathEdit->setIconEditButton(QIcon(":/edit.png"));

    QStringList searchpaths = set.value("SearchPlate", QStringList() << "U:/BTSData/MeasurementData/"
                                        << "Z:/BTSData/MeasurementData/"
                                        << "W:/BTSData/MeasurementData/"
                                        << "K:/BTSData/MeasurementData/"
                                        << "O:/BTSData/MeasurementData/"
                                        << "C:/Data/").toStringList();

    foreach (QString d, searchpaths)
    {
        QString absolutePath = QFileInfo(d).absoluteFilePath();
        //        qDebug() << QFileInfo(d).isDir() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
        //        qDebug() << d << absolutePath;
        if (!_searchPath->addPath(d))
            qDebug() << "Warning search path not added";
    }
    connect(_searchPath, SIGNAL(pathsChanged(QStringList,QStringList)), this, SLOT(updatePaths()));
    connect(_searchPath, SIGNAL(currentPathChanged(QString,QString)), this, SLOT(updatePaths()));


    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(_searchPath);
    mainLayout->addWidget(pathEdit);

    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);

    ppython->setLayout(mainLayout);

    return ppython;
}

void SearchOptionEditor::updatePaths()
{
    QStringList sp = _searchPath->paths();

    QSettings s;
    s.setValue("SearchPlate", sp);

}

CloudOptionEditor::CloudOptionEditor(QWidget *parent)
{
    // Create the form layout
    formLayout = new QFormLayout;

    // Create the combo box to select the cloud provider
    providerComboBox_ = new QComboBox;
    providerComboBox_->addItem("AWS");
    providerComboBox_->addItem("GCS");
    providerComboBox_->addItem("Azure");

    // Connect the combo box to the slot to update the form fields
    connect(providerComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CloudOptionEditor::updateFormFields);

    // Create the form fields
    QList<QLineEdit*> connects;

    accessKeyEdit_ = new QLineEdit;
    secretKeyEdit_ = new QLineEdit;
    bucketEdit_ = new QLineEdit;
    keyEdit_ = new QLineEdit;
    connectionStringEdit_ = new QLineEdit;
    containerEdit_ = new QLineEdit;
    blobEdit_ = new QLineEdit;



    // Add the form fields to the layout
    formLayout->addRow("Cloud Provider:", providerComboBox_);
    formLayout->addRow("Access Key:", accessKeyEdit_);
    formLayout->addRow("Secret Key:", secretKeyEdit_);
    formLayout->addRow("Bucket:", bucketEdit_);
    formLayout->addRow("Key:", keyEdit_);
    formLayout->addRow("Connection String:", connectionStringEdit_);
    formLayout->addRow("Container:", containerEdit_);
    formLayout->addRow("Blob:", blobEdit_);

    // Create the group box to hold the form fields
    QGroupBox *formGroupBox = new QGroupBox("Cloud Storage Information");
    formGroupBox->setLayout(formLayout);

    // Create the main layout
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(formGroupBox);

    // Set the main layout
    setLayout(mainLayout);

    // Restore from the Save Settings
    loadSettings();

    // Update the form fields to show the fields for the selected provider
    updateFormFields();
    connects << accessKeyEdit_<<secretKeyEdit_<<bucketEdit_<<keyEdit_<<connectionStringEdit_<<containerEdit_<<blobEdit_;

    for (auto item : connects)
        connect(item, SIGNAL(textChanged(QString)), this, SLOT(saveSettings()));

}

QWidget *CloudOptionEditor::searchPath()
{
    return new QWidget();
}

void CloudOptionEditor::loadSettings()
{
    QSettings settings;

    qDebug() << "Load"
             << settings.value("CloudStorage/provider", "AWS")
             << settings.value("CloudStorage/accessKey")
             << settings.value("CloudStorage/secretKey")
             << settings.value("CloudStorage/bucket")
             << settings.value("CloudStorage/key")
             << settings.value("CloudStorage/connectionString")
             << settings.value("CloudStorage/container")
             << settings.value("CloudStorage/blob")
        ;

    providerComboBox_->setCurrentText(settings.value("CloudStorage/provider", "AWS").toString());
    accessKeyEdit_->setText(settings.value("CloudStorage/accessKey").toString());
    secretKeyEdit_->setText(settings.value("CloudStorage/secretKey").toString());
    bucketEdit_->setText(settings.value("CloudStorage/bucket").toString());
    keyEdit_->setText(settings.value("CloudStorage/key").toString());
    connectionStringEdit_->setText(settings.value("CloudStorage/connectionString").toString());
    containerEdit_->setText(settings.value("CloudStorage/container").toString());
    blobEdit_->setText(settings.value("CloudStorage/blob").toString());
}

void CloudOptionEditor::saveSettings()
{
    QSettings settings;

    settings.beginGroup("CloudStorage");
    settings.setValue("provider", providerComboBox_->currentText());
    settings.setValue("accessKey", accessKeyEdit_->text());
    settings.setValue("secretKey", secretKeyEdit_->text());
    settings.setValue("bucket", bucketEdit_->text());
    settings.setValue("key", keyEdit_->text());
    settings.setValue("connectionString", connectionStringEdit_->text());
    settings.setValue("container", containerEdit_->text());
    settings.setValue("blob", blobEdit_->text());
    settings.endGroup();

    qDebug() << "Save" << settings.value("CloudStorage/provider", "AWS")
             << settings.value("CloudStorage/accessKey")
             << settings.value("CloudStorage/secretKey")
             << settings.value("CloudStorage/bucket")
             << settings.value("CloudStorage/key")
             << settings.value("CloudStorage/connectionString")
             << settings.value("CloudStorage/container")
             << settings.value("CloudStorage/blob")
        ;

}

void CloudOptionEditor::updateFormFields()
{
    // Get the selected provider
    QString provider = providerComboBox_->currentText();

    // Show or hide the form fields based on the selected provider
    accessKeyEdit_->setVisible(provider == "AWS");
    formLayout->labelForField(accessKeyEdit_)->setVisible(provider == "AWS");

    secretKeyEdit_->setVisible(provider == "AWS");
    formLayout->labelForField(secretKeyEdit_)->setVisible(provider == "AWS");

    bucketEdit_->setVisible(provider == "GCS" || provider == "AWS");
    formLayout->labelForField(bucketEdit_)->setVisible(provider == "GCS" || provider == "AWS");

    keyEdit_->setVisible(provider == "GCS");
    formLayout->labelForField(keyEdit_)->setVisible(provider == "GCS");

    connectionStringEdit_->setVisible(provider == "Azure");
    formLayout->labelForField(connectionStringEdit_)->setVisible(provider == "Azure");

    containerEdit_->setVisible(provider == "Azure");
    formLayout->labelForField(containerEdit_)->setVisible(provider == "Azure");

    blobEdit_->setVisible(provider == "Azure");
    formLayout->labelForField(blobEdit_)->setVisible(provider == "Azure");

    saveSettings();
}
