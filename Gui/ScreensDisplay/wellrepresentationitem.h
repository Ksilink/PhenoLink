#ifndef WELLREPRESENTATIONITEM_H
#define WELLREPRESENTATIONITEM_H

#include <QGraphicsObject>
#include "Core/Dll.h"


class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class GraphicsScreensItem;
class WellColorisation;


class DllGuiExport WellRepresentationItem : public QGraphicsObject
{
  Q_OBJECT
public:

  WellRepresentationItem(QGraphicsItem * parent = 0);
  WellRepresentationItem(qreal x, qreal y, qreal w, qreal h, QGraphicsItem * parent = 0);

  QRectF boundingRect() const;


  void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);


  void setBrush(QBrush br);

  void setWellPosition(QPoint p);

  QPoint getWellPosition();

  void setGraphicsScreensItem(GraphicsScreensItem* g);

  void setColorizer(WellColorisation* c);

protected:
  virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);

  void tagSelection(QString tag);
  void untagSelection(QString tag);

signals:
  void selectionChanged();

public  slots:

  void tagData();
  void untagData();

   void addTag();

protected:
  QPoint _wellPosition;
  QRectF _rect,_brect;
  QBrush _brush;

  bool   _drag;
  QPoint _dragStartPosition;

  GraphicsScreensItem* _gsi; // Object to handle the current view over the screens
  WellColorisation *_colorizer;

};

#endif // WELLREPRESENTATIONITEM_H
