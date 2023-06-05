#include "imagegraphicsview.h"
#include <QMouseEvent>



//#include <QtOpenGL/QGLWidget>

ImageGraphicsView::ImageGraphicsView(QWidget *parent) :
  QGraphicsView(parent)
{
//  setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
//  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

ImageGraphicsView::ImageGraphicsView(QGraphicsScene *scene, QWidget *parent):
  QGraphicsView(scene,parent)
{
//  setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
  //  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

//void ImageGraphicsView::mouseDoubleClickEvent(QMouseEvent *event)
//{

//}

//void ImageGraphicsView::mouseMoveEvent(QMouseEvent *event)
//{
////  Q_UNUSED(event);
////    event->pos();
//  event->ignore();
//}
