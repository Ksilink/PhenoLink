#include "graphicssignitem.h"

#include <QStyle>
#include <QStyleOption>

#include <QBrush>
#include <QPen>

#include <QTimerEvent>
#include <QPainter>
#include <QPainterPath>

GraphicsSignItem::GraphicsSignItem(QGraphicsItem *parent):
  QGraphicsObject(parent), _sig(Minus)
{
  setZValue(1); // stack on top of image
  setAcceptedMouseButtons(Qt::LeftButton);
  setAcceptHoverEvents(true);
//  prepare();

  startTimer(500);
}

GraphicsSignItem::GraphicsSignItem(Signs sig, QGraphicsItem *parent):
  QGraphicsObject(parent), _sig(sig)
{
  setZValue(1); // stack on top of image
  setAcceptedMouseButtons(Qt::LeftButton);
  setAcceptHoverEvents(true);

//  prepare();

  startTimer(500);
}

/*void GraphicsSignItem::prepare()
{

    pol = new QGraphicsPolygonItem();
    QPolygonF poly, sq;

    switch (_sig)
    {
    case Plus:
        poly << QPointF(2,6) << QPointF(6,6) << QPointF(6,2) << QPointF(10, 2) << QPointF(10,6) << QPointF(14,6)
             << QPointF(14, 10) << QPointF(10,10) << QPointF(10, 14) << QPointF(6,14) << QPointF(6,10) << QPointF(2,10);
        break;
    case PrevIm: // Triangle Pointing Left
        poly << QPointF(14,2) << QPointF(14,14) << QPointF(2,8);
        break;
    case NextIm: // Triangle Pointing Right
        poly << QPointF(2,2) << QPointF(2,14) << QPointF(14,8);
        break;
    case FwdPlay: // Right Pointing Triangle with vertical bar (Play/pause)
        poly << QPointF(2,2) << QPointF(2,14) << QPointF(14,8);

        sq << QPointF(16,2) << QPointF(20,2) << QPointF(20,14) << QPointF(16,14);
        break;
    case BwdPlay: // Left Pointing Triangle with vertical bar
        poly << QPointF(14,2) << QPointF(14,14) << QPointF(2,8);
        sq << QPointF(-4,2) << QPointF(0,2) << QPointF(0,14) << QPointF(-4,14);
        break;
    case NextFrame: // Double Right Pointing Triangle
    {
        poly << QPointF(2,2) << QPointF(2,14) << QPointF(14,8);
        QPolygonF p = poly.translated(8,0);
        poly = poly.united(p);
    }  break;
    case PrevFrame:// Double Left Pointing Triangle
    {   poly << QPointF(14,2) << QPointF(14,14) << QPointF(2,8);
        QPolygonF p = poly.translated(8,0);
        poly = poly.united(p);
    }  break;
    case SliceUp: // Triangle Pointing up
        poly << QPointF(2,14) << QPointF(14,14) << QPointF(8,2);
        break;
    case SliceDown: // Triangle Pointing down
        poly << QPointF(2,2) << QPointF(14,2) << QPointF(8,14);
        break;
    case Minus:
    default:
        poly << QPointF(2,6) << QPointF(14,6)
             << QPointF(14, 10) << QPointF(2,10);

    }

    QStyleOption option(QStyleOption::Version, QStyleOption::SO_Button);
    pol->setPen(option.palette.mid().color());
    pol->setBrush(option.palette.button());
    pol->setZValue(3);
    //  QBrush brush(option.palette.mid());
    //  QPen pen;


    pol->setPolygon(poly);
    addToGroup(pol);

    if (_sig == Minus || _sig == Plus)
    {
        ell = new QGraphicsEllipseItem();
        ell->setRect(QRectF(0,0,16,16));

        ell->setPen(option.palette.mid().color());
        ell->setBrush(option.palette.button());
        ell->setZValue(2);
        addToGroup(ell);
    }

    if (_sig == FwdPlay || _sig == BwdPlay)
    {
        pol2 = new QGraphicsPolygonItem();
        pol2->setPen(option.palette.mid().color());
        pol2->setBrush(option.palette.button());
        pol2->setZValue(3);
        pol2->setPolygon(sq);
        addToGroup(pol2);
    }

} */

void GraphicsSignItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (!isVisible()) return;

  QPolygonF poly, sq;

  switch (_sig)
    {
    case Plus:
      poly << QPointF(2,6) << QPointF(6,6) << QPointF(6,2) << QPointF(10, 2) << QPointF(10,6) << QPointF(14,6)
           << QPointF(14, 10) << QPointF(10,10) << QPointF(10, 14) << QPointF(6,14) << QPointF(6,10) << QPointF(2,10);
      break;
    case PrevIm: // Triangle Pointing Left
      poly << QPointF(14,2) << QPointF(14,14) << QPointF(2,8);
      break;
    case NextIm: // Triangle Pointing Right
      poly << QPointF(2,2) << QPointF(2,14) << QPointF(14,8);
      break;
    case FwdPlay: // Right Pointing Triangle with vertical bar (Play/pause)
      poly << QPointF(2,2) << QPointF(2,14) << QPointF(14,8);

      sq << QPointF(16,2) << QPointF(20,2) << QPointF(20,14) << QPointF(16,14);
      break;
    case BwdPlay: // Left Pointing Triangle with vertical bar
      poly << QPointF(14,2) << QPointF(14,14) << QPointF(2,8);
      sq << QPointF(-4,2) << QPointF(0,2) << QPointF(0,14) << QPointF(-4,14);
      break;
    case NextFrame: // Double Right Pointing Triangle
      {
        poly << QPointF(2,2) << QPointF(2,14) << QPointF(14,8);
        QPolygonF p = poly.translated(8,0);
        poly = poly.united(p);
      }  break;
    case PrevFrame:// Double Left Pointing Triangle
      {   poly << QPointF(14,2) << QPointF(14,14) << QPointF(2,8);
        QPolygonF p = poly.translated(8,0);
        poly = poly.united(p);
      }  break;
    case SliceUp: // Triangle Pointing up
      poly << QPointF(2,14) << QPointF(14,14) << QPointF(8,2);
      break;
    case SliceDown: // Triangle Pointing down
      poly << QPointF(2,2) << QPointF(14,2) << QPointF(8,14);
      break;
    case Minus:
    default:
      poly << QPointF(2,6) << QPointF(14,6)
           << QPointF(14, 10) << QPointF(2,10);

    }


  painter->setPen(option->palette.mid().color());
  painter->setBrush(option->palette.button());

  if (_sig == Minus || _sig == Plus)
    {
      painter->drawEllipse(QRectF(0,0,16,16));
    }

  if (_sig == FwdPlay || _sig == BwdPlay)
    {
      painter->drawPolygon(sq);
    }

  painter->drawPolygon(poly);
}

// BUG: Need to adapt the bounding rect size to the proper component,
// the proper dimensions are crucial for proper placement of the signs
QRectF GraphicsSignItem::boundingRect() const
{
  return QRectF(-4,-4,20,20);
}

void GraphicsSignItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  Q_UNUSED(event);

  emit mouseClick();
}

void GraphicsSignItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{ // Change display style when hovering the item
  Q_UNUSED(event);
  setOpacity(1.0);
}

void GraphicsSignItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{ // Change display style when leaving the hovering
  Q_UNUSED(event);
  startTimer(500);
}

void GraphicsSignItem::timerEvent(QTimerEvent *event)
{
  float opacity = this->opacity();

  if (opacity > 0.1)
    {
      setOpacity(opacity-0.1);
    }
  else
    {
      killTimer(event->timerId());
    }
}

