#include <QPainter>
#include <QMenu>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>

#include "wellplatemodel.h"
#include "wellrepresentationitem.h"
#include <QApplication>

#include "graphicsscreensitem.h"
#include "Core/wellplatemodel.h"
#include <QDrag>

#include <ScreensDisplay/wellcolorisation.h>

#include <QMimeData>
#include <QInputDialog>
#include <QSettings>
#include <QDir>

WellRepresentationItem::WellRepresentationItem(QGraphicsItem *parent):
    QGraphicsObject(parent), _colorizer(0)
{
    setCursor(Qt::ArrowCursor);
    //  setFlags(QGraphicsItem::ItemIsSelectable);
}

WellRepresentationItem::WellRepresentationItem(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent):
    QGraphicsObject(parent), _rect(x,y,w,h), _brect(x-2,y-2,w+4,h+4), _colorizer(0)
{
    setCursor(Qt::ArrowCursor);
    //  setFlags(QGraphicsItem::ItemIsSelectable);
}


/// NOTE: Adapt to properly handle the Data Model presentation
void WellRepresentationItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    //  option->palette.

    if (_colorizer)
    {
        _colorizer->paint(painter, _wellPosition, _rect);
    }
    else
    {
        painter->setBrush(_brush);
        painter->setPen(Qt::black);
        painter->drawRect(_rect);
    }
    // Paint borders for selection
    ExperimentFileModel * efm = _gsi->currentRepresentation();
    if (!efm) return;
    //  qDebug() << "WellRepresentationItem paint (" << _wellPosition << ")";

    //  efm
    QPen pen = painter->pen();
    pen.setColor(Qt::blue);

    if (efm->isCurrent(_wellPosition))
    {
        //      qDebug() << "Current";
        pen.setWidthF(2);
        painter->setPen(pen);
        painter->drawLine(_brect.topLeft(), _brect.topRight());
        painter->drawLine(_brect.topLeft(), _brect.bottomLeft());
        painter->drawLine(_brect.topRight(), _brect.bottomRight());
        painter->drawLine(_brect.bottomLeft(), _brect.bottomRight());
    }
    else
        if (/*isSelected()*/ efm->isSelected(_wellPosition)){
            //      qDebug() << "Not Current";
            pen.setWidthF(2);
            painter->setPen(pen);

            //      if (!efm->isSelected(QPoint(_wellPosition.x(), _wellPosition.y()-1)))
            {
                painter->drawLine(_brect.topLeft(), _brect.topRight());
            }
            //      if (!efm->isSelected(QPoint(_wellPosition.x()-1, _wellPosition.y())))
            {
                painter->drawLine(_brect.topLeft(), _brect.bottomLeft());
            }
            //      if (!efm->isSelected(QPoint(_wellPosition.x()+1, _wellPosition.y())))
            {
                painter->drawLine(_brect.topRight(), _brect.bottomRight());
            }
            //      if (!efm->isSelected(QPoint(_wellPosition.x(), _wellPosition.y()+1)))
            {
                painter->drawLine(_brect.bottomLeft(), _brect.bottomRight());
            }
        }

    // Draw Tag
    QStringList l = efm->getTags(_wellPosition);
    if (!l.isEmpty())
    {
        QString tA; // Tag Acronyms
        l.sort();
//        foreach (QString a, l)
//            tA.append(a.at(0)+a.at(1)+",");
        tA=l.join(",");
        QFont font = painter->font();
        font.setPixelSize(6);
        painter->setFont(font);
        painter->drawText(_rect.adjusted(1,1,-1,-1), tA);
    }

}

void WellRepresentationItem::setBrush(QBrush br)
{
    _brush = br;
}

void WellRepresentationItem::setWellPosition(QPoint p)
{
    _wellPosition = p;
}

QPoint WellRepresentationItem::getWellPosition()
{
    return _wellPosition;
}

void WellRepresentationItem::setGraphicsScreensItem(GraphicsScreensItem *g)
{
    _gsi = g;
}

void WellRepresentationItem::setColorizer(WellColorisation *c)
{  
    _colorizer = c;

    update(); // colorizer changed => the data needs to be redrawn
}

void WellRepresentationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() & Qt::LeftButton)
    {
        _dragStartPosition = event->screenPos();
        ExperimentFileModel* r =  _gsi->currentRepresentation();
        if (r)
        {
            r->clearState(ExperimentFileModel::IsSelected);
            //          r->setCurrent(_wellPosition, true);
            //          qDebug() << "mouse Press"         << _wellPosition;
            //          emit selectionChanged();

        }
        //      if (r)
        //        {
        //          r->select(_wellPosition, isSelected());
        //          event->accept();
        //        }
    }
    if (event->button() & Qt::RightButton)
    {
        QMenu menu;

        QSettings set;
        QStringList tt = set.value("Tags", QStringList()).toStringList();
        if (tt.size())
        {
            QMenu* m = menu.addMenu("Tag");

            foreach (QString st, tt)
            {
                m->addAction(st, this, SLOT(tagData()));

            }
        }
        // Add action to add tag && tag the selected data :D
        menu.addAction("Create Tag", this, SLOT(addTag()));
        if (tt.size())
        {
            QMenu* m = menu.addMenu("Untag");

            foreach (QString st, tt)
            {
                m->addAction(st, this, SLOT(untagData()));

            }
        }
        menu.exec(event->screenPos());
    }
    QGraphicsObject::mousePressEvent(event);
    //  event->accept();
}

