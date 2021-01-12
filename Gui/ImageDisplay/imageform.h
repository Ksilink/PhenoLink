#ifndef IMAGEFORM_H
#define IMAGEFORM_H


#include <QGraphicsView>
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>

#include <QPointF>

#include "ImageDisplay/graphicssignitem.h"

#include "Core/wellplatemodel.h"
#include <Core/sequenceinteractor.h>

namespace Ui {
class ImageForm;
}

class GraphicsPixmapItem;
class ScrollZone;
class SequenceInteractor;

// FIXME: Replace the image display widget from a QGraphicsView
// to a basic QWidget with proper scale & handling of the viewed data
class ImageForm : public QWidget, public CoreImage
{
    Q_OBJECT

    enum VideoPlay {VideoStop, VideoForward, VideoBackward } video_status;
public:
    explicit ImageForm(QWidget *parent = 0, bool packed = true);
    ~ImageForm();

    void setPixmap(QPixmap pix);

    void setModelView(SequenceFileModel* view, SequenceInteractor *interactor = 0);

    SequenceFileModel* modelView();

    virtual QSize	sizeHint() const;
    //  ImageForm* getCurrent();


    void connectInteractor();
    QString contentPos();
    void updateImage();
    virtual void modifiedImage();
    virtual void changeFps(double fps);

    void setScrollZone(ScrollZone* z);
    void setSelectState(bool val);
    void updateButtonVisibility();

    QStringList getChannelNames();

    void updateDecorator(QList<QGraphicsItem *> decors);
protected:
    virtual void resizeEvent(QResizeEvent * event) ;
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    virtual void paintEvent(QPaintEvent *event);

signals:
    void interactorModified();

protected slots:

    void minusClicked();
    void plusClicked();
    void prevImClicked();
    void nextImClicked();

    void sliceUpClicked();
    void sliceDownClicked();

    void nextFrameClicked();
    void prevFrameClicked();
    void FwdPlayClicked();
    void BwdPlayClicked();

#ifdef Checkout_With_VTK
    void display3DRendering();
#endif

    void imageClick(QPointF pos);
    void imageDoubleClick(QPointF pos);
    void imageMouseMove(QPointF pos);
    void mouseOverImage(QPointF pos);
    void watcherPixmap();
protected:


    qreal gsiWidth(GraphicsSignItem::Signs sign);
    qreal gsiHeight(GraphicsSignItem::Signs sign);

    virtual void closeEvent(QCloseEvent* event);
    virtual void keyPressEvent(QKeyEvent * event);
    virtual void timerEvent(QTimerEvent *event);

private slots:

    void on_ImageForm_customContextMenuRequested(const QPoint &pos);
    void copyToClipboard();
    void copyCurrentImagePath();
    void copyCurrentSequencePath();
    void overlayHistogram();
    void saveVideo();
    void changePacking();
    void refinePacking();
    void biasCorrection();
    void popImage();
    void removeFromView();


public slots:

    void changeCurrentSelection();
    void redrawPixmap();
    void scale(int delta);


    void redrawPixmap(QPixmap img);
private:
    bool packed,wasPacked;
    bool bias_correction;

    ScrollZone* sz;

    Ui::ImageForm *ui;

    SequenceFileModel* _view;
    SequenceInteractor* _interactor;

    QPixmap _pix;
    GraphicsPixmapItem *pixItem;
    QString imageInfos;
    QString imagePosInfo;

    GraphicsSignItem* gsi[GraphicsSignItem::Count];
    QGraphicsTextItem* textItem;
    QGraphicsTextItem* textItem2;

    QPointF _pos;

    // Fir taking measurements over image
    QPointF _size_start, _size_end;
    bool _moving;
    QGraphicsLineItem* _ruler;
    int playTimerId;

    QList<QGraphicsItem *> _decorators;

    // User Settings
    double scaleFactor, aspectRatio, currentScale;

    //  static ImageForm* _selectedForm;
    bool isRunning, fromButton;
    int timer_id;

    //  static SequenceInteractor* _current_interactor;
    Q_DISABLE_COPY(ImageForm);
};

#endif // IMAGEFORM_H
