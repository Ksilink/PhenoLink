#include "screensgraphicsview.h"



ScreensGraphicsView::ScreensGraphicsView(QWidget *parent):
  QGraphicsView(parent)
{
  setDragMode(QGraphicsView::RubberBandDrag);
}

void ScreensGraphicsView::resizeEvent(QResizeEvent *event)
{

}

// Modify the selected well to redraw it
void ScreensGraphicsView::wellSelected(QPoint point)
{

}
