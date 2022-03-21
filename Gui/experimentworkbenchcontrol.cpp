#include "experimentworkbenchcontrol.h"

#include <QDebug>

#include <QMessageBox>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QTreeWidget>

#include <QLabel>
#include <QGridLayout>
#include <QComboBox>

#include <QSpinBox>

#include <ctkWidgets/ctkDoubleRangeSlider.h>
#include <ctkWidgets/ctkCollapsibleGroupBox.h>
#include <ctkWidgets/ctkRangeSlider.h>
#include <ctkWidgets/ctkPopupWidget.h>
#include <ctkWidgets/ctkDoubleSlider.h>
#include <ctkWidgets/ctkCheckableHeaderView.h>
#include <ctkWidgets/ctkColorPickerButton.h>
#include <ctkWidgets/ctkCheckableComboBox.h>

#include <Gui/ScreensDisplay/graphicsscreensitem.h>
#include <Gui/ScreensDisplay/wellcolorisation.h>

#include <QButtonGroup>
#include <QListWidget>

// Import opencv LDA class...
#include "opencv2/core.hpp"

ExperimentWorkbenchControl::ExperimentWorkbenchControl(ExperimentFileModel *mdl, GraphicsScreensItem* _gfx, QWidget *parent):
    QWidget(parent),
    _mdl(mdl),gfx(_gfx),
    _choice_container(0),
    _tags_container(0),
    _coloriz(0),  _xpTree(0), _colorButton(0), _colorButton2(0)
{
    if (mdl->getValidSequenceFiles().size() == 0) return;

    QSettings sets;

    //  QWidget* wid = new QWidget;
    ctkCollapsibleGroupBox* gbox = new ctkCollapsibleGroupBox("Plate coloring");
    QWidget* wid = gbox;
    //            wid->setAttribute();
    QVBoxLayout* lay = new QVBoxLayout(wid);
    lay->setContentsMargins(1,1,1,1);
    lay->setSpacing(1);


    {
        QWidget* container = new QWidget(gbox);

        QGridLayout* layout = new QGridLayout(container);

        layout->addWidget(new QLabel("Colorization"), 0, 0);
        // Colorization
        _coloriz = new QComboBox(container);
        connect(_coloriz, SIGNAL(currentIndexChanged(QString)), this, SLOT(colorizer_change(QString)));
        _coloriz->insertItems(0, QStringList() << "Value/PCA" << "LDA");
        layout->addWidget(_coloriz, 0, 1);


        lay->addWidget(container);
    }


    QWidget* rd = new QWidget(gbox);
    rd->setLayout(new QHBoxLayout(rd));

    _inverseColor = new QCheckBox;
    //  box->setAttribute(Qt::WA_DeleteOnClose);
    //  _inverseColor->setText("inv");
    _inverseColor->setToolTip("Invert signal intensities (dark becomes bright, and bright -> dark");
    _inverseColor->setChecked(false);//fo->active());
    //  box->connect(box, SIGNAL(toggled(bool)), fo, SLOT(setActive(bool)));

    connect(_inverseColor, SIGNAL(toggled(bool)), this, SLOT(updateModel()));

    rd->layout()->addWidget(_inverseColor);


    _rangeDisplay =   new ctkDoubleRangeSlider(Qt::Horizontal);

    connect(_rangeDisplay, SIGNAL(valuesChanged(double,double)), this, SLOT(updateModel()));

    rd->layout()->addWidget(_rangeDisplay);

    lay->addWidget(rd);

    _colorButton = new ctkColorPickerButton();
    _colorButton->setColor(QColor(Qt::white));
    _colorButton->setDisplayColorName(false);

    connect(_colorButton, SIGNAL(colorChanged(QColor)), this, SLOT(updateModel()));

    lay->addWidget(_colorButton);
    _colorButton2 = new ctkColorPickerButton();
    _colorButton2->setColor(QColor(Qt::red));
    _colorButton2->setDisplayColorName(false);

    connect(_colorButton2, SIGNAL(colorChanged(QColor)), this, SLOT(updateModel()));

    lay->addWidget(_colorButton2);
    _colorButton2->hide();



    _choice_container = new QWidget(this);
    {
        QGridLayout* layout = new QGridLayout(_choice_container);

        // Gradient will display data as a horizontal linear gradient for time evolution
        // Spread will create small rectangles representing each one a time point
        // Multiple, allows the user to select multiple timepoints
        layout->addWidget(new QLabel("Time"),0,0);
        layout->addWidget(createComboBox(QStringList() << "Time p." << "PCA" /* << "Mean" << "Median" << "Min" << "Max" << "Sum"  << "Gradient" << "Spread" << "Multiple" */, mdl->getValidSequenceFiles().first()->getTimePointCount()), 0, 1);

        layout->addWidget(new QLabel("Field"),0,2);
        layout->addWidget(createComboBox(QStringList() << "Field" << "PCA" /* << "Mean" << "Median" << "Min" << "Max"  << "Sum"  << "Spread"<< "Multiple" */, mdl->getValidSequenceFiles().first()->getFieldCount()), 0, 3);

        layout->addWidget(new QLabel("Z Stack"),1,0);
        layout->addWidget(createComboBox(QStringList() << "Z" << "PCA" /*<< "Mean" << "Median" << "Min" << "Max"  << "Sum" << "Spread"<< "Multiple"  */, mdl->getValidSequenceFiles().first()->getZCount()), 1, 1);

        layout->addWidget(new QLabel("Channel"),1,2);
        layout->addWidget(createComboBox(QStringList() << "Chan." << "PCA" /* << "Mean" << "Median" << "Min" << "Max"  << "Sum" << "Spread" << "Multiple" */, mdl->getValidSequenceFiles().first()->getChannels()), 1, 3);

    }

    lay->addWidget(_choice_container);

    _tags_container = new QWidget(this);
    {
        QGridLayout* layout = new QGridLayout(_tags_container);

        // Gradient will display data as a horizontal linear gradient for time evolution
        // Spread will create small rectangles representing each one a time point
        // Multiple, allows the user to select multiple timepoints
        layout->addWidget(new QLabel("Tags"),0,0,1,2);
        _selected_tags = new ctkCheckableComboBox();
        QStringList tags = sets.value("Tags", QStringList()).toStringList();
        _selected_tags->addItems(tags);
        //wd->setSelectionMode(QListWidget::MultiSelection);
        layout->addWidget(_selected_tags,0,2,1,2);
    }

    lay->addWidget(_tags_container);
    _tags_container->hide();

    _xpTree = new QTreeWidget(wid);
    _xpTree->header()->hide();
    _xpTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    _xpTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(_xpTree, SIGNAL(itemSelectionChanged()), this, SLOT(features_tree_selectionChanged()));

    lay->addWidget(_xpTree);

    this->setLayout(new QVBoxLayout());
    this->layout()->addWidget(wid);



}