void WellRepresentationItem::tagData()
{
    QAction* send = qobject_cast<QAction*>(sender());
    if (send)
        tagSelection(send->text());
}

void WellRepresentationItem::untagData()
{
    QAction* send = qobject_cast<QAction*>(sender());
    if (send)
        untagSelection(send->text());
}

void WellRepresentationItem::addTag()
{
    // Txt message box with tag name,
    // if ok, add to tag list and tag current data :)

    bool ok;
    QString text = QInputDialog::getText(nullptr, tr("Add new tag name"),
                                         tr("Tag Name:"), QLineEdit::Normal,
                                         QString(), &ok);
    if (ok && !text.isEmpty())
    {

        QSettings set;
        QStringList tt = set.value("Tags", QStringList()).toStringList();

        if (!tt.contains(text))
        {
            tt << text;
            set.setValue("Tags", tt);
        }
        tagSelection(text);
    }
}

void WellRepresentationItem::tagSelection(QString tag)
{
    ExperimentFileModel* r =  _gsi->currentRepresentation();
    // Iterate trough selection
    QList<SequenceFileModel*> sel = r->getSelection();

    foreach (SequenceFileModel* f, sel) f->setTag(tag);

    // Update  tag file
    QList<SequenceFileModel*> alls = r->getValidSequenceFiles();
    QSet<QString> tags;
    foreach (SequenceFileModel* s, alls)
    {
        auto ltags = s->getTags();
        tags.unite(QSet<QString>(ltags.begin(), ltags.end())); //::fromList( s->getTags()));
    }
    //  qDebug() << tags;
    QSettings set;
    QDir dir(set.value("databaseDir").toString());

    QFile file(dir.absolutePath() + "/"+ r->hash() + ".tagmap");
    if (file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QTextStream ex(&file);

        ex << tags.values().join(';') << endl;

        foreach (SequenceFileModel* m, alls)
            ex << m->Pos() << ';' << m->getTags().join(";") << endl;
        file.close();
    }
}

void WellRepresentationItem::untagSelection(QString tag)
{
    // Iterate through selection
    // Update  tag file
    ExperimentFileModel* r =  _gsi->currentRepresentation();
    // Iterate trough selection
    QList<SequenceFileModel*> sel = r->getSelection();
    foreach (SequenceFileModel* f, sel)
        f->unsetTag(tag);

    // Update  tag file
    QList<SequenceFileModel*> alls = r->getValidSequenceFiles();
    QSet<QString> tags;
    foreach (SequenceFileModel* s, alls)
    {
        auto ltags = s->getTags();
        tags.unite(QSet<QString>(ltags.begin(), ltags.end())); // ::fromList( s->getTags()));
    }

    QSettings set;
    QDir dir(set.value("databaseDir").toString());

    QFile file(dir.absolutePath() + "/"+ r->hash() + ".tagmap");
    if (file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QTextStream ex(&file);

        ex << tags.values().join(';') << Qt::endl;

        foreach (SequenceFileModel* m, alls)
            ex << m->Pos() << ';' << m->getTags().join(";") << Qt::endl;

        file.close();
    }
}


void WellRepresentationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{

    if (!(event->buttons() & Qt::LeftButton))  { QGraphicsObject::mouseMoveEvent(event);  return; }
    if (!flags().testFlag(ItemIsSelectable))    { QGraphicsObject::mouseMoveEvent(event); return; }

    if ((event->screenPos() - _dragStartPosition).manhattanLength()
            < QApplication::startDragDistance())
    {
        QGraphicsObject::mouseMoveEvent(event);
        return;
    }

    _drag = true;
    QDrag *drag = new QDrag(this);

    ExperimentFileModel * efm = _gsi->currentRepresentation();
    QList<QGraphicsItem*> items = scene()->selectedItems();
    efm->clearState(ExperimentFileModel::IsSelected);

    if (efm)
        foreach (QGraphicsItem* itms, items)
        {
            WellRepresentationItem* wri = dynamic_cast<WellRepresentationItem*>(itms);
            if (wri)
            {
                efm->select(wri->getWellPosition(), true);
            }
        }

    QMimeData *mimeData = new QMimeData;

    mimeData->setData("checkout/Wells", QByteArray());
    drag->setMimeData(mimeData);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    //    qDebug() << dropAction;
    if (dropAction != Qt::IgnoreAction)
    {
        scene()->clearSelection();
    }


    //  QPixmap pix(300,200);
    //  QPainter pa(&pix);

    //  pix.save("c:/temp/bob.png");

    //  scene()->render(&pa);
    //  drag->setPixmap(pix);
}




void WellRepresentationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
    if (!_drag && flags().testFlag(ItemIsSelectable))
    {
        //      setSelected(!isSelected());
        ExperimentFileModel* r =  _gsi->currentRepresentation();
        if (r)
            r->select(_wellPosition, isSelected());

        _gsi->update();
        event->accept();
        //      qDebug() << "mouse Release" << _wellPosition << isSelected();
        //      emit selectionChanged();
    }
    else
        QGraphicsObject::mouseReleaseEvent(event);
    _drag = false;
}

QVariant WellRepresentationItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged)
    {
        _gsi->currentRepresentation()->select(_wellPosition, value.toBool());
        //      qDebug() << "item change " << _wellPosition << value.toBool();
        emit selectionChanged();
        update();
    }
    return QGraphicsItem::itemChange(change, value);
}


QRectF WellRepresentationItem::boundingRect() const
{
    return _rect;
}
