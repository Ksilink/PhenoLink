#include "imageform.h"
#include "ui_imageform.h"

#include <QGraphicsPixmapItem>

#include <QGraphicsItem>

#include <QDebug>

#include <QSettings>
#include <QScrollBar>

#include <QMenu>
#include <QAction>
#include <QClipboard>

#include "graphicssignitem.h"
#include "graphicspixmapitem.h"


#include <Core/sequenceinteractor.h>
#include <QCloseEvent>
#include <QLabel>

#include <QtConcurrent/QtConcurrent>


#include "ImageDisplay/scrollzone.h"

//ImageForm* ImageForm::_selectedForm = 0;


// Force min size to be 320x270
ImageForm::ImageForm(QWidget *parent) :
    QWidget(parent),
    video_status(VideoStop),
   sz(0),
   ui(new Ui::ImageForm), aspectRatio(0.0), currentScale(1.0f),  isRunning(false)
{
    ui->setupUi(this);
    // Set scrollbar to disabled to avoid the interpretation of wheel events
    ui->graphicsView->verticalScrollBar()->setEnabled(false);
    ui->graphicsView->horizontalScrollBar()->setEnabled(false);

    pixItem = new GraphicsPixmapItem();
    //    pixItem->
    if (!ui->graphicsView->scene())
        ui->graphicsView->setScene(new QGraphicsScene());

    QGraphicsScene* scene = ui->graphicsView->scene();
    scene->addItem(pixItem);
    connect(pixItem, SIGNAL(mouseClick(QPointF)), this, SLOT(changeCurrentSelection()));

    connect(pixItem, SIGNAL(mouseClick(QPointF)), this, SLOT(imageClick(QPointF)));
    connect(pixItem, SIGNAL(mouseOver(QPointF)), this, SLOT(mouseOverImage(QPointF)));

    connect(pixItem, SIGNAL(mouseWheel(int)), this, SLOT(scale(int)));

    pixItem->setPos(0,0);




    struct {
        GraphicsSignItem::Signs sign;
        const char* slot;
    } createData[] = {
    { GraphicsSignItem::Minus, SLOT(minusClicked()) },
    { GraphicsSignItem::Plus, SLOT(plusClicked()) },
    { GraphicsSignItem::PrevIm, SLOT(prevImClicked()) },
    { GraphicsSignItem::NextIm, SLOT(nextImClicked()) },
    { GraphicsSignItem::SliceUp, SLOT(sliceUpClicked()) },
    { GraphicsSignItem::SliceDown, SLOT(sliceDownClicked()) },
    { GraphicsSignItem::FwdPlay, SLOT(FwdPlayClicked()) },
    { GraphicsSignItem::BwdPlay, SLOT(BwdPlayClicked()) },
    { GraphicsSignItem::NextFrame, SLOT(nextFrameClicked()) },
    { GraphicsSignItem::PrevFrame, SLOT(prevFrameClicked()) }
}   ;

    for (unsigned i = 0; i < (unsigned)GraphicsSignItem::Count; ++i)
    {
        gsi[createData[i].sign] = new GraphicsSignItem(createData[i].sign);
        gsi[createData[i].sign]->setScale(0.7);
        connect(gsi[createData[i].sign], SIGNAL(mouseClick()), this, createData[i].slot);
        scene->addItem(gsi[createData[i].sign]);
    }

    textItem = new QGraphicsTextItem();
    textItem->setDefaultTextColor(Qt::yellow);

    imageInfos = QString("");
    textItem->setPlainText(imageInfos);

    scene->addItem(textItem);


    textItem2 = new QGraphicsTextItem();
    textItem2->setDefaultTextColor(Qt::yellow);

    imagePosInfo = QString("");
    textItem2->setPlainText(imagePosInfo);

    scene->addItem(textItem2);


    QSettings q;
    //    FIXME: Set the parameters information windows to display the values
    scaleFactor = q.value("ImageWidget/Scale", .75).toFloat();

    //    if (!_selectedForm) _selectedForm = this;

}

