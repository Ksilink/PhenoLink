

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
#include <QInputDialog>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>
#include <QListWidget>

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

#include <Widgets/overlayfilter.h>

#undef signals
#include <arrow/api.h>
#include <arrow/filesystem/filesystem.h>
#include <arrow/util/iterator.h>
#include <arrow/ipc/writer.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/api.h>



namespace fs = arrow::fs;

void MainWindow::overlayClearTags()
{
    ui->tagList->clear();
}



void MainWindow::overlay_selection(const QString& text)
{
    overlayClearTags();

    SequenceInteractor* inter = _sinteractor.current();
    QStringList overlays = inter->getMetaList();
    int id = overlays.indexOf(text);

    Q_UNUSED(id);

}

void MainWindow::change_overlay_details(QString values, QString overlay, int id)
{
    overlayClearTags();

    ui->valuesOverlay->clear();

    auto vals = values.split("\r\n");
    ui->valuesOverlay->insertItems(0, vals);
    ui->overlayId->setText(QString("%1").arg(id));
    ui->pickOverlay->setCurrentText(overlay);

    SequenceInteractor* inter = _sinteractor.current();

    ui->showOverlay->setPixmap(inter->getSubPixmap(overlay, id));
}





void MainWindow::on_prevOverlay_clicked()
{

    //
    overlayClearTags();
    int val = std::max(0, ui->overlayId->text().toInt() - 1);
    SequenceInteractor* inter = _sinteractor.current();

    for (auto sp : ui->overlayControl->findChildren<QSpinBox*>(ui->pickOverlay->currentText()))
        if (sp)
        {
            sp->setValue(val);

            ui->showOverlay->setPixmap(inter->getSubPixmap(ui->pickOverlay->currentText(), val));

            QStringList tags = inter->getTag(ui->pickOverlay->currentText(), val);
            ui->tagList->clear();
            ui->tagList->addItems(tags);


        }
    ui->overlayId->setText(QString("%1").arg(val));
}


void MainWindow::on_nextOverlay_clicked()
{
    overlayClearTags();
    int val = ui->overlayId->text().toInt() + 1;

    for (auto sp : ui->overlayControl->findChildren<QSpinBox*>(ui->pickOverlay->currentText()))
        if(sp)
        {
            sp->setValue(val);

            SequenceInteractor* inter = _sinteractor.current();
            ui->showOverlay->setPixmap(inter->getSubPixmap(ui->pickOverlay->currentText(), val));

            QStringList tags = inter->getTag(ui->pickOverlay->currentText(), val);
            ui->tagList->clear();
            ui->tagList->addItems(tags);
        }
    ui->overlayId->setText(QString("%1").arg(val));

}

void MainWindow::on_clearTags_clicked()
{
    ui->tagList->clear();

    SequenceInteractor* inter = _sinteractor.current();
    inter->setTag(ui->pickOverlay->currentText(), ui->overlayId->text().toInt(), QStringList());
}



void MainWindow::on_addTag_clicked()
{
    QString tg =ui->tagSelector->currentText();

    if (ui->tagSelector->findText(tg)==-1)
        ui->tagSelector->insertItem(0, tg);
    if(ui->tagList->findItems( tg, Qt::MatchExactly).size()==0)
        ui->tagList->addItem(tg);

    QStringList tags;
    for (int i = 0; i < ui->tagList->count(); ++i)
        tags << ui->tagList->item(i)->text();

    SequenceInteractor* inter = _sinteractor.current();
    inter->setTag(ui->pickOverlay->currentText(), ui->overlayId->text().toInt(), tags);
}


void MainWindow::on_delTag_clicked()
{
    auto tg = ui->tagList->currentItem()->text();
    //    auto tg =ui->tagSelector->currentText();
    QStringList names;

    for (int i =0; i <   ui->tagList->count();++i)
        if (ui->tagList->item(i)->text()!=tg)
            names<<ui->tagList->item(i)->text();

    ui->tagList->clear();
    ui->tagList->addItems(names);

    SequenceInteractor* inter = _sinteractor.current();
    inter->setTag(ui->pickOverlay->currentText(), ui->overlayId->text().toInt(), names);

}

