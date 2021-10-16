#include "item.h"
#include <QPointer>
#include "engine.h"

using namespace svgt;

struct Item::Impl
{
    Item& self;
    QUrl source;
    QPointer<QQuickItem> item;
    Engine& engine;
    QMap<QString, QString> props;
    QVector<QMetaProperty> metaProps;
    QMetaMethod updateSlot;
    std::weak_ptr<Blueprint> bp;
    QString destination;

    Impl(Item&);

    bool setup(QQuickItem*);
}; // struct Item::Impl


Item::Impl::Impl(Item& self)
    : self(self)
    , engine(Engine::instance())
{
    int slotIdx = self.metaObject()->indexOfMethod("propertiesUpdated()");
    if (slotIdx < 0 || slotIdx >= self.metaObject()->methodCount()) {
        qDebug() << "Couldn't find 'propertiesUpdated' slot" << slotIdx;
        std::terminate();
    }
    updateSlot = self.metaObject()->method(slotIdx);
}

bool Item::Impl::setup(QQuickItem* item)
{
    qDebug() << Q_FUNC_INFO;
    if (!item || item == this->item) return false;

    this->item = item;

    const QMetaObject* mo = item->metaObject();
    metaProps.clear();
    for(int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        QMetaMethod notifySignal = prop.notifySignal();
        notifySignal.methodIndex();
        QObject::connect(item, notifySignal, &self, updateSlot);
        metaProps.push_back(std::move(prop));
    }

    self.propertiesUpdated();

    return true;
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
}

QQuickItem* Item::item() const
{
    return impl->item;
}

void Item::setItem(QQuickItem* item)
{
    if (impl->setup(item)) {
        emit itemChanged();
    }
}

QUrl Item::source() const
{
    return impl->source;
}

void Item::setSource(const QUrl& str)
{
    if (impl->source == str) {
        return;
    }
    impl->source = str;
    impl->bp = impl->engine.blueprint(str);
    emit sourceChanged();
}

QString Item::destination() const
{
    return impl->destination;
}

void Item::propertiesUpdated()
{
    qDebug() << Q_FUNC_INFO;
    if (!impl->item) {
        return;
    }
    for (const auto& prop : impl->metaProps) {
        auto str = prop.read(impl->item).toString();
        str.replace('#', "-");
        impl->props[prop.name()] = std::move(str);
    }
    
    if (auto sp = impl->bp.lock()) {
        impl->destination = sp->construct(impl->props);
    }
    qDebug() << impl->destination;
}