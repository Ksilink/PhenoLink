
#include <QtCore>
#ifdef WIN32
#include <QApplication>
#endif

#include <QtNetwork>
#include <QDebug>

#include <fstream>
#include <iostream>


#include "Core/pluginmanager.h"

#include <Core/checkoutprocessplugininterface.h>

#include <Core/checkoutprocess.h>

#include "Core/config.h"

#ifdef WIN32
#include <windows.h>
#include <wincon.h>
#endif

#include <QtConcurrent>

#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"

#include <Core/networkprocesshandler.h>

// addData/<project>/<CommitName>/<Plate> <= POST data as CBOR
// CBOR data should look alike:
// [ { "hash": "process hash value", "Well": "","timepoint":"","fieldId":"","sliceId":"","channel":"","tags":"", "res names": "", ... },
// all rows that are ready for this push
//   {} ]
// When all "hash" is finished
// ListProcesses
// Process/<ProcPath>
//

#include <QtConcurrent>

void doconnect()
{
    // Need to be done in main Thread ???
    auto c = new CheckoutHttpClient("192.168.1.104", 8020);
    c->send("/addData/Default");
}

int main(int ac, char** av)
{
#ifdef WIN32
    QApplication app(ac, av);
#else
    QCoreApplication app(ac, av);
#endif

    app.setApplicationName("PhenoLink");
    app.setApplicationVersion(CHECKOUT_VERSION);
    app.setOrganizationDomain("WD");
    app.setOrganizationName("WD");

    QtConcurrent::run(doconnect);

    return app.exec();
}