ImageForm::~ImageForm()
{
    delete ui;
}

void ImageForm::setPixmap(QPixmap pix)
{

    _pix = pix;
    pixItem->setPixmap(_pix);
    ui->graphicsView->setSceneRect(pixItem->boundingRect());

    // pixItem->setPos(0,0);
    //pixItem->setPos(_pix.width()/2, _pix.height()/2);
}


void ImageForm::watcherPixmap()
{
    QFutureWatcher<QPixmap>* wa = dynamic_cast<QFutureWatcher<QPixmap>* >(sender());
    //  qDebug() << "Redrzw";
    if (!wa) {
        qDebug() << "Error retreiving watchers finished state";
        return;
    }

    redrawPixmap(wa->future().result());
    isRunning = false;
    //  wa->deleteLater();
    //  _waiting_Update = false;
    delete wa;
}


void ImageForm::redrawPixmap(QPixmap img)
{
    //    qDebug() << "redrawPixmap";
    setPixmap(img);
    pixItem->update();
}


QPixmap runnerInteractorGetPixmap( SequenceInteractor* _interactor)
{
    //    qDebug() << "Start pixmap";
    QPixmap r = _interactor->getPixmap();
    //qDebug() << "Finished pixmap";

    return r;
}

void ImageForm::redrawPixmap()
{
    if (!isRunning)
    {
        isRunning = true;
        QFutureWatcher<QPixmap>* wa = new QFutureWatcher<QPixmap>();
        connect(wa, SIGNAL(finished()), this, SLOT(watcherPixmap()));

        QFuture<QPixmap>  future = QtConcurrent::run(runnerInteractorGetPixmap, _interactor);
        wa->setFuture(future);
        //      future.waitForFinished();
    }
  //  scale(0);

    //qDebug() << "Redraw Pixmap";
    //    redrawPixmap(QPixmap::fromImage(_interactor->getImage()));
}


void ImageForm::updateImage()
{
    setPixmap(_interactor->getPixmap());
    scale(0);
}

void ImageForm::modifiedImage()
{
    redrawPixmap();
}

void ImageForm::setScrollZone(ScrollZone *z)
{
    sz = z;
    sz->clearSelection();
    sz->select(this);
    setSelectState(true);
}

void ImageForm::setSelectState(bool val)
{
    modelView()->setSelectState(val);
}

void ImageForm::scale(int delta)
{
    changeCurrentSelection();

    QRectF r = ui->graphicsView->viewport()->rect();

    // Center of view position

    QPointF center = pixItem->mapFromScene(QPointF(r.width()/2, r.height()/2));
    // Or mouse position
    if (fromButton) center = _pos;

    //1280*1070
    if (delta != 0)
    {
        if (delta < 0)
        {
            if (currentScale * scaleFactor >= 1)
            {
                currentScale *= scaleFactor;
            }
        }
        else
        {
            currentScale /= scaleFactor;
        }
    }

    // Recenter prior to scale to properly position afterwards
    pixItem->setPos(0,0);
    // Apply scaling
    pixItem->setScale(aspectRatio * currentScale);
    // Correct position with mapping of the previous center to the edge
    pixItem->setPos(  - pixItem->mapToScene(center)
                      // Correct with respect to the mouse pos...
                      + (QPointF(ui->graphicsView->size().width(), ui->graphicsView->size().height())
                         - ui->graphicsView->mapFromGlobal(QCursor::pos()))
                      )
            ;
    fromButton = false;

}


void ImageForm::connectInteractor()
{
    if (_interactor)
    {
        //    QMetaObject::Connection c = connect(_interactor, SIGNAL(modifiedImage()), this, SLOT(redrawPixmap()));
        //    qDebug() << c;
        _interactor->addImage(this);
    }
    this->_interactor->setCurrent(_interactor);

    textItem->setPlainText(imageInfos);
}

