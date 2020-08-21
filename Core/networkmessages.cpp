#include "networkmessages.h"



const QLatin1String getNetworkMessageString(NetworkMessagesType type)
{
  static const  QLatin1String array[] =
  {
      QLatin1String("processes"),
      QLatin1String("proc_details"),
      QLatin1String("start_proc"),
      QLatin1String("proc_status"),
      QLatin1String("exit_server"),
      QLatin1String("proc_payload"),
      QLatin1String("delete_payload")
  };


  return array[type];
}
