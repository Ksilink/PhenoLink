        #include <QGraphicsScene>
    #include <QGraphicsView>
    #include <QDebug>

    #include <QToolButton>
    #include <QIcon>
    #include <QGraphicsProxyWidget>

    #include "graphicsscreensitem.h"

    #include "wellrepresentationitem.h"
    #include "screensgraphicsview.h"



    GraphicsScreensItem::GraphicsScreensItem(QGraphicsItem *parent):
        QGraphicsRectItem(parent), _coloriser(0)
    {
        //  setCursor(Qt::OpenHandCursor);
        // setFlags(QGraphicsItem::ItemIsSelectable);
        //  setFlags(QGraphicsRectItem::ItemSendsScenePositionChanges);

    }

    void GraphicsScreensItem::setColumnItem(QGraphicsItemGroup *col)
    {
        _col = col;
    }

    void GraphicsScreensItem::setRowItem(QGraphicsItemGroup *row)
    {
        _row = row;
    }

    QVariant GraphicsScreensItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
    {

        if (change == ItemPositionChange && scene())
        {
            QPointF newPos = value.toPointF();
            //      qDebug() << "Item pos:" << pos() << "new pos"<< newPos;
            //      QRectF rect = scene()->sceneRect();
            QGraphicsView* v = scene()->views().first();

            QRectF rect = v->mapToScene(v->viewport()->rect()).boundingRect(),
                    tr = rect;

            QRectF br = scene()->sceneRect();

            rect.setX(br.width()  > rect.width()  ? rect.right()-br.width() : 0);

            rect.setY(br.height() > rect.height() ? rect.bottom()-br.height() : 0);

            rect.setBottom(0);
            rect.setRight(0);

            if (!rect.contains(newPos))
            {
                newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
                newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
            }


            _col->setX(newPos.x());
            _col->setY(tr.top());

            _row->setX(tr.left());
            _row->setY(newPos.y());
            return newPos;
        }

        return QGraphicsRectItem::itemChange(change, value);

    }

    void GraphicsScreensItem::Handmode(bool checked)
    {
        QGraphicsItem::GraphicsItemFlags f = flags();

        //  setFlag(QGraphicsRectItem::ItemIsMovable, checked);
        //  setCursor(QCursor(checked ? Qt::OpenHandCursor : Qt::ArrowCursor));

        //  QList<WellRepresentationItem*> items = findChildren<WellRepresentationItem*>();

        //  foreach(WellRepresentationItem* it, items)
        //    {
        //      it->setFlag(ItemIsMovable, checked);
        //      it->setCursor(QCursor(checked ? Qt::OpenHandCursor : Qt::ArrowCursor));
        //    }
    }

    void GraphicsScreensItem::innerSelectionChanged()
    {
    //  qDebug() << "GSI :: innerSelectionChanged();";


      emit selectionChanged();
    }


    QRectF GraphicsScreensItem::constructWellRepresentation(ExperimentFileModel* mdl,  ScreensGraphicsView* view)
    {
        QGraphicsScene *scene = view->scene();
        view->setExperimentModel(mdl);
        _models = mdl;
        //  ExperimentFileModel*  mdl = smdl.first();

        // Display the letters in the
        const char* letter = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        qreal w = 28, h = 14;
        qreal pos = w;
        QFont f; f.setPointSize(6);


        QGraphicsItemGroup* colGrp = new QGraphicsItemGroup();
        QGraphicsItemGroup* rowGrp = new QGraphicsItemGroup();

        QGraphicsRectItem* r = new QGraphicsRectItem(w,0, mdl->getColCount()*w, h);
        r->setBrush(QBrush(Qt::darkGray));  r->setZValue(1);  colGrp->addToGroup(r);

        r = new QGraphicsRectItem(0,h, w, mdl->getRowCount()*h);

        QRectF result = QRectF(w,0, mdl->getColCount()*w, h).united(QRectF(0,h, w, mdl->getRowCount()*h));

        r->setBrush(QBrush(Qt::darkGray));  r->setZValue(1);  rowGrp->addToGroup(r);

        for (unsigned col = 0; col < mdl->getColCount(); ++col, pos += w)
        {
            //      QGraphicsTextItem* txt = new QGraphicsTextItem(QString("%1").arg(letter[col]));
            QGraphicsTextItem* txt = new QGraphicsTextItem(QString("%1").arg(col+1, 2, 10, QLatin1Char('0')));


            txt->setFont(f);

            qreal cx = (w - txt->boundingRect().width()  ) / 2.;
            qreal cy = (h - txt->boundingRect().height() ) / 2.;

            txt->setX(pos + cx);
            txt->setY(cy);

            txt->setZValue(2);

            colGrp->addToGroup(txt);
        }


        int posy = 0;
        //  GraphicsScreensItem* sci = new GraphicsScreensItem();


        setRect(QRectF(w, h, mdl->getColCount()*w, mdl->getRowCount()*h));

    //    for (unsigned r = 0; r < mdl->getColCount(); ++r)
        for (unsigned r = 0; r < mdl->getRowCount(); ++r) /// FIXED: Error in indexing here, should have been row and not col
        {
            //      QGraphicsTextItem* txt = new QGraphicsTextItem(QString("%1").arg(r));
            QGraphicsTextItem* txt = r < 27 ? new QGraphicsTextItem(QString("%1").arg(letter[r])) :
                                              new QGraphicsTextItem(QString("%1%2").arg(letter[r/27]).arg(letter[r%27]));

            txt->setFont(f);

            posy += h;
            qreal cx = (w - txt->boundingRect().width()  ) / 2.;
            qreal cy = (h - txt->boundingRect().height() ) / 2.;

            txt->setX(cx);
            txt->setY(posy + cy);
            rowGrp->addToGroup(txt);
            txt->setZValue(2);
            pos = w;
            for (unsigned col = 0; col < mdl->getColCount(); ++col, pos += w)
            {

                WellRepresentationItem* rc = new WellRepresentationItem(pos+2, posy+2, w-4, h-4, this);
                connect(rc, SIGNAL(selectionChanged()), this, SLOT(innerSelectionChanged()));

                rc->setWellPosition(QPoint(r, col));
                rc->setGraphicsScreensItem(this);

                if (mdl->hasMeasurements(QPoint(r, col)) && (*mdl)(r,col).isValid())
                {
                    rc->setBrush(QBrush(Qt::green));
                    rc->setFlags(QGraphicsItem::ItemIsSelectable);
                    _items << rc;
                }
                else
                  {

                    QBrush b(Qt::darkRed);
                    if (!(*mdl)(r,col).isValid())
                      b.setStyle(Qt::DiagCrossPattern);
                    rc->setBrush(b);
                  }
            }
        }

        scene->addItem(this);
        scene->addItem(colGrp);
        scene->addItem(rowGrp);

        //  QToolButton* but = new QToolButton();

        //  but->setCheckable(true);
        //  but->setIcon(QIcon("://openhand.png"));
        //  but->setChecked(true);

        //  connect(but, SIGNAL(toggled(bool)), this, SLOT(Handmode(bool)));

        //  QGraphicsProxyWidget* gpw = scene->addWidget(but);
        //  gpw->setScale(1);

        setColumnItem(colGrp);
        setRowItem(rowGrp);

        return result;
    }



    ExperimentFileModel *GraphicsScreensItem::currentRepresentation()
    {
      return _models;
    }

    void GraphicsScreensItem::updateColorisationModel(WellColorisation *cols)
    {
      _coloriser = cols;

      foreach (WellRepresentationItem* wri, _items)
        wri->setColorizer(cols);



    }

    WellColorisation *GraphicsScreensItem::colorisationModel()
    {
      return _coloriser;
    }