QTreeWidget *ExperimentWorkbenchControl::getXPTree()
{
    return _xpTree;
}

QWidget * ExperimentWorkbenchControl::createComboBox(QStringList list, int max)
{
    QWidget* combo = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(combo);

    if (max > 1)
    {
        QComboBox* tcb = new QComboBox();
        QString name = list.front();

        tcb->setObjectName(name);
        list.front() = QString("%1 1").arg(name); // Force first entry to be pointing at the first value

        tcb->addItems(list);

        tcb->setCurrentIndex(1);

        connect(tcb, SIGNAL(currentIndexChanged(int)), this, SLOT(featuresFusionModeChanged(int)));

        layout->addWidget(tcb);

        QSpinBox* box = new QSpinBox(combo);
        box->setObjectName(name);

        layout->addWidget(box);
        box->setValue(1);
        box->setMinimum(1);
        box->setMaximum(max);
        box->hide();
        connect(box, SIGNAL(valueChanged(int)), this, SLOT(featuresSpinBoxChanged(int)));

        //  tcb->setUserData();
        layout->setSpacing(1);
        layout->setContentsMargins(0,0,0,0);
    }
    else
    {
        layout->addWidget(new QLabel("-"));
    }
    return combo;
}

void ExperimentWorkbenchControl::fillSelector(QList<QSpinBox*> fi, QVector<QList<int> >& select, int pos)
{
    if (fi.size())
    {
        QSpinBox* box = fi.first();
        //      qDebug() << box->value() << box->isHidden();
        QList<QComboBox*> lb = box->parent()->findChildren<QComboBox*>(box->objectName());
        if (lb.size() && lb.first()->currentIndex() == 0)
            //      if (!fi.first()->isHidden())
            select[pos] << box->value();
    }
}

void ExperimentWorkbenchControl::colorizer_change(QString txt)
{
    if (txt == "LDA")
    {
        if (_colorButton2) _colorButton2->show();
        if (_choice_container)_choice_container->hide();
        if (_tags_container) _tags_container->show();
    }
    else
    {
        if (_colorButton2)        _colorButton2->hide();
        if (_choice_container)_choice_container->show();
        if (_tags_container) _tags_container->hide();
    }

}

