#ifndef GRAPHICSPIXMAPITEM_H
#define GRAPHICSPIXMAPITEM_H

#include <QGraphicsPixmapItem>
#include <QObject>



// FIXME: Modify the handling pixmap item
// A proper rendering of the sequence interactor would be better off
class GraphicsPixmapItem : public QObject, public QGraphicsPixmapItem
{  
  Q_OBJECT
public:
  explicit GraphicsPixmapItem(QGraphicsItem *parent = 0);


protected:
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
//  virtual void	mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
  virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
  virtual void wheelEvent(QGraphicsSceneWheelEvent * event) ;
signals:

 void mouseClick(QPointF pos);
 void mouseOver(QPointF pos);
 void mouseWheel(int delta);

public slots:

};

#endif // GRAPHICSPIXMAPITEM_H
