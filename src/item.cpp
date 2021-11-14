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
    QString destination;
    QPointer<Engine> engine;
    QPointer<QObject> object;
    Engine::FileIdWPtr fileId;
    QMetaMethod updateSlot;
    QVector<QMetaProperty> metaProps;
    std::vector<Connection> connections;
    
    Impl(Item&);

    bool setup();
}; // struct Item::Impl


Item::Impl::Impl(Item& self)
    : self(self)
{
    int slotIdx = self.metaObject()->indexOfMethod("propertiesUpdated()");
    Q_ASSERT(slotIdx >= 0 && slotIdx < self.metaObject()->methodCount());
    updateSlot = self.metaObject()->method(slotIdx);
    Q_ASSERT(updateSlot.isValid());
}

bool Item::Impl::setup()
{
    qDebug() << Q_FUNC_INFO;

    if (!object || !engine) {
        return false;
    }

    connections.clear();
    metaProps.clear();

    auto ptr = engine->getFileId(source);

    if (!ptr) {
        return false;
    }

    auto requiredProps = engine->getRequiredProperties(ptr);

    const QMetaObject* mo = object->metaObject();

    QVector<QMetaProperty> props;
    std::vector<Connection> conns;
    for (const auto& name : qAsConst(requiredProps)) {
        int pIdx = mo->indexOfProperty(qPrintable(name));

        if (pIdx <= 0) {
            qCritical() << "No property in object:" << name << "source:" << source;
            continue;
        }
       
        auto metaProperty = mo->property(pIdx);
        auto notifySignal = metaProperty.notifySignal();

        if (notifySignal.isValid()) {
            auto connection = QObject::connect(object, notifySignal, &self, updateSlot, Qt::UniqueConnection);
            if (connection) {
                conns.emplace_back(std::move(connection));
            }
        }

        props.append(metaProperty);
    }

    metaProps = std::move(props);
    connections = std::move(conns);
    fileId = ptr;

    return true;
}

Item::Item(QObject* parent)
    : QObject(parent)
    , impl(std::make_unique<Impl>(*this))
{
}

Item::~Item() {}

Engine* Item::engine() const
{
    return impl->engine;
}

void Item::setEngine(Engine* engine)
{
    Q_ASSERT(!impl->engine && engine);
    if (impl->engine == engine) {
        return;
    }
    impl->engine = engine;
    impl->fileId.reset();
    propertiesUpdated();
    emit engineChanged();
}

QObject* Item::object() const
{
    return impl->object;
}

void Item::setObject(QObject* object)
{
    if (impl->object == object) {
        return;
    } 
    impl->object = object;
    impl->fileId.reset();
    propertiesUpdated();
    emit objectChanged();
}

QString Item::source() const
{
    return impl->source;
}

void Item::setSource(const QString& source)
{
    if (impl->source == source) {
        return;
    }
    impl->source = source;
    impl->fileId.reset();
    propertiesUpdated();
    emit sourceChanged();
}

QString Item::destination() const
{
    return impl->destination;
}

void Item::propertiesUpdated()
{
    if (impl->source.isEmpty()) {
        if (!impl->destination.isEmpty()) {
            impl->destination.clear();
            emit destinationChanged();
        }
        return;
    }

    if (impl->fileId.expired() && !impl->setup()) {
        impl->destination.clear();
        emit destinationChanged();
        return;
    }

    auto ptr = impl->fileId.lock();

    Q_ASSERT_X(ptr, qPrintable(impl->source), "failed to create ptr");

    auto destination = impl->engine->getDestination(ptr, impl->metaProps, impl->object);

    if (impl->destination != destination) {
        impl->destination = destination;
        emit destinationChanged();
    }
}