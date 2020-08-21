#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QRect>
#include <QStyle>

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    explicit FlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~FlowLayout();

    virtual void addItem(QLayoutItem *item);
    virtual void removeItem(QLayoutItem *item);

    virtual int horizontalSpacing() const;
    virtual int verticalSpacing() const;
    virtual Qt::Orientations expandingDirections() const;
    virtual bool hasHeightForWidth() const;
    virtual int heightForWidth(int) const;
    virtual int count() const;
    virtual QLayoutItem *itemAt(int index) const;
    virtual QSize minimumSize() const;
    virtual void setGeometry(const QRect &rect);
    virtual QSize sizeHint() const;
    virtual QLayoutItem *takeAt(int index);

  template <class T>
    QList<T> childs()
    {
      QList<T> l;

      foreach (QLayoutItem* it, itemList)
        {
          T o = qobject_cast<T>(it->widget());
          if (o)
            l << o;
        }

      return l;
    }

private:
    int doLayout(const QRect &rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> itemList;

    int _ncols;

    int m_hSpace;
    int m_vSpace;
};


#endif // FLOWLAYOUT_H