QString ImageForm::contentPos()
{
    if (_interactor && _interactor->getSequenceFileModel())
        return _interactor->getSequenceFileModel()->Pos();
    return QString();
}


void ImageForm::updateButtonVisibility()
{
    for (unsigned i = 0; i < (unsigned)GraphicsSignItem::Count; ++i)
        gsi[i]->setVisible(true);

    if (_view->getZCount() <= 1)
    {
        gsi[GraphicsSignItem::SliceDown]->setVisible(false);
        gsi[GraphicsSignItem::SliceUp]->setVisible(false);
    }

    if (_view->getTimePointCount() <= 1)
    {
        gsi[GraphicsSignItem::FwdPlay]->setVisible(false);
        gsi[GraphicsSignItem::BwdPlay]->setVisible(false);
        gsi[GraphicsSignItem::PrevFrame]->setVisible(false);
        gsi[GraphicsSignItem::NextFrame]->setVisible(false);
    }

    if (_view->getFieldCount() <= 1)
    {
        gsi[GraphicsSignItem::NextIm]->setVisible(false);
        gsi[GraphicsSignItem::PrevIm]->setVisible(false);
    }
}

void ImageForm::setModelView(SequenceFileModel *view, SequenceInteractor* interactor)
{
    this->_view = view;
    //    this->_interactor = new SequenceInteractor(view);
    if (interactor) this->_interactor = interactor;
    this->_interactor->setCurrent(_interactor);


    imageInfos = QString("%1 (Z: %2, t: %3, F: %4)")
            .arg(_interactor->getSequenceFileModel()->Pos())
            .arg(_interactor->getZ())
            .arg(_interactor->getTimePoint())
            .arg(_interactor->getField());


    updateButtonVisibility();


}

SequenceFileModel *ImageForm::modelView()
{
    return _view;
}

QSize ImageForm::sizeHint() const
{
    if (!_pix.isNull())   return _pix.size();

    return QSize(256,216);
}

//ImageForm *ImageForm::getCurrent()
//{
//    return _selectedForm;
//}


// The position of the controls needs to be recomputed and corrected

