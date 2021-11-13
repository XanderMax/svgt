#include "engine.h"
#include <private/qabstractfileengine_p.h>
#include <QMap>
#include <QUrl>
#include <QMetaProperty>

namespace
{

class Blueprint;
class Generator
{
    QMap<QString, std::shared_ptr<Blueprint>> blueprints;
    QMap<QString, QByteArray> cache;
    QMap<QString, int> fileNames2Id;
public:
    Generator() = default;

    std::shared_ptr<Blueprint> getBlueprint(const QString&);
    const QByteArray& getData(const QString&) const;
    bool contains(const QString&) const;
    void addData(const QString&, QByteArray&&);
    int fileNameId(const QString&);
}; // class Generator

class Blueprint
{
    Generator& gen;
    QString fileId;

    void parse(const QString&);
public:
    Blueprint(const QString&, Generator& gen);

    QVector<QMetaProperty> filterProps(const QVector<QMetaProperty>&) const;
    QString getFilename(const QVector<QMetaProperty>&, const QObject*) const;
}; // class Blueprint

class ForemanImpl : public svgt::Foreman
{
    QVector<QMetaProperty> props;
    std::weak_ptr<Blueprint> blueprint;
public:
    ForemanImpl(const std::shared_ptr<Blueprint>&);
    QString destination(const QObject*) override;
}; // class ForemanImpl

} // namespace

struct svgt::Engine::Impl
{
    Generator gen;
};

std::shared_ptr<Blueprint> Generator::getBlueprint(const QString& fileName)
{
    auto it = blueprints.constFind(fileName);
    if (it != blueprints.constEnd()) {
        return *it;
    }
    auto ptr = std::make_shared<Blueprint>(fileName, *this);
    blueprints.insert(fileName, ptr);
    return ptr;
}

const QByteArray& Generator::getData(const QString& fileName) const
{
    auto it = cache.constFind(fileName);
    if (it != cache.constEnd()) {
        return *it;
    }
    return QByteArray();
}

bool Generator::contains(const QString& fileName) const
{
    return cache.contains(fileName);
}

void Generator::addData(const QString& fileName, QByteArray&& data)
{
    cache[fileName] = std::move(data);
}

int Generator::fileNameId(const QString& fileName)
{
    auto it = fileNames2Id.constFind(fileName);
    if (it != fileNames2Id.constEnd()) {
        return *it;
    }
    auto id = fileNames2Id.size();
    fileNames2Id.insert(fileName, id);
    return id;
}

Blueprint::Blueprint(const QString& fileName, Generator& gen)
    : gen(gen)
{
    fileId = QString::number(gen.fileNameId(fileName), 16);
    parse(fileName);
}

QString Blueprint::getFilename(const QVector<QMetaProperty>& props, const QObject* obj) const
{
    Q_ASSERT(!props.isEmpty() && obj);

    if (props.isEmpty() || obj) {
        return QString();
    }

    QString filename;

    filename.append('/').append(fileId);
    
    for (auto it = props.constBegin(), end = props.constEnd(); it != end; ++it) {
        QString value = it->read(obj).toString();
        value.replace('#', '-');
        filename.append('-');
        filename.append(value);
    }

    filename.append(".svgt");

    return filename;
    
}

ForemanImpl::ForemanImpl(const std::shared_ptr<::Blueprint>& blueprint)
    : blueprint(blueprint)
{
    Q_ASSERT(blueprint);
}

svgt::Engine::Engine(QObject* parent)
    : QObject(parent)
    , impl(std::make_unique<Impl>())
{
}

svgt::Engine::~Engine() = default;

std::shared_ptr<svgt::Foreman> svgt::Engine::foreman(const QUrl& url, const QVector<QMetaProperty>&)
{
    if (url.isEmpty()) {
        return nullptr;
    }

    std::shared_ptr<::Blueprint> bp = impl->gen.getBlueprint(url.toString());

    return nullptr;
}