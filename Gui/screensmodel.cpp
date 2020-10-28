#include "screensmodel.h"

#include <QDebug>
#include <QStandardItemModel>
//#include <QFileSystemModel>
#include <QProgressBar>
#include <QCoreApplication>

#include <Core/checkoutdataloaderplugininterface.h>

ScreensModel::ScreensModel(QObject *parent) :
    QStandardItemModel(parent)
{


}

bool ScreensModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QStandardItemModel::setData(index, value, role);
    if (role == Qt::CheckStateRole && flags(index).testFlag(Qt::ItemIsUserCheckable))
    {
        if(value == Qt::Checked)
        {
            checkedIndexes.insert(index);
            if(hasChildren(index) == true)
            {
                recursiveCheck(index, value);
            }
        }
        else
        {
            checkedIndexes.remove(index);
            if(hasChildren(index) == true)
            {
                recursiveCheck(index, value);
            }
        }
        emit dataChanged(index, index);
        return true;
    }
    return QStandardItemModel::setData(index, value, role);
}


QStringList ScreensModel::getCheckedDirectories(bool reset)
{
    QStringList r;
    for (QSet<QModelIndex>::iterator
         it = checkedIndexes.begin(), e = checkedIndexes.end();
         it != e;
         ++it)
    {
        QString d = data(*it, Qt::UserRole + 4).toString();

        //        if(hasChildren(*it) == true)
        //            recursiveCheck(*it, QVariant(Qt::Unchecked));

        if (!d.isEmpty())
        {
            QString parent = data(it->parent(), Qt::UserRole + 4).toString();
            if (!parent.isEmpty())
            {
                _child_toGroup.insert(d, parent);
                _group_toChild.insert(parent, d);
            }
            r << d;
        }
    }

    if (reset)
        clearCheckedDirectories();

    return r;
}


void StandardItemRecursiveCheck(QStandardItem* item)
{
    if (item && item->isCheckable())
        item->setCheckState(Qt::Unchecked);

    for (int i = 0; i < item->rowCount(); ++i)
    {
        StandardItemRecursiveCheck(item->child(i));
    }
}

void ScreensModel::clearCheckedDirectories()
{
    checkedIndexes.clear();

    for (int i = 0; i < rowCount(); ++i)
    {

        QStandardItem* ritem = item(i);
        StandardItemRecursiveCheck(ritem);
    }



    //  QStringList r;
    //  for (QSet<QModelIndex>::iterator
    //       it = checkedIndexes.begin(), e = checkedIndexes.end();
    //       it != e;
    //       ++it)
    //    {
    //      QString d = data(*it, Qt::UserRole + 4).toString();

    ////      QStandardItemModel::index(i, 0, index);

    ////      if ((*it).isValid())
    ////        setData(*it, QVariant(Qt::Unchecked), Qt::CheckStateRole);

    //      recursiveCheck(*it, QVariant(Qt::Unchecked));
    //    }


}

#include <QDir>
#include <QFileInfo>
#include <QStandardItem>


void ScreensModel::recurse(QString dir, QStringList search, QStandardItem* parent, QStandardItem* pparent, QStandardItem *gpparent)
{
    step = 1;

    QStringList nowild, wild;

    for (QStringList::iterator it = search.begin(),e = search.end();
         it != e; ++it)
        if (it->contains("*")) // First process non wild card dataset, to increase speed with large amount of data
            wild << *it;
        else
            nowild << *it;

    bool done = false;

    for (QStringList::iterator it = nowild.begin(),e = nowild.end();
         it != e && !done ; ++it)
    {
        QFileInfo i(dir + "/" + *it);
        if (i.exists())
        {
            parent->setData(i.absoluteFilePath(), Qt::UserRole + 4);
            //          qDebug() << i.absoluteFilePath();
            parent->setFlags(parent->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            parent->setData(Qt::Unchecked, Qt::CheckStateRole);
            if (pparent)
            {
                pparent->setFlags(parent->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                pparent->setData(Qt::Unchecked, Qt::CheckStateRole);
            }
            if (gpparent)
            {
                gpparent->setFlags(parent->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                gpparent->setData(Qt::Unchecked, Qt::CheckStateRole);
            }
            done = true;
        }
    }
    if (!done) // if already process can skip this
    {
        for (QStringList::iterator it = wild.begin(),e = wild.end();
             it != e && ! done ; ++it)
        {
            QDir glob(dir);
            QFileInfoList ff = glob.entryInfoList(QStringList() << *it, QDir::Files);
            foreach (QFileInfo i, ff)
            {
                parent->setData(i.absoluteFilePath(), Qt::UserRole + 4);
                //                qDebug() << i.absoluteFilePath();
                parent->setFlags(parent->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                parent->setData(Qt::Unchecked, Qt::CheckStateRole);
                if (pparent)
                {
                    pparent->setFlags(parent->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                    pparent->setData(Qt::Unchecked, Qt::CheckStateRole);
                }
                if (gpparent)
                {
                    gpparent->setFlags(parent->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                    gpparent->setData(Qt::Unchecked, Qt::CheckStateRole);
                }
                done = true;
                break;
            }
        }
    }

    QDir ddir(dir);
    int child = 0;
    QStringList ents = ddir.entryList(QStringList() << "*", QDir::AllDirs | QDir::NoDotAndDotDot);
    for (QStringList::Iterator it = ents.begin(), e = ents.end(); it != e; ++it)
    {
        QStandardItem* r = new QStandardItem(*it);

        parent->setChild(child++, r);
        recurse(dir + "/" + *it, search, r , parent, pparent);
    }
}

void ScreensModel::addDirectoryTh(QString &dir)
{
    addDirectory(dir);
}


QStandardItem* ScreensModel::addDirectory(QString &dir)
{
    //qDebug() << "Addding directory scan: " << dir;
    QFileInfo f(dir);

    QString dispName = f.completeBaseName();

    QStringList search = CheckoutDataLoader::handler().handledFiles();

    //	qDebug() << "Data loader file patterns: " << search;


    QStandardItem* item = new QStandardItem(dispName);
    item->setData(dir, Qt::ToolTipRole);

    this->appendRow(item);

    step = 1;
    recurse(dir, search, item, 0,0);
    return item;
}

QString ScreensModel::getGroup(QString name)
{
    return _child_toGroup[name];
}


bool ScreensModel::recursiveCheck(const QModelIndex &index, const QVariant &value)
{
    if(hasChildren(index))
    {
        int i;
        int childrenCount = rowCount(index);
        QModelIndex child;
        for(i=0; i<childrenCount; i++)
        {
            child = QStandardItemModel::index(i, 0, index);
            this->setData(child, value, Qt::CheckStateRole);
        }
    }
    return true;
}