void ImageForm::resizeEvent(QResizeEvent *event)
{
    //    Q_UNUSED(event);

    // qDebug() << "ImageForm: Resize Event" << event->oldSize() << event->size();

    gsi[GraphicsSignItem::Minus]->setPos(gsiWidth(GraphicsSignItem::Plus)+2, 0);

    textItem->setPos(gsiWidth(GraphicsSignItem::Minus)+2, 0);


    //    gsiNextIm
    gsi[GraphicsSignItem::NextIm]->setPos(ui->graphicsView->size().width()-16-2,
                                          ui->graphicsView->size().height()-16-2);

    //    gsiPrevIm
    gsi[GraphicsSignItem::PrevIm]->setPos(ui->graphicsView->size().width()-2*16-2,
                                          ui->graphicsView->size().height()-16-2);

    //    gsiSliceUp
    gsi[GraphicsSignItem::SliceUp]->setPos(ui->graphicsView->size().width()-2*gsiWidth(GraphicsSignItem::SliceUp)-2,
                                           0);
    //    gsiSliceDown
    gsi[GraphicsSignItem::SliceDown]->setPos(ui->graphicsView->size().width()-2*gsiWidth(GraphicsSignItem::SliceDown)-2,
                                             gsiHeight(GraphicsSignItem::SliceUp)+2);


    //    gsiFwdPlay
    gsi[GraphicsSignItem::FwdPlay]->setPos(0,
                                           ui->graphicsView->size().height() - 2*gsiHeight(GraphicsSignItem::FwdPlay));
    //    gsiNextFrame
    gsi[GraphicsSignItem::NextFrame]->setPos(gsiWidth(GraphicsSignItem::FwdPlay)+6,
                                             ui->graphicsView->size().height() - 2*gsiHeight(GraphicsSignItem::FwdPlay));


    //    gsiBwdPlay
    gsi[GraphicsSignItem::BwdPlay]->setPos(gsiWidth(GraphicsSignItem::PrevFrame)+12,
                                           ui->graphicsView->size().height() - 4 * gsiHeight(GraphicsSignItem::FwdPlay));
    //    gsiPrevFrame
    gsi[GraphicsSignItem::PrevFrame]->setPos(0,
                                             ui->graphicsView->size().height() - 4 * gsiHeight(GraphicsSignItem::FwdPlay));


    textItem2->setPos(gsiWidth(GraphicsSignItem::PrevFrame)+12,
                      ui->graphicsView->size().height() - 2  * gsiHeight(GraphicsSignItem::FwdPlay));

    QSize r = pixItem->pixmap().size();//boundingRect();

    aspectRatio = std::max(event->size().width()/ (float)r.width(),
                           event->size().height()/(float)r.height());

    QPointF center = pixItem->mapFromScene(QPointF(r.width()/2, r.height()/2));

    // Recenter prior to scale to properly position afterwards
    pixItem->setPos(0,0);
    // Apply scaling
    pixItem->setScale(aspectRatio * currentScale);
    // Correct position with mapping of the previous center to the edge
    pixItem->setPos(  - pixItem->mapToScene(center)
                      // Correct with respect to the mouse pos...
                      + (QPointF(ui->graphicsView->size().width(), ui->graphicsView->size().height())
                         - ui->graphicsView->mapFromGlobal(QCursor::pos()))
                      )
            ;

}

void ImageForm::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    //  changeCurrentSelection();
    //  event->accept();
    //  qDebug() << "PressEvent...";
    //  qDebug() << pixItem->pos();
}

void ImageForm::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (parent())
    {
        this->setParent(0);
        this->show();
    }
    else
    {
        this->setParent(sz->widget());

        sz->widget()->layout()->addWidget(this);

        this->show();
    }
}

void ImageForm::paintEvent(QPaintEvent *event)
{

    QWidget::paintEvent(event);
    if (sz->isSelected(this))
    {
        QPainter painter(this);

        QPen pen(Qt::red);
        pen.setWidth(3);
        painter.setPen(pen);

        painter.drawRect(rect());
    }
}

void ImageForm::minusClicked()
{
    fromButton = true;
    scale(-1);
}

void ImageForm::plusClicked()
{
    fromButton = true;
    scale(1);
}

void ImageForm::prevImClicked()
{
    if (_interactor->getField() > 1)
    {
        _interactor->setField(_interactor->getField() - 1);
        setPixmap(_interactor->getPixmap());
    }
    changeCurrentSelection();
}

void ImageForm::nextImClicked()
{
    if (_interactor->getField() <= _interactor->getFieldCount())
    {
        _interactor->setField(_interactor->getField() + 1);
        setPixmap(_interactor->getPixmap());
    }
    changeCurrentSelection();
}

void ImageForm::sliceUpClicked()
{
    if (_interactor->getZ() <= _interactor->getZCount())
    {
        _interactor->setZ(_interactor->getZ()+1);
        setPixmap(_interactor->getPixmap());
    }
    changeCurrentSelection();
}

void ImageForm::sliceDownClicked()
{
    if (_interactor->getZ() > 1)
    {
        _interactor->setZ(_interactor->getZ() - 1);
        setPixmap(_interactor->getPixmap());
    }
    changeCurrentSelection();
}

void ImageForm::nextFrameClicked()
{
    if (_interactor->getTimePoint() <= _interactor->getTimePointCount())
    {
        _interactor->setTimePoint(_interactor->getTimePoint()+1);
        setPixmap(_interactor->getPixmap());
    }
    changeCurrentSelection();
}

