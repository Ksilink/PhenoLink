#ifndef SCREENSMODEL_H
#define SCREENSMODEL_H

//#include <QFileSystemModel>
//#include <QSortFilterProxyModel>

#include <QStandardItemModel>
#include <QSet>

#include <Dll.h>


class QProgressBar;

class  ScreensModel : public QStandardItemModel
{
    //    Q_OBJECT
public:
    explicit ScreensModel(QObject *parent = 0);


    //    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    //    void setSourceModel(QAbstractItemModel *sourceModel);
    //    QVariant data(const QModelIndex &index, int role) const;
    //    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role)  ;

    QStringList getCheckedDirectories(bool reset = true);
    void clearCheckedDirectories();

    QStandardItem* addDirectory(QString& dir);
    void addDirectoryTh(QString& dir);
    QString getGroup(QString name);
protected:
    //    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool recursiveCheck(const QModelIndex &index, const QVariant &value);

    QSet<QModelIndex> checkedIndexes;
    void recurse(QString dir, QStringList search, QStandardItem *parent, QStandardItem *pparent, QStandardItem *gpparent);

    //signals:

    //public slots:
protected:
    int step;

    QMultiMap<QString, QString> _group_toChild; // Associated directory with child data & parent data
    QMap<QString, QString> _child_toGroup;
};

#endif // SCREENSMODEL_H
