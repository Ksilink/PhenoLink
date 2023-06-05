#ifndef WELLCOLORISATION_H
#define WELLCOLORISATION_H

#include <QObject>
#include <QColor>
#include <QItemSelection>

class QPainter;
class ExperimentFileModel;

class MatrixDataModel;

struct WellColorisationControl
{
  double  min,max;
  bool    inverted;

  QColor colorSet;

};

class WellColorisation : public QObject
{
  Q_OBJECT
public:
  explicit WellColorisation(QObject *parent = 0);

  void paint(QPainter* painter, QPoint pos, QRectF &rect);

  void setDataModel(MatrixDataModel* mdl);
  void updateControl(double min, double max, bool isInverted, QColor color, QColor color2);


  MatrixDataModel* getDataModel();
signals:

public slots:
  void data_selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
protected:
//  ExperimentFileModel * _efm;
  MatrixDataModel* model;

  double  min,max;
  bool    inverted;

  QColor color, _color2;
};


//class WellColorisationLDA: public WellColorisation
//{
//    Q_OBJECT
//public:
//    explicit WellColorisationLDA(QObject *parent = 0);

//};


#endif // WELLCOLORISATION_H
