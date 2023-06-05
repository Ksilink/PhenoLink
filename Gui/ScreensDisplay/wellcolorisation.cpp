#include "wellcolorisation.h"

#include <Core/wellplatemodel.h>

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QRectF>

WellColorisation::WellColorisation(QObject *parent) :
  QObject(parent),
  model(0)
{
    setObjectName("PCA");
}

void WellColorisation::paint(QPainter *painter, QPoint pos, QRectF& rect)
{
  // apply weighting to color
  QColor col = color;
  double val = 0;

  if (model->isActive(pos.x(), pos.y()))
    val = model->data(pos.x(), pos.y());

  double cval = std::min(255., std::max(0., 255 * (inverted
       ? 1. - (val-min)/(max-min) :
         (val-min)/(max-min))));

  col.setHsv(col.hue(), col.saturation(), cval);

  QBrush brush(col);
  painter->setBrush(brush);
  painter->drawRect(rect);

  QString tA = model->getTag(model->pointToPos(pos));
  painter->drawText(rect.adjusted(1,1,-1,-1), tA);


}

void WellColorisation::setDataModel(MatrixDataModel *mdl)
{
  model =  mdl;
}

void WellColorisation::updateControl(double min, double max, bool isInverted, QColor color, QColor color2)
{
    this->min = min;
    this->max = max;
    this->inverted = isInverted;
    this->color = color;
    this->_color2 = color2;
}



MatrixDataModel *WellColorisation::getDataModel()
{
  return model;
}



void WellColorisation::data_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
}

//WellColorisationLDA::WellColorisationLDA(QObject *parent)
//    : WellColorisation(parent)
//{
//    setObjectName("LDA");
//}
