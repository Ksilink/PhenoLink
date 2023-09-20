#ifndef MAINCLI_H
#define MAINCLI_H

#include <QObject>
#include <QApplication>
#include <QtCore>
#include <QtConcurrent>

//#include "qhttp/qhttpserver.hpp"
//#include "qhttp/qhttpserverconnection.hpp"
//#include "qhttp/qhttpserverrequest.hpp"
//#include "qhttp/qhttpserverresponse.hpp"

#include <QJsonObject>
//using namespace qhttp::server;

struct helper : public QObject //: public QHttpServer
{
    Q_OBJECT

public:
   QJsonArray setupProcess(QJsonObject ob, QRegularExpression siteMatcher);

   void setParams(QString proc, QString commit, QStringList params, QStringList plates);
   void setDump(QString dumpfile);


protected:

   QString dump;

   QString proc, commitName;

   QStringList params;
   QStringList plates;
};



#endif // MAINCLI_H
