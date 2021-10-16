#ifndef __UEHUICEUICNIUCEBCIUEBCIECBIEUCBEIC__
#define __UEHUICEUICNIUCEBCIUEBCIECBIEUCBEIC__

#include <QQmlExtensionPlugin>
#include "item.h"

namespace svgt
{

class Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char* uri) override
    {
        qmlRegisterType<Item>(uri, 1, 0, "SVGTItem");
    }
}; //class plugin

} // namespace svgt

#endif // __UEHUICEUICNIUCEBCIUEBCIECBIEUCBEIC__