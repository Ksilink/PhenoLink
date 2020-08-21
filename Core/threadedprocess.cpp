#include "threadedprocess.h"

ThreadedProcess::ThreadedProcess(CheckoutProcessPluginInterface *proc):
    _proc(proc)
{
}

void ThreadedProcess::run()
{
    _proc->exec();
}
