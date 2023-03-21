#ifndef ${UpperPlugin}_H
#define ${UpperPlugin}_H

#include <QtPlugin>
#include <QString>

#include "Core/Dll.h"


#include "Core/checkoutprocessplugininterface.h"

class DllPluginExport ${plugin_name}: public QObject, public CheckoutProcessPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CheckoutProcessPluginInterface_iid)
    Q_INTERFACES(CheckoutProcessPluginInterface)

public:
    ${plugin_name}();

    virtual CheckoutProcessPluginInterface* clone();
    virtual void exec();

    virtual QString plugin_version() const { return  GitHash ; }

protected:
    ImageXP images; // Just a default image
};




#endif // ${UpperPlugin}_H
