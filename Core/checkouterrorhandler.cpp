#include "checkouterrorhandler.h"

CheckoutErrorHandler::CheckoutErrorHandler()
{

}

CheckoutErrorHandler &CheckoutErrorHandler::getInstance()
{
    static CheckoutErrorHandler handler;

    return handler;
}

void CheckoutErrorHandler::addError(QString details)
{
    this->errors += details + "\r\n";
}

bool CheckoutErrorHandler::hasErrors()
{
     return ! this->errors.isEmpty();
}

QString CheckoutErrorHandler::getErrors()
{
    return this->errors;
}

void CheckoutErrorHandler::resetErrors()
{
    this->errors.clear();
}



