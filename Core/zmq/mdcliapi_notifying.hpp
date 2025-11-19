#ifndef MDCLIAPI_NOTIFYING_HPP
#define MDCLIAPI_NOTIFYING_HPP

#include "mdcliapi.hpp"
#include <QMessageBox>
#include <QObject>

//  ---------------------------------------------------------------------
//  Extended client class with GUI popup notifications
//  Inherits from mdcli and overrides error handling to show popups

class NotifyingMDCli : public QObject, public mdcli
{
    Q_OBJECT

public:
    //  ---------------------------------------------------------------------
    //  Constructor

    NotifyingMDCli(QString broker, QString id = QString(), int verbose = 0)
        : mdcli(broker, id, verbose)
    {
    }

    //  ---------------------------------------------------------------------
    //  Destructor

    virtual ~NotifyingMDCli()
    {
    }

signals:
    void workerCrashed(QString service, QString message);
    void noWorkersAvailable(QString service, QString message);

protected:
    //  ---------------------------------------------------------------------
    //  Override error notification handler to show GUI popups

    void handle_error_notification(const QCborMap& error) override
    {
        QString errorType = error[QString("error")].toString();
        QString message = error[QString("message")].toString();
        QString service = error[QString("service")].toString();
        QString timestamp = error[QString("timestamp")].toString();
        
        // Show popup notification based on error type
        if (errorType == "WORKER_CRASHED")
        {
            QString fullMessage = QString(
                "A worker processing job '%1' has crashed.\n\n"
                "%2\n\n"
                "The job will be automatically retried.\n\n"
                "Time: %3"
            ).arg(service).arg(message).arg(timestamp);
            
            QMessageBox::warning(
                nullptr,
                "Worker Crashed",
                fullMessage,
                QMessageBox::Ok
            );
            
            emit workerCrashed(service, message);
        }
        else if (errorType == "NO_WORKERS")
        {
            QString fullMessage = QString(
                "No workers are available to process job '%1'.\n\n"
                "%2\n\n"
                "Please start worker servers and try again.\n\n"
                "Time: %3"
            ).arg(service).arg(message).arg(timestamp);
            
            QMessageBox::critical(
                nullptr,
                "No Workers Available",
                fullMessage,
                QMessageBox::Ok
            );
            
            emit noWorkersAvailable(service, message);
        }
        
        // Call base implementation for logging
        mdcli::handle_error_notification(error);
    }
};

#endif // MDCLIAPI_NOTIFYING_HPP
