#include "optioneditor.h"

#include <QSettings>
#include <QPushButton>
#include <QToolButton>
#include <QStandardPaths>
#include <QDebug>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
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
    init_script->setCurrentPath(set.value("Python/InitScript", QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/checkout_init.py").toString() );
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
        qDebug() << QFileInfo(d).isDir() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
        qDebug() << d << absolutePath;
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
    pagesWidget->addWidget(new PythonOptionEditor);

    //    pagesWidget->addWidget(new QueryPage);

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

    setWindowTitle(tr("Checkout Configuration options"));
}

void ConfigDialog::createIcons()
{
    QListWidgetItem *configButton = new QListWidgetItem(contentsWidget);
    configButton->setIcon(QIcon(":/configuration.png"));
    configButton->setText(tr("Configuration"));
    configButton->setTextAlignment(Qt::AlignHCenter);
    configButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *updateButton = new QListWidgetItem(contentsWidget);
    updateButton->setIcon(QIcon(":/python.png"));
    updateButton->setText(tr("Python Options"));
    updateButton->setTextAlignment(Qt::AlignHCenter);
    updateButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

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




void GlobalOptions::ServGui(QStringList var, int i,
                            QList<QVariant> ports, QGridLayout* serv_layout,
                            bool isServ)
{
    QString serv = var.at(i);
    int port = ports.at(i).toInt();

    serverhost = new QLineEdit(serv);
    serverhost->setToolTip("Default: 127.0.0.1");
    connect(serverhost, SIGNAL(textChanged(QString)), this, SLOT(updatePaths()));

    serverPort = new QSpinBox();
    serverPort->setMinimum(0);
    serverPort->setMaximum(std::numeric_limits<unsigned short>::max());
    serverPort->setValue(port);
    serverPort->setToolTip("Default: 13378");
    connect(serverPort, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    serv_layout->addWidget(new QLabel("Server Host"), i, 0);
    serv_layout->addWidget(serverhost, i, 1);
    serv_layout->addWidget(new QLabel("Server Port"), i, 2);
    serv_layout->addWidget(serverPort, i, 3);

    if (i > 0)
    {
        QToolButton* but = new QToolButton;
        but->setText("-");
        but->setObjectName(QString("%1").arg(i));
        connect(but, SIGNAL(clicked(bool)), this, SLOT(delServer()));
        serv_layout->addWidget(but, i, 4);
    }

    if (isServ)
    {
        QToolButton* but = new QToolButton;
        but->setText("+");
        but->setObjectName(QString("%1").arg(i));
        connect(but, SIGNAL(clicked(bool)), this, SLOT(addServer()));

        serv_layout->addWidget(but, i, 5);
    }
}

QWidget *GlobalOptions::features()
{
    QSettings set;
    QFileIconProvider icp;

    ctkCollapsibleGroupBox* ppython = new ctkCollapsibleGroupBox("Global Options");

    ppython->setObjectName("Globalopts");


    QFormLayout* mainLayout = new QFormLayout;

    mainLayout->setObjectName("Global Option Layout");

    refreshRate = new QSpinBox();
    refreshRate->setMinimum(0);
    refreshRate->setMaximum(60000);
    refreshRate->setValue(set.value("RefreshRate", 300).toInt());
    refreshRate->setToolTip("Default: 300");
    connect(refreshRate, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    mainLayout->addRow("Process Query Refressh Rate ", refreshRate);



    minServerProcs = new QSpinBox();
    minServerProcs->setMinimum(0);
    minServerProcs->setMaximum(100000);
    minServerProcs->setValue(set.value("MinProcs", 20).toInt());
    minServerProcs->setToolTip("Default: 20");
    connect(minServerProcs, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    mainLayout->addRow("Minimum Process List Size ", minServerProcs);

    maxRefreshQuery = new QSpinBox();
    maxRefreshQuery->setMinimum(0);
    maxRefreshQuery->setMaximum(1000);
    maxRefreshQuery->setValue(set.value("maxRefreshQuery", 100).toInt());
    maxRefreshQuery->setToolTip("Default: 100");
    connect(maxRefreshQuery, SIGNAL(valueChanged(int)), this, SLOT(updatePaths()));

    mainLayout->addRow("Maximum states query", maxRefreshQuery);



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

    QStringList var = set.value("Server", QStringList() << "127.0.0.1").toStringList();
    QList<QVariant> ports = set.value("ServerP", QList<QVariant>() << QVariant((int)13378)).toList();

    for (int i = 0; i < var.size(); ++i)
    {
        ServGui(var, i, ports, serv_layout, i == (var.size()-1));
    }

    //    QWidget* wid = new QWidget;
    //    wid->setLayout(serv_layout);

    //    mainLayout->addWidget(wid);
    mainLayout->addRow(serv_layout);

    ppython->setLayout(mainLayout);

    return ppython;
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
        qDebug() << QFileInfo(d).isDir() <<  QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
        qDebug() << d << absolutePath;
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
                       buildPaths( set.value("Python/InitScript", QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/checkout_init.py").toString()));
    mainLayout->addRow("Core Application Log file",
                       buildPaths( QStandardPaths::standardLocations(QStandardPaths::DataLocation).first() + "/WD_CheckoutLog.txt"));
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
    set.setValue("RefreshRate", refreshRate->value());
    set.setValue("maxRefreshQuery", maxRefreshQuery->value());

    this->parentWidget()->startTimer(refreshRate->value());


    QStringList var;
    QList<QVariant> ports;
    QGridLayout* lay = findChild<QGridLayout*>("ServLayout");
    for (int i = 0; i < lay->rowCount(); ++i)
    {
        QLineEdit* ed = qobject_cast<QLineEdit*>(lay->itemAtPosition(i, 1)->widget());
        QSpinBox* sp = qobject_cast<QSpinBox*>(lay->itemAtPosition(i, 3)->widget());

        if (ed) var << ed->text();
        if (sp) ports << sp->value();
    }
    set.setValue("Server", var);
    set.setValue("ServerP", ports);



    set.setValue("ScreensDirectory", screensPath->paths());
    set.setValue("databaseDir", databasePath->currentPath());
    //    qDebug() << "Saved options";
}


void GlobalOptions::openDirectory()
{
    QVariant var = sender()->property("path");

    if (var.isNull() || !var.canConvert(QMetaType::QString)) return;


    showInGraphicalShell(this, var.toString());
}

void GlobalOptions::copyDirectory()
{
    QVariant var = sender()->property("path");

    if (var.isNull() || !var.canConvert(QMetaType::QString)) return;

    QApplication::clipboard()->setText(var.toString());

}

void printParent(QObject* ob, int pos = 0)
{

    qDebug() << pos << ob->objectName();
    if (ob->parent())
        printParent(ob->parent(), pos + 1);
}


void GlobalOptions::addServer()
{
    QSettings set;

    int send = sender()->objectName().toInt();
    QStringList var = set.value("Server", QStringList() << "127.0.0.1").toStringList();
    QList<QVariant> ports = set.value("ServerP", QList<QVariant>() << QVariant((int)13378)).toList();

    var.insert(send, "127.0.0.1");
    ports.insert(send, 13378);

    set.setValue("Server", var);
    set.setValue("ServerP", ports);

    QGridLayout* servl =  qobject_cast<QWidget*>(sender()->parent())->findChild<QGridLayout*>("ServLayout");
    servl->itemAtPosition(send, 5)->widget()->hide();

    ServGui(var, send+1, ports, servl, true);    return;

    send = sender()->objectName().toInt();
    var = set.value("Server", QStringList() << "127.0.0.1").toStringList();
    ports = set.value("ServerP", QList<QVariant>() << QVariant((int)13378)).toList();

    var.removeAt(send);
    ports.removeAt(send);

    set.setValue("Server", var);
    set.setValue("ServerP", ports);
}

void GlobalOptions::delServer()
{
    QSettings set;

    int send = sender()->objectName().toInt();
    QStringList var = set.value("Server", QStringList() << "127.0.0.1").toStringList();
    QList<QVariant> ports = set.value("ServerP", QList<QVariant>() << QVariant((int)13378)).toList();

    var.removeAt(send);
    ports.removeAt(send);

    set.setValue("Server", var);
    set.setValue("ServerP", ports);

    // Need to check if on the right object... may be wrong right now
    QGridLayout* servl =  qobject_cast<QWidget*>(sender()->parent())->findChild<QGridLayout*>("ServLayout");

    for (int i = 0; i < 6; ++i)
        if (servl->itemAtPosition(send, i) &&
                servl->itemAtPosition(send, i)->widget())
            servl->itemAtPosition(send, i)->widget()->hide();
    return;

    send = sender()->objectName().toInt();
    var = set.value("Server", QStringList() << "127.0.0.1").toStringList();
    ports = set.value("ServerP", QList<QVariant>() << QVariant((int)13378)).toList();

    var.removeAt(send);
    ports.removeAt(send);

    set.setValue("Server", var);
    set.setValue("ServerP", ports);
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
    QString param;
    if (!QFileInfo(pathIn).isDir())
        param = QLatin1String("/select,");
    param += QDir::toNativeSeparators(pathIn);
    QString command = explorer + " " + param;
    QProcess::startDetached(command);
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
}
