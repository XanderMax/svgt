#include "engine.h"
#include <private/qabstractfileengine_p.h>
#include <QMap>
#include <QUrl>
#include <QMetaProperty>
#include <QDebug>
#include <memory>

namespace
{
using Id = QString;
class Blueprint
{
    QVector<QByteArray> chunks;
    QVector<QString> properties;
    void parse(const QString&);
public:
    Blueprint(const QString&);

    const QVector<QString>& requiredProps() const;
    QByteArray construct(const QVector<QMetaProperty>&, const QObject*) const;
}; // class Blueprint

class Generator
{
    QMap<Id, Blueprint> blueprints;
    QMap<QString, QByteArray> cache;
    QMap<QString, Id> fileNames2Id;
public:
    Generator() = default;

    const QByteArray& getData(const QString&);
    Id fileNameId(const QString&);
    QVector<QString> requiredProps(const Id&);
    QString getFilename(const Id&, const QVector<QMetaProperty>&, const QObject*);
}; // class Generator

class FileEngine : public QAbstractFileEngine
{
    const QByteArray& data;
    qint64 pos = 0;
public:
    FileEngine(const QByteArray&);
    bool open(QIODevice::OpenMode) override;
    qint64 read(char*, qint64) override;
    bool seek(qint64) override;
}; // class FileEngine

class FileHandler : public QAbstractFileEngineHandler
{
    Generator& gen;
public:
    FileHandler(Generator& gen);
    QAbstractFileEngine* create(const QString& name) const;
}; // class FileHandler

class FileIdImpl : public svgt::Engine::FileId
{
    QString idStr;
public:
    FileIdImpl(const QString&);
    const QString& id() const;
};

} // namespace

FileEngine::FileEngine(const QByteArray& data)
    : QAbstractFileEngine()
    , data(data)
{
    Q_ASSERT(!data.isEmpty());
    if (data.isEmpty()) {
        qCritical() << "something went wrong";
        std::abort();
    }
}

struct svgt::Engine::Impl
{
    Generator generator;
    FileHandler handler;

    Impl();
};

bool FileEngine::open(QIODevice::OpenMode)
{
    return true;
}

qint64 FileEngine::read(char* storage, qint64 maxlen)
{
    if (data.isEmpty()) return 0;
    qint64 r = qMin(maxlen, data.size() - pos);
    memcpy(storage, data.data() + pos, r);
    return r;
}

bool FileEngine::seek(qint64 i)
{
    if (i >= 0 && i < data.size()) {
        pos = i;
        return true;
    }
    return false;
}

FileHandler::FileHandler(Generator& gen)
    : QAbstractFileEngineHandler()
    , gen(gen)
{
    qDebug() << Q_FUNC_INFO;
}

QAbstractFileEngine* FileHandler::create(const QString& name) const
{
    if (!name.endsWith(".svgt")) {
        return nullptr;
    }

    const auto& data = gen.getData(name);

    Q_ASSERT_X(!data.isEmpty(), qPrintable(name), "no data for file");
        
    if (data.isEmpty()) {
        return nullptr;
    }

    return new FileEngine(data);
}

FileIdImpl::FileIdImpl(const QString& idStr)
    : idStr(idStr)
{
    Q_ASSERT(!idStr.isEmpty());
}

const QString& FileIdImpl::id() const
{
    return idStr;
}

svgt::Engine::Impl::Impl()
    : generator()
    , handler(generator)
{
}

const QByteArray& Generator::getData(const QString& fileName)
{
    auto it = cache.find(fileName);
    if (it != cache.end()) {
        return *it;
    }
    return cache[fileName];
}
QVector<QString> Generator::requiredProps(const Id& id)
{
    auto it = blueprints.constFind(id);

    Q_ASSERT_X(it != blueprints.constEnd(), qPrintable(id), "no such id");

    if (it == blueprints.constEnd()) {
        return QVector<QString>();
    }

    return it->requiredProps();
}

QString Generator::fileNameId(const QString& fileName)
{
    auto it = fileNames2Id.constFind(fileName);
    if (it != fileNames2Id.constEnd()) {
        return *it;
    }
    auto id = QString::number(fileNames2Id.size(), 16);
    fileNames2Id.insert(fileName, id);

    blueprints.insert(id, Blueprint(fileName));

    return id;
}

QByteArray Blueprint::construct(const QVector<QMetaProperty>& props, const QObject* obj) const
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

    Q_ASSERT_X(file.isOpen(), qPrintable(fileName), "failed to open file for parsing");

    if (!file.isOpen()) {
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
}

Blueprint::Blueprint(const QString& fileName)
{
    Q_ASSERT(!fileName.isEmpty());
    if (!fileName.isEmpty()) {
        parse(fileName);
    }
}

const QVector<QString>& Blueprint::requiredProps() const
{
    return properties;
}

QString Generator::getFilename(const Id& id, const QVector<QMetaProperty>& props, const QObject* obj)
{
    Q_ASSERT(obj);

    if (!obj || id.isEmpty()) {
        return QString();
    }

    auto blueprintIt = blueprints.constFind(id);

    Q_ASSERT_X(blueprintIt != blueprints.constEnd(), qPrintable(id), "no such id");

    if (blueprintIt == blueprints.constEnd()) {
        return QString();
    }

    QString filename(id);

    filename.prepend('/');
    
    for (auto it = props.constBegin(), end = props.constEnd(); it != end; ++it) {
        QString value = it->read(obj).toString();
        value.replace('#', '-');
        filename.append('-');
        filename.append(value);
    }

    filename.append(".svgt");

    auto dataIt = cache.constFind(filename);
    if (dataIt == cache.constEnd()) {
        cache[filename] = blueprintIt->construct(props, obj);
    }

    return filename;
}

svgt::Engine::Engine(QObject* parent)
    : QObject(parent)
    , impl(std::make_unique<Impl>())
{
}

svgt::Engine::~Engine() = default;

svgt::Engine::FileIdPtr svgt::Engine::getFileId(const QString& fileName)
{
    
    if (fileName.isEmpty()) {
        return nullptr;
    }

    auto id = impl->generator.fileNameId(fileName);

    Q_ASSERT_X(!id.isEmpty(), qPrintable(fileName), "failed to get file id");

    if (id.isEmpty()) {
        return nullptr;
    }

    qDebug() << fileName << "->" << id;

    return std::make_shared<FileIdImpl>(id);
}

QVector<QString> svgt::Engine::getRequiredProperties(const svgt::Engine::FileIdPtr& id)
{
    auto fileId = std::static_pointer_cast<FileIdImpl>(id);
    if (!fileId) {
        return QVector<QString>();
    }

    return impl->generator.requiredProps(fileId->id());
}

QString svgt::Engine::getDestination(const svgt::Engine::FileIdPtr& id, const QVector<QMetaProperty>& metaProps, const QObject* obj)
{
    auto fileId = std::static_pointer_cast<FileIdImpl>(id);
    if (!fileId || !obj) {
        return QString();
    }

    return impl->generator.getFilename(fileId->id(), metaProps, obj);
}