void ImageForm::prevFrameClicked()
{
    if (_interactor->getTimePoint() > 1)
    {
        _interactor->setTimePoint(_interactor->getTimePoint() - 1);
        setPixmap(_interactor->getPixmap());
    }
    changeCurrentSelection();
}

void ImageForm::FwdPlayClicked()
{
    if (video_status != ImageForm::VideoStop) {
        video_status = ImageForm::VideoStop;
        killTimer(timer_id);
        return;
    }
    video_status = ImageForm::VideoForward;

    qDebug() << "Fwd Play"<< _interactor->getTimePoint()<<"total"<< _interactor->getTimePointCount();
   
    timer_id = startTimer(45);
}

void ImageForm::BwdPlayClicked()
{
    if (video_status != ImageForm::VideoStop) {
        video_status = ImageForm::VideoStop;
        killTimer(timer_id);
        return;
    }

    video_status = ImageForm::VideoBackward;
    qDebug() << "Bwd Play"<< _interactor->getTimePoint()<<"total"<< _interactor->getTimePointCount();

    timer_id = startTimer(45);
}


#ifdef Checkout_With_VTK

#include <QVTKWidget.h>
#include <vtkAutoInit.h>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

#include <vtkProperty.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredPoints.h>
#include <vtkImageData.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkImageViewer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderer.h>
#include <vtkAssembly.h>

#include <vtkSmartPointer.h>
#include <vtkCamera.h>
#include <vtkFiniteDifferenceGradientEstimator.h>
#include <vtkImageClip.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsReader.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkColorTransferFunction.h>
#include "vtkSmartVolumeMapper.h"

void ImageForm::display3DRendering()
{

    QVTKWidget* vtk = new QVTKWidget(0);

    vtk->resize(120,120);

    //  Build vtkStructuredPoints from data
    vtkSmartPointer<vtkImageData> structuredPoints
            = vtkSmartPointer<vtkImageData>::New();

    structuredPoints->SetDimensions(_pix.width(), _pix.height() ,
                                    _interactor->getZCount());

    structuredPoints->AllocateScalars(VTK_UNSIGNED_CHAR, 4); // RGB 3 channels...


    int* dims = structuredPoints->GetDimensions();
    for (int z = 0; z < dims[2]; z++)
        for (int y = 0; y < dims[1]; y++)
            for (int x = 0; x < dims[0]; x++)
            {
                unsigned char* pixel = static_cast<unsigned char*>(structuredPoints->GetScalarPointer(x,y,z));

                pixel[0] = 0;
                pixel[1] = 0;
                pixel[2] = 0;
                pixel[3] = 0;

            }

    SequenceFileModel* model = _interactor->getSequenceFileModel();

    for (int z = 0; z < dims[2]; z++)
    {
        QMap<int, QString> l = model->getChannelsFiles(_interactor->getTimePoint(), _interactor->getField(), z+1);

        foreach(QString s , l)
        {
            //            qDebug() << s;
            cv::Mat im = cv::imread(s.toStdString(),  2);
            ImageInfos* ifo = _interactor->imageInfos(s);

            if (im.empty()) break;

            if (im.type() != CV_16U)
            {
                cv::Mat t;
                im.convertTo(t, CV_16U);
                im = t;
            }


            //        qDebug() << im.size().height << im.size().width << z;
            for (int y = 0; y < dims[1]; y++)
            {
                unsigned short *p = im.ptr<unsigned short>(y);
             //   const unsigned short v = *p;
                const float mi = ifo->getDispMin(),
                        ma = ifo->getDispMax();
                const int R = ifo->Red();
                const int G = ifo->Green();
                const int B = ifo->Blue();
                float mami = ma - mi;

                for (int x = 0; x < dims[0]; x++,p++)
                {

                    unsigned char* pixel = static_cast<unsigned char*>(structuredPoints->GetScalarPointer(x,y,z));
                    const float f = std::min(1.f, std::max(0.f, (*p - mi) / (mami)));

                    pixel[0] = (unsigned char)std::min(255.f, pixel[0] + f * B);
                    pixel[1] = (unsigned char)std::min(255.f, pixel[1] + f * G);
                    pixel[2] = (unsigned char)std::min(255.f, pixel[2] + f * R);

                    pixel[3] += (255.*f) / _interactor->getChannels() ;
                }
            }
        }
    }
    //    ImageInfos* nfo = _interactor->getChannelImageInfos(0);

    // Create a transfer function mapping scalar value to opacity
    vtkSmartPointer<vtkPiecewiseFunction> oTFun =
            vtkSmartPointer<vtkPiecewiseFunction>::New();


    vtkSmartPointer<vtkColorTransferFunction> cTFun =
            vtkSmartPointer<vtkColorTransferFunction>::New();


    vtkSmartPointer<vtkVolumeProperty> property =
            vtkSmartPointer<vtkVolumeProperty>::New();
    property->SetIndependentComponents(false);
    property->SetScalarOpacity(oTFun);
    property->SetColor(cTFun);
    property->SetInterpolationTypeToLinear();

    oTFun->AddPoint(0, 0.0);
    oTFun->AddPoint(255.0, 4.);


    vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> mapper =
            vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();

    mapper->SetBlendModeToComposite();
    property->ShadeOff();
    property->SetScalarOpacityUnitDistance(1.0);


    mapper->SetInputData(structuredPoints);
    vtkSmartPointer<vtkVolume> volume =
            vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(mapper);
    volume->SetProperty(property);


    // Setup window
    vtkSmartPointer<vtkRenderWindow> renderWindow =
            vtkSmartPointer<vtkRenderWindow>::New();

    // Setup renderer
    vtkSmartPointer<vtkRenderer> renderer =
            vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);

    renderer->AddViewProp(volume);
    renderer->ResetCamera();

    vtk->SetRenderWindow(renderWindow);
    renderWindow->Render();

    vtk->show();

}
#endif

