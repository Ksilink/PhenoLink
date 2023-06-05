#include "overlayfilter.h"
#include "ui_overlayfilter.h"
#include <QAction>
#include <QMenu>

OverlayFilter::OverlayFilter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverlayFilter)
{
    ui->setupUi(this);
}

OverlayFilter::~OverlayFilter()
{
    delete ui;
}

void OverlayFilter::setTagList(QStringList l)
{

    ui->overlay_tags->clear();
    ui->overlay_tags->addItems(l);
}

void OverlayFilter::on_pushButton_clicked()
{ // Append the selection to the list & send the filter update message

    QStringList match;

    for (auto items:  ui->overlay_tags->selectedItems())
        match << items->text().split(" (").front();

    match.sort();

    ui->overlay_filter->addItem(match.join(";"));

   match.clear();

   for (int i = 0; i < ui->overlay_filter->count(); ++i)
       match << ui->overlay_filter->item(i)->text();

   emit filterChanged(match);
}


void OverlayFilter::on_overlay_filter_customContextMenuRequested(const QPoint &pos)
{

    QMenu menu(this);

    auto *del = menu.addAction("Remove item");
    auto *clear = menu.addAction("Clear all filter");

    auto res = menu.exec(ui->overlay_filter->mapToGlobal(pos));


    if (res == del)
    {
        QStringList items;
        auto li = ui->overlay_filter->selectedItems();
        for (int i = 0; i < ui->overlay_filter->count(); ++i)
            if (!li.contains(ui->overlay_filter->item(i) ))
                items << ui->overlay_filter->item(i)->text();
        ui->overlay_filter->clear();
        ui->overlay_filter->addItems(items);
        emit filterChanged(items);
        return;
    }

    if (res == clear)
    {
        ui->overlay_filter->clear();
    }


}

