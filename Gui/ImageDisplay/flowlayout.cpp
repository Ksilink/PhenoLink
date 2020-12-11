#include <QSettings>
#include <QLayout>
#include <QWidget>

#include "flowlayout.h"

#include <cmath>
#include <QDebug>

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), _ncols(3), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
    : QLayout(), _ncols(3), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

void FlowLayout::removeItem(QLayoutItem *item)
{
    QLayout::removeItem(item);
    itemList.removeAll(item);
    item->widget()->deleteLater();
}

int FlowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) {
        return m_hSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
}

int FlowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) {
        return m_vSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
}

int FlowLayout::count() const
{
    return itemList.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    else
        return 0;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return Qt::Orientations();
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    QLayoutItem *item;
    foreach (item, itemList)
        size = size.expandedTo(item->minimumSize());

    QSettings q;
    size += QSize(2*margin(), 2*margin());
    //size.setHeight(size.height() * (int)ceil(itemList.size() / (float)q.value("Image Flow/Count", _ncols).toInt()));
    //qDebug() << "Minimum size reported:" << size;
    return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    if (itemList.isEmpty()) return 0;

    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;


    QSettings q;
    int ncols =  q.value("Image Flow/Count", _ncols).toInt();

    QLayoutItem *item;
    foreach (item, itemList) {
        QWidget *wid = item->widget();
        if (wid->isHidden()) continue;

        int spaceX = horizontalSpacing();
        if (spaceX == -1)
            spaceX = wid->style()->layoutSpacing(
                        QSizePolicy::Frame, QSizePolicy::Frame, Qt::Horizontal);
        int spaceY = verticalSpacing();
        if (spaceY == -1)
            spaceY = wid->style()->layoutSpacing(
                        QSizePolicy::Frame, QSizePolicy::Frame, Qt::Vertical);
        int nextX = x + (effectiveRect.width() ) / ncols;
        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + (effectiveRect.width() )/ ncols;
            lineHeight = 0;
        }

   /*     float ratio = std::min(std::ceil(effectiveRect.width() / (float)ncols) / item->sizeHint().width(),
            ((float)effectiveRect.height()) / item->sizeHint().height());*/

        float ratio = (effectiveRect.width() / (float)ncols) / item->sizeHint().width();

        if (!testOnly)
        {
          /*  float lratio = std::min(std::ceil(effectiveRect.width() / (float)ncols) / item->sizeHint().width(),
                ((float)effectiveRect.height()) / item->sizeHint().height());*/
            item->setGeometry(QRect(x, y, item->sizeHint().width() * ratio, item->sizeHint().height() * ratio));
        }

        x = nextX;
        lineHeight = qMax(lineHeight, (int)(item->sizeHint().height() * ratio));
    }

    return y + lineHeight - rect.y() + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *parent = this->parent();
    if (!parent) {
        return -1;
    } else if (parent->isWidgetType()) {
        QWidget *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, 0, pw);
    } else {
        return static_cast<QLayout *>(parent)->spacing();
    }
}
