#ifndef CHECKOUTERRORHANDLER_H
#define CHECKOUTERRORHANDLER_H

#include <QString>
#include <Dll.h>

class DllCoreExport CheckoutErrorHandler
{
public:
    CheckoutErrorHandler();


    static CheckoutErrorHandler& getInstance();

    void addError(QString details);
    bool hasErrors();
    QString getErrors();
    void resetErrors();

protected:

    QString errors;

};

#endif // CHECKOUTERRORHANDLER_H
