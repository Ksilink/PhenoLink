#ifndef SCROLLZONE_H
#define SCROLLZONE_H

#include <QScrollArea>
#include "mainwindow.h"

class QDragEnterEvent;
class QDropEvent;
class SequenceFileModel;
class ImageForm;
class QProgressDialog;

class ScrollZone : public QScrollArea
{
    Q_OBJECT
public:
    explicit ScrollZone(QWidget *parent = 0);


    void setMainWindow(MainWindow* wi);

    void removeSequences(QList<SequenceFileModel*>& lsfm);

    void insertImage(SequenceFileModel *sfm, SequenceInteractor* iactor = nullptr);
    void refresh(SequenceFileModel *sfm);

    void select(ImageForm* f);
    void toggle(ImageForm* f);
    void unselect(ImageForm* f);
    bool isSelected(ImageForm* f);
    void clearSelection();
    void updateSelection();
    QList<ImageForm*> currentSelection();
    void toggleRange(ImageForm* f);
    void selectRange(ImageForm* f);
    void addSelectedWells();

    int items();

    void removeImageForm(ImageForm *im);
    SequenceInteractor *getInteractor(SequenceFileModel *mdl);
protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);


protected:
    QMap<SequenceFileModel*, ImageForm*> _seq_toImg;
    QList<ImageForm*> _selection;
    MainWindow* _mainwin;
//    QProgressDialog* _progDiag;

    QWidget* _wid;


    void setupImageFormInteractor(ImageForm *f);
};

#endif // SCROLLZONE_H
