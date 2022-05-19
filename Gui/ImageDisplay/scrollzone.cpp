#include "scrollzone.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "flowlayout.h"
#include "imageform.h"
#include <Core/checkouterrorhandler.h>
#include "Core/wellplatemodel.h"

#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>

#include <QPushButton>

#include <QProgressDialog>
#include <QMessageBox>


ScrollZone::ScrollZone(QWidget *parent) :
    QScrollArea(parent),
    _progDiag(nullptr)
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
        sfm->setAsShowed(false);
        sfm->removeMeta();

        QList<ImageForm*> lif = findChildren<ImageForm*>();
        foreach (ImageForm* imf, lif)
        {
            if (imf->modelView() == sfm)
            {
                imf->close();
                _selection.removeAll(imf);
                delete imf;
            }
        }
        _seq_toImg.remove(sfm);

    }

    _mainwin->resetSelection();

}


void ScrollZone::removeImageForm(ImageForm* im)
{

    blockSignals(true);

    _selection.removeAll(im);
    QList<SequenceFileModel*> sfm;
    for (auto it =  _seq_toImg.begin(), e = _seq_toImg.end(); it != e; ++it)
        if (it.value() == im)
        {
            sfm << it.key();
            it.key()->setAsShowed(false);
        }
    for (auto s: sfm)
        _seq_toImg.remove(s);
    // qDebug() << "Removing Image Form" << im << sfm;
    blockSignals(false);

    _mainwin->resetSelection();
}


void ScrollZone::dragEnterEvent(QDragEnterEvent *event)
{
    //    qDebug() << event->mimeData()->formats();
    if (event->mimeData()->hasFormat("checkout/Wells"))
        event->acceptProposedAction();
}

void ScrollZone::dropEvent(QDropEvent *event)
{

    _mainwin->graySelection();

    static int groupId = 0;
    //  qDebug() << "Drop event" << this->objectName();

    QString key = _mainwin->workbenchKey();
    if (event->keyboardModifiers() == Qt::ControlModifier)
    {
        key += QString("%1").arg(groupId);
        groupId++;
    }

    //    qDebug() << "Loading with key" << key;

    ImageInfos::key(key);

    if (event->mimeData()->hasFormat("checkout/Wells"))
    {
        event->acceptProposedAction();
        addSelectedWells();
        //      qDebug() << "Load Time: " << timer.elapsed();
    }
    ImageInfos::key(_mainwin->workbenchKey()); // Reset to workbench key


    if (CheckoutErrorHandler::getInstance().hasErrors())
    {
        QString err = CheckoutErrorHandler::getInstance().getErrors();
        qDebug() << "Error while loading image occured !" << err;
        err.truncate(80);
        QMessageBox::critical(this, "Drop file yielded errors", err);
        CheckoutErrorHandler::getInstance().resetErrors();
    }


    _mainwin->ungraySelection();

}

static QMutex mutex;

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
    if (!_progDiag) {
        _progDiag = new QProgressDialog(this);
    }
    _progDiag->setLabelText("Loading Wells");
    _progDiag->setMaximum(2*count);

    QList< SequenceInteractor*> all;
    for (auto &sfm : sfm_list)
    {
        SequenceInteractor* intr = new SequenceInteractor(sfm, QString("0"));
        intr->preloadImage();


        _progDiag->setValue(_progDiag->value()+1);
        all <<  intr;
        qApp->processEvents();
    }

    // =  QtConcurrent::blockingMapped(sfm_list, InsertFctor(this, _progDiag));
    _progDiag->setLabelText("Displaying Wells");
    foreach(SequenceInteractor* m, all)
    {
        insertImage(m->getSequenceFileModel(), m);
        _progDiag->setValue(_progDiag->value()+1);
        qApp->processEvents();
    }
    _progDiag->close();
    delete _progDiag;
    _progDiag = 0;
}

int ScrollZone::items()
{
    return  findChildren<ImageForm*>().size();
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
}

void ScrollZone::insertImage(SequenceFileModel* sfm, SequenceInteractor* iactor)
{
    SequenceViewContainer::getHandler().addSequence(sfm);

    ImageForm* f = new ImageForm(this, !sfm->hasProperty("unpack"));
    _seq_toImg[sfm] = f;

    SequenceInteractor* intr =  iactor ? iactor : new SequenceInteractor(sfm, QString("0"));
    f->setModelView(sfm, intr);

    setupImageFormInteractor(f);
    f->scale(0);

    connect(f, SIGNAL(overlayInfos(QString, QString,int)),
            _mainwin, SLOT(change_overlay_details(QString, QString, int)));

}


SequenceInteractor* ScrollZone::getInteractor(SequenceFileModel* mdl)
{
    if (_seq_toImg.contains(mdl))
        return _seq_toImg[mdl]->getInteractor();

    return nullptr;
}

void ScrollZone::refresh(SequenceFileModel *sfm)
{
    if (_seq_toImg.contains(sfm))
    {
        _seq_toImg[sfm]->updateButtonVisibility();
        _seq_toImg[sfm]->redrawPixmap();
    }
}

void ScrollZone::clearSelection()
{
    _selection.clear();
}

void ScrollZone::updateSelection()
{
    for (ImageForm* f: _selection)
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

    if (_selection.size())
    {
        bool start = false;
        ImageForm* last = _selection.last();

        foreach(ImageForm * i, l)
        {
            if (i == f || i == last)
                start = !start;

            if (start && !_selection.contains(i))
                select(i);
        }
        select(f);
    }
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
