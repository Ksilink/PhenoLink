#ifndef NETWORKMESSAGES_H
#define NETWORKMESSAGES_H

#include "Dll.h"
#include <QString>


enum NetworkMessagesType
{
  Processes,
  Process_Details,
  Process_Start,
  Process_Status,
  Exit_Server,
  Process_Payload,
  Delete_Payload
};


DllCoreExport const QLatin1String  getNetworkMessageString(NetworkMessagesType type);




#endif // NETWORKMESSAGES_H