#define ABORT_ON_FAILURE(expr)                     \
    do {                                             \
    arrow::Status status_ = (expr);                \
    if (!status_.ok()) {                           \
    std::cerr << status_.message() << std::endl; \
    abort();                                     \
    }                                              \
    } while (0);


#define ArrowGet(var, id, call, msg) \
    auto id = call; if (!id.ok()) { qDebug() << msg; return ; } auto var = id.ValueOrDie();


void ReadFeather(QString file, StructuredMetaData& data)
{
    data.setProperties("Filename", file);

    std::string uri = file.toStdString(), root_path;
    ArrowGet(fs, r0, fs::FileSystemFromUriOrPath(uri, &root_path), "Arrow File not loading" << file);

    ArrowGet(input, r1, fs->OpenInputFile(uri), "Error openning arrow file" << file);
    ArrowGet(reader, r2, arrow::ipc::RecordBatchFileReader::Open(input), "Error Reading records");

    auto schema = reader->schema();

    QStringList fields;
    QList<int> field_pos;
    int pos = 0;
    for (auto f : schema->fields())
    {
        int count = 0;
        for (auto r : { "tags" }) if (f->name() == r) count++;
        pos++;

        if (count) continue;
        if (f->type() !=  arrow::float32()) continue;

        fields << QString::fromStdString(f->name());
        field_pos << pos;
    }


    ArrowGet(rowC, r3, reader->CountRows(), "Error reading row count");
    cv::Mat mat(rowC, fields.size(), CV_32F);

    data.setProperties("ChannelNames", fields.join(";"));

    int r = 0;
    for (int record = 0; record < reader->num_record_batches(); ++record)
    {
        ArrowGet(rb, r4, reader->ReadRecordBatch(record), "Error Get Record");
        int c = 0, mr = 0;

        for (auto& field : fields)
        {
            auto array = std::static_pointer_cast<arrow::FloatArray>(rb->GetColumnByName(field.toStdString()));
            if (array)
            {
                for (int s = 0; s < array->length(); ++s)
                {
                    if (array->IsValid(s))
                        mat.at<float>(s+r, c) = array->Value(s);
                }

                mr = std::max(mr, (int)array->length());
            }
            c++;
        }

        r+=mr;
    }

    data.setContent(mat);
    r = 0;
    for (int record = 0; record < reader->num_record_batches(); ++record)
    {
        auto rb = reader->ReadRecordBatch(record).ValueOrDie();
        auto tag = std::static_pointer_cast<arrow::StringArray>(rb->GetColumnByName("tags"));

        for (int s = 0; s < tag->length(); ++s)
            if (tag->IsValid(s))
            {
                std::string t = tag->GetString(s);
                //                qDebug() << "Reading:" << r+s << QString::fromStdString(t);
                data.setTag(r+s, QString::fromStdString(t).split(";"));
            }
        r+=tag->length();
    }
}

