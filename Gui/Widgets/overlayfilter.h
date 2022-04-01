#ifndef OVERLAYFILTER_H
#define OVERLAYFILTER_H

#include <QWidget>

namespace Ui {
class OverlayFilter;
}

class OverlayFilter : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayFilter(QWidget *parent = nullptr);
    ~OverlayFilter();

    void setTagList(QStringList);

private slots:
    void on_pushButton_clicked();

    void on_overlay_filter_customContextMenuRequested(const QPoint &pos);

signals:
    void filterChanged(QStringList);

private:
    Ui::OverlayFilter *ui;
};

#endif // OVERLAYFILTER_H
