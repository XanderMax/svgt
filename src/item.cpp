#include "item.h"

#include <vector>
#include <QPointer>
#include "engine.h"

using namespace svgt;

class Connection
{
    QMetaObject::Connection c;
public:
    Connection(QMetaObject::Connection&& c) : c(std::move(c)) {}
    ~Connection() {QObject::disconnect(c);}
};

struct Item::Impl
{
    Item& self;
    QUrl source;
    QPointer<QObject> object;
    QPointer<Engine> engine;
    QVector<QMetaProperty> metaProps;
    QMetaMethod updateSlot;
    std::vector<Connection> connections;
    QString destination;
    bool completed;

    Impl(Item&);

    void setup();
}; // struct Item::Impl


Item::Impl::Impl(Item& self)
    : self(self)
    , completed(false)
{
    int slotIdx = self.metaObject()->indexOfMethod("propertiesUpdated()");
    Q_ASSERT(slotIdx >= 0 && slotIdx < self.metaObject()->methodCount());
    updateSlot = self.metaObject()->method(slotIdx);
}

void Item::Impl::setup()
{
    qDebug() << Q_FUNC_INFO;
    if (!completed || !object || !engine || !updateSlot.isValid()) {
        return;
    }

    const QMetaObject* mo = object->metaObject();

    metaProps.clear();
    connections.clear();

    for(int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        auto prop = mo->property(i);
        auto notifySignal = prop.notifySignal();
        auto connection = QObject::connect(object, notifySignal, &self, updateSlot);
        connections.emplace_back(std::move(connection));
        metaProps.push_back(std::move(prop));
    }

    self.propertiesUpdated();
}

Item::Item(QObject* parent)
    : QObject(parent)
    , impl(std::make_unique<Impl>(*this))
{

}

Item::~Item() {}

void Item::classBegin()
{
    qDebug() << Q_FUNC_INFO;
}

void Item::componentComplete()
{
    qDebug() << Q_FUNC_INFO;
    impl->completed = true;
    impl->setup();
}

Engine* Item::engine() const
{
    return impl->engine;
}

void Item::setEngine(Engine* engine)
{
    if (impl->engine != engine) {
        impl->engine = engine;
        impl->setup();
        emit engineChanged();
    }
}

QObject* Item::object() const
{
    return impl->object;
}

void Item::setObject(QObject* object)
{
    if (impl->object != object) {
        impl->object = object;
        impl->setup();
        emit objectChanged();
    }
}

QUrl Item::source() const
{
    return impl->source;
}

void Item::setSource(const QUrl& source)
{
    if (impl->source != source) {
        impl->source = source;
        propertiesUpdated();
        emit sourceChanged();
    }
}

QString Item::destination() const
{
    return impl->destination;
}

void Item::propertiesUpdated()
{
    qDebug() << Q_FUNC_INFO;

    if (impl->source.isEmpty()) {
        impl->destination.clear();
        emit destinationChanged();
        return;
    }

    if (!impl->object || !impl->engine) {
        return;
    }

    auto dest = impl->engine->getDestination(impl->source, impl->metaProps, impl->object);
    
    if (impl->destination != dest) {
        impl->destination = dest;
        emit destinationChanged();
    }

    qDebug() << impl->destination;
}