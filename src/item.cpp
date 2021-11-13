#include "item.h"

#include <vector>
#include <QPointer>
#include "engine.h"

using namespace svgt;

class Connection
{
    std::unique_ptr<QMetaObject::Connection> ptr;
public:
    Connection(QMetaObject::Connection&& conn) 
        : ptr(std::make_unique<QMetaObject::Connection>(std::move(conn))) 
    {}
    Connection(Connection&& conn)
        : ptr(std::move(conn.ptr))
    {}
    ~Connection() 
    {
        if (ptr) {
            QObject::disconnect(*ptr);
        }
    }
};

struct Item::Impl
{
    Item& self;
    QString source;
    QPointer<QObject> object;
    QPointer<Engine> engine;
    QMap<QString, QMetaProperty> metaProps;
    //QVector<QMetaProperty> metaProps;
    QMetaMethod updateSlot;
    std::vector<Connection> connections;
    QString destination;
    std::shared_ptr<Foreman> foreman;
    bool completed;

    Impl(Item&);

    void setupObject(QObject* obj);
    void setupEngine(Engine* engine);
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

void Item::Impl::setupObject(QObject* obj)
{
    Q_ASSERT(!object && obj);
    object = obj;

    if (!object) {
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
        metaProps.insert(prop.name(), std::move(prop));
    }

    setup();
}

void Item::Impl::setupEngine(Engine* eng)
{
    Q_ASSERT(!engine && eng);
    engine = eng;
    setup();
}

void Item::Impl::setup()
{
    qDebug() << Q_FUNC_INFO;

    if (!completed || !object || !engine || !updateSlot.isValid()) {
        return;
    }

    foreman = engine->foreman(source, metaProps);

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
    impl->setupEngine(engine);
    emit engineChanged();
}

QObject* Item::object() const
{
    return impl->object;
}

void Item::setObject(QObject* object)
{
    impl->setupObject(object);  
    emit objectChanged();
}

QString Item::source() const
{
    return impl->source;
}

void Item::setSource(const QString& source)
{
    if (impl->source != source) {
        impl->source = source;
        impl->setup();
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

    if (!impl->foreman || !impl->object) {
        return;
    }

    auto destination = impl->foreman->destination(impl->object);

    if (impl->destination != destination) {
        impl->destination = destination;
        emit destinationChanged();
    }

    qDebug() << impl->destination;
}