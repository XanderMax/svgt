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
        qmlRegisterSingletonType<Engine>(uri, 1, 0, "SvgtEngineInstance", [](auto*, auto*) {
            QObject* engine = new Engine();
            return engine;
        });
        qmlRegisterType<Item>(uri, 1, 0, "SvgtItemImpl");
    }
}; //class plugin

} // namespace svgt

#endif // __UEHUICEUICNIUCEBCIUEBCIECBIEUCBEIC__