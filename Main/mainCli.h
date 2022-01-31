#ifndef MAINCLI_H
#define MAINCLI_H

#include <QObject>
#include <QApplication>
#include <QtCore>
#include <QtConcurrent>

#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"

#include <QJsonObject>
using namespace qhttp::server;

class helper: public QHttpServer
{
    Q_OBJECT

private:
    void process(qhttp::server::QHttpRequest *req, qhttp::server::QHttpResponse *res);
    void setHttpResponse(QJsonObject ob, qhttp::server::QHttpResponse *res, bool binary = true);

public slots:
   void listParams(QJsonObject ob);

   void startProcess(QJsonObject ob, QRegExp siteMatcher);

   void setParams(QString proc, QString commit, QStringList params, QStringList plates);
   void setDump(QString dumpfile);


protected:

   QString dump;

   QString proc, commitName;

   QStringList params;
   QStringList plates;
};



#endif // MAINCLI_H
