
#include <Core/Dll.h>

#include "checkoutprocess.h"
#include "checkoutprocessplugininterface.h"
#include <QElapsedTimer>
#include <QtSql>
#include "RegistrableTypes.h"
#include <networkprocesshandler.h>


//void RegistrableParent::attachPayload(QByteArray arr) const
//{
//    CheckoutProcess::handler().attachPayload(_hash, arr, _keepInMem);
//}

//void RegistrableParent::attachPayload(QString hash, QByteArray arr) const
//{
//    CheckoutProcess::handler().attachPayload(hash, arr, _keepInMem);
//}


void RegistrableParent::attachPayload(std::vector<unsigned char> arr, size_t pos) const
{
    CheckoutProcess::handler().attachPayload(_hash, arr, _keepInMem, pos);
}

void RegistrableParent::attachPayload(QString hash, std::vector<unsigned char> arr, size_t pos) const
{
    CheckoutProcess::handler().attachPayload(hash, arr, _keepInMem, pos);
}
