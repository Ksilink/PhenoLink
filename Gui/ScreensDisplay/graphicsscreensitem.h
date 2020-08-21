#ifndef GRAPHICSSCREENSITEM_H
#define GRAPHICSSCREENSITEM_H


#include <QGraphicsRectItem>
#include <QPainter>
#include <QGraphicsScene>


#include "Core/wellplatemodel.h"

class ScreensGraphicsView;
class WellRepresentationItem;
class WellColorisation;

class GraphicsScreensItem: public QObject, public QGraphicsRectItem
{
  Q_OBJECT
public:
  explicit GraphicsScreensItem(QGraphicsItem * parent = 0);

  void setColumnItem(QGraphicsItemGroup *col);
  void setRowItem(QGraphicsItemGroup* row);

  QRectF constructWellRepresentation(ExperimentFileModel *mdl, ScreensGraphicsView* view);
  ExperimentFileModel *currentRepresentation();

  void updateColorisationModel(WellColorisation* cols);
  WellColorisation* colorisationModel();

protected:
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

protected slots:

  void	Handmode(bool checked);
  void innerSelectionChanged();

signals:
  void selectionChanged();

protected:
  QGraphicsItemGroup *_row, *_col;
  //  Screens _models;
  ExperimentFileModel* _models;

  QList<WellRepresentationItem*> _items;
  WellColorisation* _coloriser;

};


#endif // GRAPHICSSCREENSITEM_H