void ImageForm::imageClick(QPointF pos)
{
    //  qDebug() << pos;
}

void ImageForm::mouseOverImage(QPointF pos)
{
    _pos = pos;
    QString str = QString("(%1 x %2 : [").arg((int)pos.x()).arg((int)pos.y());

    QList<unsigned> l =_interactor->getData(pos);
    for (int i = 0; i < l.size(); ++i)
        str += QString("%1 ").arg(l.at(i));
    str += "])";
    imagePosInfo = str;

    textItem2->setPlainText(imagePosInfo);
    // Need to fetch image infos...
    //  qDebug() << pos <<
}

void ImageForm::changeCurrentSelection()
{
    //  Well Name C1 (Z: z, t: t, F: f)
    imageInfos = QString("%1 (Z: %2, t: %3, F: %4)")
            .arg(_interactor->getSequenceFileModel()->Pos())
            .arg(_interactor->getZ())
            .arg(_interactor->getTimePoint())
            .arg(_interactor->getField());
    textItem->setPlainText(imageInfos);
    if (sz)
    {
        QList<ImageForm*> l = sz->currentSelection();
        //qDebug() << _interactor->getTimePoint() << "Selection changed" << l.size() << qApp->keyboardModifiers() ;
        //    if (!sz->isSelected(this))
        {
            if (!qApp->keyboardModifiers().testFlag(Qt::ControlModifier)
                    &&
                    !qApp->keyboardModifiers().testFlag(Qt::ShiftModifier))
            {
                sz->clearSelection();
                sz->select(this);
                modelView()->getOwner()->clearState(ExperimentFileModel::IsSelected);
                setSelectState(true);
            }
            else
            { // Modifiers
                if (qApp->keyboardModifiers().testFlag(Qt::ControlModifier))
                    sz->toggle(this);

                if (qApp->keyboardModifiers().testFlag(Qt::ShiftModifier))
                    sz->selectRange(this);
            }

        foreach (ImageForm* p, l)
            p->repaint();
        foreach (ImageForm* p, sz->currentSelection())
            p->repaint();
        }

        _interactor->setCurrent(_interactor);
        _view->getOwner()->clearState(ExperimentFileModel::IsCurrent);
        _view->getOwner()->setCurrent(_view->pos(), true);

        repaint();

    }

    //  if (!sz->currentSelection().isEmpty())
    emit interactorModified();
}


