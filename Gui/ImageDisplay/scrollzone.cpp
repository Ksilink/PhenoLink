#include "scrollzone.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "flowlayout.h"
#include "imageform.h"

#include "Core/wellplatemodel.h"

#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>

#include <QPushButton>

#include <QProgressDialog>

ScrollZone::ScrollZone(QWidget *parent) :
  QScrollArea(parent)/*,
  _progDiag(0x0)*/
{

  QWidget* wid = new QWidget(this);
  wid->setLayout(new FlowLayout);
  wid->show();
  _wid = wid;
  setWidget(wid);

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setLineWidth(0);
  setFrameShadow(QFrame::Sunken);
  setFrameShape(QFrame::NoFrame);
  setWidgetResizable(true);
  setAcceptDrops(true);
}

void ScrollZone::setMainWindow(MainWindow *wi)
{
  _mainwin = wi;
}

void ScrollZone::removeSequences(QList<SequenceFileModel *> &lsfm)
{
  foreach (SequenceFileModel* sfm, lsfm)
    {
      QList<ImageForm*> lif = findChildren<ImageForm*>();
      foreach (ImageForm* imf, lif)
        {
          if (imf->modelView() == sfm)
            emit imf->close();
        }
    }
}

void ScrollZone::dragEnterEvent(QDragEnterEvent *event)
{
  //    qDebug() << event->mimeData()->formats();
  if (event->mimeData()->hasFormat("checkout/Wells"))
    event->acceptProposedAction();
}

void ScrollZone::dropEvent(QDropEvent *event)
{
  //  qDebug() << "Drop event" << this->objectName();

  if (event->mimeData()->hasFormat("checkout/Wells"))
    {
      event->acceptProposedAction();
      addSelectedWells();
      //      qDebug() << "Load Time: " << timer.elapsed();
    }


}

void ScrollZone::addSelectedWells()
{
  QList<SequenceFileModel*> sfm_list;
  int count = 0;

  QElapsedTimer timer;
  ScreensHandler& h = ScreensHandler::getHandler();
  Screens& s = h.getScreens();
  timer.start();

  SequenceFileModel* last = 0;

  for(Screens::iterator it = s.begin(), e = s.end(); it != e; ++it)
    {
      QList<SequenceFileModel*> seqs = (*it)->getSelection();

      for (QList<SequenceFileModel*> ::Iterator si = seqs.begin(), se = seqs.end(); si != se; ++si)
        {
          if ((*si)->isValid())
            {
              sfm_list << *si;
              //                  insertImage(*si);
              count ++;
              last = *si;
            }
        }

      (*it)->clearState(ExperimentFileModel::IsSelected);
    }

  if (last)
    {
      SequenceViewContainer::getHandler().setCurrent(last);
      //          _mainwin->updateCurrentSelection();
    }
//  if (!_progDiag) {
//      _progDiag = new QProgressDialog();
//    }
//  _progDiag->setLabelText("Loading Wells");
//  _progDiag->setMaximum(count);

  foreach (SequenceFileModel* m, sfm_list)
    insertImage(m);

//  _progDiag->close();
//  delete _progDiag;
//  _progDiag = 0;
}



void ScrollZone::setupImageFormInteractor(ImageForm* f)
{
  f->setScrollZone(this);
  f->connectInteractor();
  f->updateImage();

  connect(f, SIGNAL(interactorModified()), _mainwin , SLOT(updateCurrentSelection()));
  f->resize(256,216);
  _wid->layout()->addWidget(f);
  f->show();

  _wid->resize(_wid->size()+QSize(0,1));

//  if (_progDiag) _progDiag->setValue(_progDiag->value()+1);

}

void ScrollZone::imageInsertionWatcher()
{

  QFutureWatcher<ImageForm*>* watcher = dynamic_cast<QFutureWatcher<ImageForm*>* >(sender());

  if (!watcher) {
      qDebug() << "Error retreiving watchers finished state";
      return;
    }

  setupImageFormInteractor(watcher->future().result());
}

ImageForm* insertImageThreaded(SequenceFileModel* sfm, ImageForm* f)
{
  SequenceInteractor* intr =  new SequenceInteractor(sfm);
  intr->preloadImage();
  f->setModelView(sfm, intr);

  return f;
}


void ScrollZone::insertImage(SequenceFileModel* sfm)
{
  SequenceViewContainer::getHandler().addSequence(sfm);

  ImageForm* f = new ImageForm();
  _seq_toImg[sfm] = f;

  //  QFutureWatcher<ImageForm*> * watcher = new QFutureWatcher<ImageForm*>();
  //  connect(watcher, SIGNAL(finished()), this, SLOT(imageInsertionWatcher()));

  //  QFuture<ImageForm*> future = QtConcurrent::run(insertImageThreaded, sfm, f);
  //  watcher->setFuture(future);


  SequenceInteractor* intr =  new SequenceInteractor(sfm);
  intr->preloadImage();
  f->setModelView(sfm, intr);

  setupImageFormInteractor(f);
  f->scale(0);


  qApp->processEvents();
}

void ScrollZone::refresh(SequenceFileModel *sfm)
{
  //  _seq_toImg[sfm]->setModelView(sfm);
  _seq_toImg[sfm]->updateButtonVisibility();

  _seq_toImg[sfm]->redrawPixmap();
}

void ScrollZone::clearSelection()
{
  _selection.clear();
}

void ScrollZone::updateSelection()
{
  foreach (ImageForm* f, _selection)
    f->update();
}

QList<ImageForm *> ScrollZone::currentSelection()
{
  return _selection;
}

void ScrollZone::toggleRange(ImageForm *f)
{
  //   iterate the flow
  //    QList<ImageForm*> l = ((FlowLayout*)widget()->layout())->childs<ImageForm*>();
  QList<ImageForm*> l = ((FlowLayout*)_wid->layout())->childs<ImageForm*>();
  bool start = false;
  ImageForm* last = _selection.last();

  foreach (ImageForm* i, l)
    {
      if (i == f || i == last)
        start = !start;
      if (start)
        toggle(i);
    }

}

void ScrollZone::selectRange(ImageForm *f)
{
  //  QList<ImageForm*> l = ((FlowLayout*)widget()->layout())->childs<ImageForm*>();
  QList<ImageForm*> l = ((FlowLayout*)_wid->layout())->childs<ImageForm*>();


  bool start = false;
  ImageForm* last = _selection.last();

  foreach (ImageForm* i, l)
    {
      if (i == f || i == last)
        start = !start;

      if (start && !_selection.contains(i))
        select(i);
    }
  select(f);
}

void ScrollZone::select(ImageForm *f)
{
  //  qDebug() <<" Select" << f->contentPos();
  f->setSelectState(true);
  _selection << f;
}

void ScrollZone::toggle(ImageForm *f)
{
  //  qDebug() <<"Toggle" << f->contentPos();
  if (_selection.contains(f))
    {
      _selection.removeAll(f);
      f->setSelectState(false);

    }
  else
    {
      _selection << f;
      f->setSelectState(true);
    }
}

void ScrollZone::unselect(ImageForm *f)
{
  _selection.removeAll(f);
  f->setSelectState(false);

}

bool ScrollZone::isSelected(ImageForm *f)
{
  return _selection.contains(f);
}
