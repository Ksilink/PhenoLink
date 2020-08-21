#include "graphicspixmapitem.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>

#include <QDebug>

GraphicsPixmapItem::GraphicsPixmapItem(QGraphicsItem *parent) :
  QGraphicsPixmapItem(parent)
{
  setFlags(QGraphicsItem::ItemIsMovable
           | QGraphicsItem::ItemSendsScenePositionChanges);
  setAcceptHoverEvents(true);
}


QVariant GraphicsPixmapItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
//  qDebug() << "Changing item..";
  if (change == ItemPositionChange && scene())
    {

      QPointF newPos = value.toPointF();
      //      qDebug() << "Item pos:" << pos() << "new pos"<< newPos;
      //      QRectF rect = scene()->sceneRect();
      QGraphicsView* v = scene()->views().first();

      QRectF rect = v->mapToScene(v->viewport()->rect()).boundingRect();


      QRectF br = mapToScene(boundingRect()).boundingRect();

      rect.setX(br.width()  > rect.width()  ? rect.right()-br.width() : 0);

      rect.setY(br.height() > rect.height() ? rect.bottom()-br.height() : 0);

      rect.setBottom(0);
      rect.setRight(0);

      if (!rect.contains(newPos))
        {
          newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
          newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
          return newPos;
        }


    }
  return QGraphicsPixmapItem::itemChange(change, value);
}

void GraphicsPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsPixmapItem::mousePressEvent(event);
//  event->ignore();
  emit mouseClick(event->pos());
}

//void GraphicsPixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
//{

//}

void GraphicsPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
  QGraphicsPixmapItem::hoverMoveEvent(event);
  emit mouseOver(event->pos());
}

void GraphicsPixmapItem::wheelEvent(QGraphicsSceneWheelEvent *event)
{ 
  event->accept();
  emit mouseWheel(event->delta());
}