void ExperimentWorkbenchControl::updateModel(bool dataChanged)
{
    // Needs to be performed in a QFuture<>
    QList<QTreeWidgetItem*> olist = _xpTree->selectedItems();
    QList<QString> dblist, memlist;

    // Filter out non usable data
    foreach (QTreeWidgetItem* itm, olist)
        if ( itm->parent() &&
             itm->parent() != _xpTree->invisibleRootItem()
             )
        {
            if (itm->parent()->text(0) == "Database")
                dblist << itm->text(0);
            else
                memlist << itm->text(0);
        }

    bool isLDA =   (_coloriz->currentText() == "LDA" );
    if (isLDA && gfx->colorisationModel() )
        dataChanged = true;

    if (isLDA && _selected_tags->checkedIndexes().size() == 0) return;

    if (!dataChanged && !gfx->colorisationModel()) return;


    MatrixDataModel* mdm = 0;
    // Fetch data
    if (isLDA)
    {
        mdm = dataChanged ? _mdl->computedDataModel()->exposeData(memlist, dblist) :
                            gfx->colorisationModel()->getDataModel();
    }
    else
    {
        QVector<QList<int> > select(4);

        fillSelector(findChildren<QSpinBox*>("Time p."), select, MatrixDataModel::Time);
        fillSelector(findChildren<QSpinBox*>("Field"), select, MatrixDataModel::Field);
        fillSelector(findChildren<QSpinBox*>("Z"), select, MatrixDataModel::Stack);
        fillSelector(findChildren<QSpinBox*>("Chan."), select, MatrixDataModel::Channel);

        //  qDebug() << select;
        mdm = dataChanged ? _mdl->computedDataModel()->exposeData(memlist, dblist, select) :
                            gfx->colorisationModel()->getDataModel();
    }
    if (!mdm)
    {
        QMessageBox::warning(this, "Experiment Workbench display",  "Retreiving data for display failed\r\nPlease select another dataset");
        return;
    }

    //
    // Apply desired transformations
    // First a simple PCA over the Data is suggested
    if (dataChanged && mdm->columnCount() > 1)
    {
        if (isLDA)
        {
            perform_lda(mdm);
        }
        else
        {
            perform_pca(mdm);
        }
    }


    // display accordingly

    WellColorisation* colorizer = gfx->colorisationModel();
    QVector<QVector<double> > data = (*mdm)();


    if (!colorizer)
    {
        colorizer = new WellColorisation();
    }


    if (dataChanged)
    {
        double min = std::numeric_limits<double>::max(), max = -std::numeric_limits<double>::max();

        for (int i = 0; i < data[0].size(); ++i)
        {
            double v = data[0][i];
            //          qDebug() << v;
            if (min > v) min = v;
            else if (max < v) max = v;
        }
        //      qDebug() << min << max;

        _rangeDisplay->setRange(min,max);
        _rangeDisplay->setValues(min,max);
    }




    MatrixDataModel* oldmodel = colorizer->getDataModel();

    QColor c = this->_colorButton->color().toHsv();
    c.setHsv(c.hue(), c.saturation(), 255);

    QColor c2 = this->_colorButton2->color().toHsv();
    c2.setHsv(c2.hue(), c2.saturation(), 255);

    colorizer->updateControl(_rangeDisplay->minimumValue(), _rangeDisplay->maximumValue(), this->_inverseColor->checkState() == Qt::Checked,  c, c2);

    colorizer->setDataModel(mdm);

    gfx->updateColorisationModel(colorizer);

    if (dataChanged)  delete oldmodel;
}

