#include "engine.h"
#include <private/qabstractfileengine_p.h>
#include <QMap>
#include <QUrl>
#include <QMetaProperty>
#include <QDebug>

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
    QVector<QByteArray> chunks;
    QVector<QString> properties;

    void parse(const QString&);
    QByteArray construct(const QVector<QMetaProperty>&, const QObject*);
public:
    Blueprint(const QString&, Generator& gen);

    QVector<QMetaProperty> filterProps(const QMap<QString, QMetaProperty>&) const;
    QString getFilename(const QVector<QMetaProperty>&, const QObject*);
}; // class Blueprint

class ForemanImpl : public svgt::Foreman
{
    QVector<QMetaProperty> props;
    std::weak_ptr<Blueprint> blueprint;
public:
    ForemanImpl(const std::shared_ptr<Blueprint>&, const QVector<QMetaProperty>&);
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

QByteArray Blueprint::construct(const QVector<QMetaProperty>& props, const QObject* obj)
{
    QByteArray data;

    int i = 0;
    for (const auto& array : chunks) {
        if (!array.isEmpty()) {
            data.append(array);
            continue;
        }
        Q_ASSERT(props.size() > i);
        if (props.size() <= i) {
            return QByteArray();
        }
        auto prop = props[i++];
        data.append(prop.read(obj).toString());
    }

    return data;
}

void Blueprint::parse(const QString& fileName)
{
    QFile file(fileName);

    file.open(QIODevice::ReadOnly);

    Q_ASSERT_X(file.isOpen(), fileName.toStdString().c_str(), "failed to open file for parsing");

    if (!file.isOpen()) {
        qDebug() << "failed to open file" << fileName;
        return;
    }
    
    QByteArray data = file.readAll();

    chunks.clear();

    if (data.isEmpty()) {
        return;
    }

    if (data.size() <= 4) {
        chunks.append(std::move(data));
        return;
    }

    int beg = 0, openBrace = -1;
    for (int i = 0; i < data.size() - 1; ++i) {
        qDebug() << Q_FUNC_INFO << data.size() << i;
        if (beg >= data.size()) {
            break;
        }
        if (data[i] == '{' && data[i + 1] == '{') {
            openBrace = i;
        }
        if (data[i] == '}' && data[i + 1] == '}' && openBrace >= 0) {
            QString property = data.mid(openBrace + 2, i - openBrace -2);
            auto dataChunk = data.mid(beg, openBrace - beg);
            chunks.append(std::move(dataChunk));
            chunks.append(QByteArray());
            properties.append(property);
            beg = i + 2;
            openBrace = -1;
        }
    }

    if (beg < data.size()) {
        chunks.append(data.mid(beg, -1));
    }

    qDebug() << Q_FUNC_INFO << chunks;
}

Blueprint::Blueprint(const QString& fileName, Generator& gen)
    : gen(gen)
{
    fileId = QString::number(gen.fileNameId(fileName), 16);
    parse(fileName);
}

QVector<QMetaProperty> Blueprint::filterProps(const QMap<QString, QMetaProperty>& props) const
{
    QVector<QMetaProperty> metaProps;

    for(const auto& name : properties) {
        auto it = props.constFind(name);
        if (it == props.constEnd()) {
            return QVector<QMetaProperty>();
        }
        metaProps.append(*it);
    }

    return metaProps;
}

QString Blueprint::getFilename(const QVector<QMetaProperty>& props, const QObject* obj)
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

    const QByteArray& data = gen.getData(filename);
    if (data.isEmpty()) {
        gen.addData(filename, construct(props, obj));
    }

    return filename;
    
}

ForemanImpl::ForemanImpl(const std::shared_ptr<::Blueprint>& blueprint, const QVector<QMetaProperty>& props)
    : blueprint(blueprint)
    , props(props)
{
    Q_ASSERT(blueprint);
}

QString ForemanImpl::destination(const QObject*)
{
    return QString();
}

svgt::Engine::Engine(QObject* parent)
    : QObject(parent)
    , impl(std::make_unique<Impl>())
{
}

svgt::Engine::~Engine() = default;

std::shared_ptr<svgt::Foreman> svgt::Engine::foreman(const QString& url, const QMap<QString, QMetaProperty>& props)
{
    if (url.isEmpty()) {
        return nullptr;
    }

    std::shared_ptr<::Blueprint> bp = impl->gen.getBlueprint(url);
    auto vec = bp->filterProps(props);

    return nullptr;
}