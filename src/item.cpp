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
    Engine::FileIdPtr fileId;
    QMetaMethod updateSlot;
    QVector<QMetaProperty> metaProps;
    std::vector<Connection> connections;
    
    Impl(Item&);

    void setup();
}; // struct Item::Impl


Item::Impl::Impl(Item& self)
    : self(self)
{
    int slotIdx = self.metaObject()->indexOfMethod("propertiesUpdated()");
    Q_ASSERT(slotIdx >= 0 && slotIdx < self.metaObject()->methodCount());
    updateSlot = self.metaObject()->method(slotIdx);
    Q_ASSERT(updateSlot.isValid());
}

void Item::Impl::setup()
{
    qDebug() << Q_FUNC_INFO;

    if (!object || !engine) {
        return;
    }

    connections.clear();
    metaProps.clear();

    fileId = engine->getFileId(source);

    if (!fileId) {
        self.propertiesUpdated();
        return;
    }

    auto requiredProps = engine->getRequiredProperties(fileId);

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

    self.propertiesUpdated();
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
    impl->setup();
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
    impl->setup();
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
    impl->setup();
    emit sourceChanged();
}

QString Item::destination() const
{
    return impl->destination;
}

void Item::propertiesUpdated()
{
    qDebug() << Q_FUNC_INFO;

    if (!impl->fileId || impl->source.isEmpty()) {
        if (!impl->destination.isEmpty()) {
            impl->destination.clear();
            emit destinationChanged();
        }
        return;
    }

    Q_ASSERT_X(impl->object, qPrintable(impl->source), "object is null");
    Q_ASSERT_X(impl->engine, qPrintable(impl->source), "engine is null");

    if (!impl->engine || !impl->object) {
        return;
    }

    auto destination = impl->engine->getDestination(impl->fileId, impl->metaProps, impl->object);

    if (impl->destination != destination) {
        impl->destination = destination;
        emit destinationChanged();
    }

    qDebug() << impl->destination;
}