qreal ImageForm::gsiWidth(GraphicsSignItem::Signs sign)
{
    return gsi[sign]->mapToScene(gsi[sign]->boundingRect()).boundingRect().width();
}

qreal ImageForm::gsiHeight(GraphicsSignItem::Signs sign)
{
    return gsi[sign]->mapToScene(gsi[sign]->boundingRect()).boundingRect().height();
}


void ImageForm::closeEvent(QCloseEvent *event)
{
    SequenceViewContainer::getHandler().removeSequence(_view);//setCurrent(*si);
    event->accept();
}

void ImageForm::keyPressEvent(QKeyEvent *event)
{
    if (sz && event->key() == Qt::Key_Delete)
    {
        QList<ImageForm*> l = sz->currentSelection();
        foreach (ImageForm* p, l)
            p->removeFromView();
        //      removeFromView(); // Modify for selection suppression
    }
}

void ImageForm::timerEvent(QTimerEvent *event)
{
    // Use this to play videos....

        if (video_status == ImageForm::VideoStop)
        {
            return;
        }
        else if (video_status == ImageForm::VideoForward)
        {
            if (_interactor->getTimePoint() < _interactor->getTimePointCount())
                _interactor->setTimePoint(_interactor->getTimePoint() + 1);
            else
                _interactor->setTimePoint(1);
        }
        else
        {
            if (_interactor->getTimePoint() > 1)
                _interactor->setTimePoint(_interactor->getTimePoint() - 1);
            else
                _interactor->setTimePoint(_interactor->getTimePoint());
        }

        setPixmap(_interactor->getPixmap());
//        changeCurrentSelection();
        imageInfos = QString("%1 (Z: %2, t: %3, F: %4)")
                .arg(_interactor->getSequenceFileModel()->Pos())
                .arg(_interactor->getZ())
                .arg(_interactor->getTimePoint())
                .arg(_interactor->getField());
        textItem->setPlainText(imageInfos);
        this->repaint();
}



void ImageForm::on_ImageForm_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);


    QAction *copy = menu.addAction("Copy Image to clipboard", this, SLOT(copyToClipboard()));
    copy->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    //    copy->setDisabled(true);
    menu.addSeparator();
    QAction *remSeq = menu.addAction("Remove Sequence", this, SLOT(removeFromView()));
    menu.addSeparator();

    // FIXME: Add the proper action to pop images

    QAction *popDiag = menu.addAction(parent() ? "Popup Image" : "Insert Image", this, SLOT(popImage()));

    if (_interactor->getZCount() > 1)
    {
        // FIXME: Add the proper action to add a 3D Volume rendering of the current image
        QAction *popDiag = menu.addAction("3D Render");

#ifdef Checkout_With_VTK
        connect(popDiag, SIGNAL(triggered()), this, SLOT(display3DRendering()));
#else
        popDiag->setDisabled(true);
#endif
    }

    menu.exec(mapToGlobal(pos));
}

void ImageForm::copyToClipboard()
{
    QApplication::clipboard()->setPixmap(_interactor->getPixmap());
}

void ImageForm::popImage()
{
    this->mouseDoubleClickEvent(0x0);
}

void ImageForm::removeFromView()
{
    _interactor->clearMemory();
    if (sz) sz->widget()->layout()->removeWidget(this);
    this->close();
}
