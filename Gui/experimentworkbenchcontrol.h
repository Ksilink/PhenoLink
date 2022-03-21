#ifndef EXPERIMENTWORKBENCHCONTROL_H
#define EXPERIMENTWORKBENCHCONTROL_H

#include <QWidget>
#include <Core/sequenceinteractor.h>

#include "screensmodel.h"

#include "Core/Dll.h"
#include <QStyledItemDelegate>

class ImageForm;
class ScreensModel;
class QTableWidget;
class QCheckBox;
class ScrollZone;
class QTreeWidget;
class QTableView;
class ctkColorPickerButton;
class ctkDoubleRangeSlider;
class QComboBox;
class GraphicsScreensItem;
class QSpinBox;
class ctkCheckableComboBox;

class ExperimentWorkbenchControl: public QWidget
{
  Q_OBJECT
public:

  ExperimentWorkbenchControl(ExperimentFileModel* mdl,GraphicsScreensItem* gfx, QWidget* parent = 0);

  QTreeWidget* getXPTree();
  QWidget *createComboBox(QStringList list, int max);


  void fillSelector(QList<QSpinBox*> fi, QVector<QList<int> > &select, int pos);
protected:


protected slots:


  void colorizer_change(QString txt);
  void features_tree_selectionChanged();
  void featuresFusionModeChanged(int index);

  void featuresSpinBoxChanged(int value);
  void updateModel(bool dataChanged = false);
  void perform_lda(MatrixDataModel *mdm);
  void perform_pca(MatrixDataModel *mdm);

protected:

  ExperimentFileModel* _mdl;
  GraphicsScreensItem* gfx;

  QWidget* _choice_container; // hide if LDA
  QWidget* _tags_container; // hide if LDA


  ctkCheckableComboBox* _selected_tags;
  QComboBox* _coloriz;
  QTreeWidget* _xpTree, *_xpTree2;
  ctkColorPickerButton* _colorButton;

  ctkColorPickerButton* _colorButton2; // hide if PCA

  ctkDoubleRangeSlider* _rangeDisplay;
  QCheckBox* _inverseColor;
};



#endif // EXPERIMENTWORKBENCHCONTROL_H
