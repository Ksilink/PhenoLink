#ifndef SCREENSGRAPHICSVIEW_H
#define SCREENSGRAPHICSVIEW_H

#include <QGraphicsView>

class WellRepresentationItem;
class ExperimentFileModel;

class ScreensGraphicsView : public QGraphicsView
{
  Q_OBJECT

public:
  ScreensGraphicsView(QWidget * parent = 0);

  void setExperimentModel(ExperimentFileModel* mdl) { _mdl = mdl; }
  ExperimentFileModel* getExperimentModel() { return _mdl; }

protected:
  virtual void	resizeEvent(QResizeEvent * event);

public slots:
  void wellSelected(QPoint point);

protected:
  ExperimentFileModel* _mdl;
//protected:
//  QMap<QPoint, WellRepresentationItem*> _wells;
};

#endif // SCREENSGRAPHICSVIEW_H
