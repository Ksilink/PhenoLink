#ifndef MAINCLI_H
#define MAINCLI_H

#include <QObject>
#include <QApplication>
#include <QJsonObject>

class helper: public QObject
{
    Q_OBJECT
public slots:
   void listParams(QJsonObject ob);

   void startProcess(QJsonObject ob);

   void setParams(QString proc, QStringList params, QStringList plates);



protected:
   QString process;
   QStringList params;
   QStringList plates;
};



#endif // MAINCLI_H
