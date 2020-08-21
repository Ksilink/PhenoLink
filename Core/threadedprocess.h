#ifndef THREADEDPROCESS_H
#define THREADEDPROCESS_H

#include <QRunnable>
#include <QElapsedTimer>

#include <PluginManager/checkoutprocessplugininterface.h>

class ThreadedProcess : public QRunnable
{
public:
    ThreadedProcess( CheckoutProcessPluginInterface* proc);


    void run();

protected:
    CheckoutProcessPluginInterface* _proc;

};

#endif // THREADEDPROCESS_H
