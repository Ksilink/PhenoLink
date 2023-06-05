#ifndef IMAGEGRAPHICSVIEW_H
#define IMAGEGRAPHICSVIEW_H

#include <QGraphicsView>

class ImageGraphicsView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit ImageGraphicsView(QWidget *parent = 0);

  explicit ImageGraphicsView(QGraphicsScene * scene, QWidget * parent = 0);



signals:

public slots:

//protected:
//  virtual void mouseMoveEvent(QMouseEvent* event);

};

#endif // IMAGEGRAPHICSVIEW_H