void ExperimentWorkbenchControl::perform_lda(MatrixDataModel *mdm)
{
    // Let use OpenCV LDA dataset
    //    cv::Mat
    // qDebug() << "LDA";

    QModelIndexList idx = _selected_tags->checkedIndexes();

    QSet<QString> tags;

    foreach (QModelIndex i, idx) tags.insert(i.data().toString());

    int rws = 0, rows = 0;

    for (int r = 0; r < mdm->rowCount(); ++r)
    {
        QPoint p = mdm->rowPos(r);
        QStringList tgs = _mdl->getTags(p);
        if ( mdm->isActive(r))
        {
            //            qDebug() << p << r << rws << tgs;
            if (!tgs.isEmpty() ) rws ++;
            else  rows ++;
        }
    }

    cv::LDA lda(tags.size()-1);

    cv::Mat dat(rws, mdm->columnCount(),CV_64FC1);
    cv::Mat lbls(rws, 1, CV_32SC1);

    QStringList slist = tags.values();
    //   slist.indexOf()
    rws = 0;
    for (int r = 0; r < mdm->rowCount(); ++r)
    {
        QPoint p = mdm->rowPos(r);
        QStringList tgs = _mdl->getTags(p);
        if (!tgs.isEmpty() && mdm->isActive(r))
        {
            for (int cols = 0; cols < mdm->columnCount();++cols)
                dat.at<double>(rws,cols) = mdm->data(p.x(),p.y(), cols);

            lbls.at<int>(rws,0) = slist.indexOf(tgs.first());
            rws ++;
        }
    }

    lda.compute(dat, lbls);
    // Project ground truth
    cv::Mat refProj = lda.project(dat);

    cv::Mat others(rows, mdm->columnCount(), CV_64FC1);
    int rws2 = 0;
    for (int r = 0; r < mdm->rowCount(); ++r)
    {
        QPoint p = mdm->rowPos(r);
        QStringList tgs = _mdl->getTags(p);
        if (tgs.isEmpty() && mdm->isActive(r))
        {
            for (int cols = 0; cols < mdm->columnCount();++cols)
                others.at<double>(rws2,cols) = mdm->data(p.x(),p.y(), cols);
            rws2 ++;
        }
    }

    if (others.rows == 0) return;

    cv::Mat proj = lda.project(others);

    rws2 = 0;
    for (int r = 0; r < mdm->rowCount(); ++r)
    {
        QPoint p = mdm->rowPos(r);
        QStringList tgs = _mdl->getTags(p);
        if (tgs.isEmpty() && mdm->isActive(r))
        {
            for (int cols = 0; cols < proj.cols;++cols)
                others.at<double>(rws2,cols) = mdm->data(p.x(),p.y(), cols);
            rws2 ++;
        }
    }

    QList<int> res;
    for (int l = 0; l < proj.rows; ++l)
    {
        int bestId = -1;
        double bestDist = 999999999.9;
        for (int i=0; i<refProj.rows; i++)
        {
            double d = cv::norm(proj.row(l), refProj.row(i));
            if (bestDist > d)
            {
                bestDist = d;
                bestId = i;
            }
        }
        res.push_back(bestId);
    }

    //   qDebug() << "Ending";

    rws2 = 0;
    for (int r = 0; r < mdm->rowCount(); ++r)
    {
        QPoint p = mdm->rowPos(r);
        QStringList tgs = _mdl->getTags(p);
        if (tgs.isEmpty() && mdm->isActive(r))
        {
            mdm->setTag(r, slist.at(lbls.at<int>(res.at(rws2),0)));
            rws2 ++;
        }
    }
}

void ExperimentWorkbenchControl::perform_pca(MatrixDataModel* mdm)
{
    // FIXME: Properly propose the filters
    QVector<QVector<double> >& data = (*mdm)();
    // Fuse Data to get a single column according to the dataset
    const int cols = mdm->columnCount();
    const int rows = data[0].size();

    QVector<double> eigv(cols);

    double en = 0;
    for (int i = 0; i < eigv.size(); ++i)
    {
        eigv[i] = 2.*(rand()/((double)RAND_MAX) - 0.5);
        en += eigv[i];
    }
    for (int c = 0; c < cols; ++c)  eigv[c] /= en;

    // PCA computation, not extra stable but for a few values it's ok
    double lastNrj = 0;
    for (int c = 0; c < 10; ++c) // iterations
    {

        QVector<double> s(cols),t(cols);
        for (int r = 0; r < rows; ++r)
            if (mdm->isActive(r))
            {

                double v = 0;
                for (int c = 0; c < cols; ++c)
                {
                    t[c] = data[c][r];
                    v += t[c]*eigv[c];
                }
                for (int c = 0; c < cols; ++c)
                    s[c] += t[c]*v;
            }
        double en = 0;
        for (int c = 0; c < cols; ++c)  en += s[c];
        //        qDebug() << "PCA energy" << en << en-lastNrj;

        for (int c = 0; c < cols; ++c)  eigv[c] = s[c] / en;
        if (en-lastNrj < 1e-2) break;
        lastNrj = en;

    }

    QVector<double> proj(rows);
    for (int r = 0; r < rows; ++r)
        if (mdm->isActive(r))
        {
            for (int c = 0; c < cols; ++c)
            {
                proj[r] = data[c][r]*eigv[c];
            }
        }

    data.prepend(proj);
}


void ExperimentWorkbenchControl::features_tree_selectionChanged()
{
    if (!_xpTree) return;

    QList<QTreeWidgetItem*> list = _xpTree->selectedItems();
    if (!list.size()) return;

    QTreeWidgetItem* it  = list.first();

    if ( it->parent() == _xpTree->invisibleRootItem())
    {
        // Text some infos telling the user to pick data not dataset

        return;
    }


    updateModel(true);
}

void ExperimentWorkbenchControl::featuresFusionModeChanged(int index)
{
    QSpinBox* spin = sender()->parent()->findChild<QSpinBox*>();
    if (!spin) return;

    if (index == 0)
    {
        spin->show();
        qApp->processEvents();
        spin->setValue(spin->value());
    }
    else
    {
        spin->hide();
    }

    updateModel(true);
}

void ExperimentWorkbenchControl::featuresSpinBoxChanged(int value)
{

    updateModel(true);
}

