#ifndef GRAPHICSSIGNITEM_H
#define GRAPHICSSIGNITEM_H

#include <QGraphicsPixmapItem>

#include <QGraphicsObject>

// Circled Plus or Minus sign items
// Next/Prev slice, image
// Time lapse play

/// FIXME: Change the Inheritance from QGraphicsItemGroup to QGraphicsItem
// And reimplements the patin method instead of using multiple path items...
// Would be cleaner, and would reuse the propert style properties
class GraphicsSignItem: public QGraphicsObject
{
  Q_OBJECT
public:

    enum Signs {
        Minus, Plus, // Zoom funcs
        FwdPlay, BwdPlay, NextFrame, PrevFrame,  // Time Lapses play
        PrevIm, NextIm, // Well browse
        SliceUp,SliceDown, // 3D Slices
        Count // For boundary, keep this one at the end
    };

    explicit GraphicsSignItem(QGraphicsItem* parent = 0);
    explicit GraphicsSignItem(Signs sig, QGraphicsItem* parent = 0);

//    void prepare();

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
    virtual QRectF boundingRect() const;

signals:
    void mouseClick();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void timerEvent(QTimerEvent * event);
protected:


    Signs _sig;
//    QGraphicsEllipseItem *ell;
//    QGraphicsPolygonItem *pol, *pol2;

};



#endif // GRAPHICSSIGNITEM_H
