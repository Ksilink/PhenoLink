

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "screensmodel.h"

#include <map>
#include <algorithm>

#include <QFileSystemModel>
#include <QDebug>

#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QSettings>

#include <QProgressBar>

#include <QScrollBar>

#include <ScreensDisplay/graphicsscreensitem.h>

#include <QGraphicsRectItem>
//#include <QPushButton>

#include <QScrollArea>
#include <QSlider>

#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QClipboard>
#include <QtConcurrent/QtConcurrent>

#ifdef WIN32
#include <QtWinExtras/QWinTaskbarProgress>
#endif


#include <ctkWidgets/ctkDoubleRangeSlider.h>
#include <ctkWidgets/ctkCollapsibleGroupBox.h>
#include <ctkWidgets/ctkRangeSlider.h>
#include <ctkWidgets/ctkPopupWidget.h>
#include <ctkWidgets/ctkDoubleSlider.h>
#include <ctkWidgets/ctkCheckableHeaderView.h>
#include <ctkWidgets/ctkColorPickerButton.h>
#include <ctkWidgets/ctkPathLineEdit.h>

#include <QCheckBox>

#include <QtSql>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>

#include <QtTest/QSignalSpy>

#include <Core/checkoutprocess.h>
#include <Core/imageinfos.h>

#include <ImageDisplay/imageform.h>

#include <ImageDisplay/scrollzone.h>
#include <QHBoxLayout>

#include "ScreensDisplay/screensgraphicsview.h"
#include <QLineEdit>
#include <QTextBrowser>

#ifdef CheckoutCoreWithPython
#include "Python/checkoutcorepythoninterface.h"
#endif

#include <optioneditor.h>

#include "Core/pluginmanager.h"
#include "Core/networkprocesshandler.h"

#include <QErrorMessage>
#include <QCloseEvent>
#include <guiserver.h>

#include <QColorDialog>





void MainWindow::overlay_selection(const QString& text)
{
    SequenceInteractor* inter = _sinteractor.current();
    QStringList overlays = inter->getMetaList();
    int id = overlays.indexOf(text);

}

void MainWindow::change_overlay_details(QString values, QString overlay, int id)
{
    ui->valuesOverlay->clear();

    auto vals = values.split("\r\n");
    ui->valuesOverlay->insertItems(0, vals);

    ui->overlayId->setText(QString("%1").arg(id));

    ui->pickOverlay->setCurrentText(overlay);
}

void MainWindow::on_prevOverlay_clicked()
{
//
    auto sp = ui->overlayControl->findChild<QSpinBox*>(ui->pickOverlay->currentText());
    if (sp)
        sp->setValue(sp->value()-1);
}


void MainWindow::on_nextOverlay_clicked()
{
    auto sp = ui->overlayControl->findChild<QSpinBox*>(ui->pickOverlay->currentText());
    if(sp)
        sp->setValue(sp->value()+1);
}



void MainWindow::on_addTag_clicked()
{
    QString tg =ui->tagSelector->currentText();

    if (ui->tagSelector->findText(tg)==-1)
        ui->tagSelector->insertItem(0, tg);
    if(ui->tagList->findItems( tg, Qt::MatchExactly).size()==0)
        ui->tagList->addItem(tg);
}


void MainWindow::on_delTag_clicked()
{
    auto tg =ui->tagSelector->currentText();
    QStringList names;

    for (int i =0; i <   ui->tagList->count();++i)
        if (ui->tagList->item(i)->text()!=tg)
            names<<ui->tagList->item(i)->text();

    ui->tagList->clear();
    ui->tagList->addItems(names);

}