void MainWindow::importOverlay()
{
    QSet<QString> tags;
    //
    QSettings set;
    QString dbP=set.value("databaseDir").toString();
    SequenceInteractor* inter = _sinteractor.current();

    // Regular Expression for finding Commit names:
    QDir dir(QString("%1/PROJECTS/%2/Checkout_Results/").arg(dbP,inter->getProjectName()));

    QMap<QString, QStringList> coms;

    for (auto ent: dir.entryInfoList(QStringList() << "*", QDir::Dirs | QDir::NoDotAndDotDot))
    {
        if (QFile::exists(ent.absoluteFilePath()+"/"+inter->getSequenceFileModel()->getOwner()->name()+".fth"))
        {
            QString commitName = ent.absoluteFilePath().replace(QString("%1/PROJECTS/%2/Checkout_Results/").arg(dbP,inter->getProjectName()) , "");
            QDir dir(ent.absoluteFilePath());
            for (auto item: dir.entryInfoList( QStringList() << QString("%1_%2_*.fth").arg(inter->getSequenceFileModel()->getOwner()->name(),inter->getSequenceFileModel()->Pos()), QDir::Files | QDir::NoDotAndDotDot))
            {
                coms[commitName] << item.absoluteFilePath();
            }
        }
    }

    if (coms.size() == 0)
    {
        QMessageBox::warning(this, "Import Overlays", "No commit found for this plate & well");
        return;
    }

    bool ok;
    QString scom = QInputDialog::getItem(this, "Select Commit Name data", "Commit Name:", coms.keys(), 0, false, &ok);

    if (ok & !scom.isEmpty())
    {
        // Let's reload the datas !!!!
        //  void addMeta(int timePoint, int fieldIdx, int Zindex, int Channel, QString name, StructuredMetaData meta);

        overlay_t = inter->getTimePoint();
        overlay_f = inter->getField();
        overlay_z = inter->getZ();
        overlay_c = inter->getChannel();
        for (auto& file: coms[scom])
        {
            QStringList name = file.split("_");
            QStringList key;
            bool found=false;
            for (auto& n: name)
            {
                if (found) key << n;
                if (n==inter->getSequenceFileModel()->Pos())  found=true;
            }
            key.last() = key.last().replace(".fth", "");

            StructuredMetaData meta;

            ReadFeather(file, meta);

            for (size_t i = 0; i < meta.length(); ++i)
            {
                auto t = meta.getTag((int)i);
                tags.unite(QSet<QString>(t.begin(), t.end()));
            }

            QString meta_name = key.join("_");
            inter->getSequenceFileModel()->addMeta(
                        overlay_t,
                        overlay_f,
                        overlay_z,
                        overlay_c,
                        meta_name,
                        meta);
        }
        inter->changed();
        updateCurrentSelection();
        ui->tagSelector->addItems(tags.values());
    }

}

void MainWindow::exportOverlay() // to force save of overlay on modification
{
    SequenceInteractor* inter = _sinteractor.current();

    QMap<QString, StructuredMetaData>& metas = inter->getSequenceFileModel()->getMetas(overlay_t,
                                                                                       overlay_f,
                                                                                       overlay_z,
                                                                                       overlay_c);

    for (auto items = metas.begin(), e = metas.end() ; items != e; ++items)
    {
        qDebug() <<  "Saving name" << items.key();
        items.value().exportData();
    }
}


void MainWindow::exportContent()
{
    if (sender())
    {
        QString name = sender()->objectName();
        QString fname = QFileDialog::getSaveFileName(this, "Save File",
                                                     QDir::homePath(), tr("csv File (*.csv)"));
        if (!fname.isEmpty())
        {
            _sinteractor.current()->exportOverlay(name, fname);
        }
    }
}


void MainWindow::on_filterOverlay_clicked()
{
    auto inter =    _sinteractor.current();

    //inter->getTag()
    if (!overlay_filter)
    {
        overlay_filter_dock = new QDockWidget(this);

        addDockWidget(Qt::TopDockWidgetArea, overlay_filter_dock);
        overlay_filter_dock->setFloating(true);

        overlay_filter = new OverlayFilter();
        overlay_filter_dock->setWidget(overlay_filter);
    }
    overlay_filter_dock->show();

    QMap<QString, int> tagCounter;

    for (int i = 0; i < inter->getTagSize(ui->pickOverlay->currentText()); ++i)
    {
        QStringList tags = inter->getTag(ui->pickOverlay->currentText(), i);
        for (auto& t : tags)
            tagCounter[t]++;
    }
    //    qDebug() << tagCounter;

    //    overlay_list->clear();
    QStringList str;
    for (auto it = tagCounter.begin(), e = tagCounter.end(); it != e; ++it)
        str <<  QString("%1 (%2)").arg(it.key()).arg(it.value());


    overlay_filter->setTagList(str);

    connect(overlay_filter, &OverlayFilter::filterChanged, [this](QStringList t)
    {
        this->_sinteractor.current()->setTagFilter(t);
    });

}

