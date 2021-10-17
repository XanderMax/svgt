#include "engine.h"
#include <private/qabstractfileengine_p.h>
#include <tuple>
#include <vector>
#include <QUrl>
#include <QDebug>
#include <QFile>
#include <QByteArray>

using namespace svgt;

namespace detail
{

class generator
{
    QMap<QString, QByteArray> precompiled;
public:
    generator() = default;
    generator(const generator&) = delete;
    generator(generator&&) = delete;

    bool contains(const QString& path) const
    {
        auto data = precompiled[path];
        qDebug() << Q_FUNC_INFO << data.size();
        return precompiled.contains(path) && !precompiled[path].isEmpty();
    }

    const QByteArray& getData(const QString& path)
    {
        if (!contains(path)) {
            qDebug() << Q_FUNC_INFO << "No data for" << path;
        }
        return precompiled[path];
    }

    void putData(const QString& path, const QByteArray& data)
    {
        if (data.isEmpty()) return;
        qDebug() << Q_FUNC_INFO << path << data.size();
        precompiled[path] = data;
    }
}; // class generator

class blueprint : public ::Blueprint
{
    struct chunk
    {
        QByteArray data; QString name;
        chunk(QByteArray data, QString name) : data(data), name(name) {}
    };
    std::vector<chunk> bpChunks;
    QByteArray data;
    generator& gen;
    QMap<QString, QByteArray> precompiled;

    QByteArray compile(const QMap<QString, QByteArray>& map) const
    {
        int size = 0;
        for (const auto& ch : bpChunks) {
            if (ch.name.isEmpty()) {
                size += ch.data.size();
                continue;
            }
            auto it = map.find(ch.name);
            if (it == map.end()) return QByteArray();
            size += ch.data.size() + it->size();
        }

        QByteArray data(size, '\0');

        int pos = 0;
        for (const auto& ch : bpChunks) {
            data.replace(pos, ch.data.size(), ch.data);
            pos += ch.data.size();
            if (ch.name.isEmpty()) continue;
            auto it = map.find(ch.name);
            if (it == map.end()) return QByteArray();
            data.replace(pos, it->size(), it.value());
            pos += it->size();
        }

        return data;
    }

    void buildBp(QByteArray& data)
    {
        qDebug() << Q_FUNC_INFO << data.size();
        bpChunks.clear();
        if (data.isEmpty()) return;
        if (data.size() <= 4) {
            bpChunks.push_back(chunk{std::move(data), QString()});
            return;
        }

// 0 1 2 4 5 6 7
// { { h p } } x
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
                bpChunks.emplace_back(std::move(dataChunk), std::move(property));
                beg = i + 2;
                openBrace = -1;
            }
        }

        if (beg < data.size()) {
            bpChunks.emplace_back(data.mid(beg, -1), QString());
        }

        for (const auto& ch : bpChunks) {
            qDebug() << Q_FUNC_INFO << ch.data << " : " << ch.name;
        }
    }
public:
    blueprint(generator& gen, const QString& path)
    : gen(gen)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "failed to open file" << path;
            std::terminate();
        }
        data = file.readAll();
        buildBp(data);
    }

    static QString getFileName(const QMap<QString, QByteArray>& props)
    {
        QString filename;
        filename.append("/");
        for (auto it = props.begin(), end = props.end(); it != end; ++it) {
            filename.append(it.key());
            filename.append("-");
            filename.append(it.value());
        }
        filename.append(".svgt");
        return filename;
    }

    QString construct(const QMap<QString, QByteArray>& map) override
    {
        QString filename = getFileName(map);
        qDebug() << Q_FUNC_INFO << filename;
        if (!gen.contains(filename)) {
            auto data = compile(map);
            qDebug() << Q_FUNC_INFO << filename << data.size();
            gen.putData(filename, data); 
        }
        return filename;
    }
}; // class blueprint

} // namespace detail

class FileEngine : public QAbstractFileEngine
{
    const QByteArray& data;
    qint64 pos = 0;
public:
    FileEngine(const QByteArray& data)
    : QAbstractFileEngine()
    , data(data)
    {
        if (data.isEmpty()) {
            qDebug() << "something went wrong";
            std::terminate();
        }
    }

    bool open(QIODevice::OpenMode) override
    {
        return true;
    }

    qint64 read(char* storage, qint64 maxlen) override
    {
        if (data.isEmpty()) return 0;
        qint64 r = qMin(maxlen, data.size() - pos);
        memcpy(storage, data.data() + pos, r);
        return r;
    }

    bool seek(qint64 i) override
    {
        if (i >= 0 && i < data.size()) {
            pos = i;
            return true;
        }
        return false;
    }
};

class FileHandler : public QAbstractFileEngineHandler
{
    detail::generator& gen;
public:
    FileHandler(detail::generator& gen)
    : QAbstractFileEngineHandler()
    , gen(gen)
    {
        qDebug() << Q_FUNC_INFO;
    }

    QAbstractFileEngine* create(const QString& name) const
    {
        if (!name.endsWith(".svgt")) {
            return nullptr;
        }
        qDebug() << Q_FUNC_INFO << name;

        const auto& data = gen.getData(name);
        
        if (data.isEmpty()) {
            qDebug() << Q_FUNC_INFO << name << "data is empty";
            return nullptr;
        }

        return new FileEngine(data);
    }
}; // FileHandler

struct Engine::Impl
{
    detail::generator gen;
    QMap<QString, std::shared_ptr<detail::blueprint>> bps;
    FileHandler fileHandler;

    std::shared_ptr<detail::blueprint> addBlueprint(const QString& path)
    {
        qDebug() << Q_FUNC_INFO << path;
        if (!bps.contains(path)) {
            auto sp = std::make_shared<detail::blueprint>(gen, path);
            bps[path] = sp;
            return sp;
        }
        return bps[path];
    }

    std::shared_ptr<detail::blueprint> getBlueprint(const QString& path) const
    {
        return bps.value(path, nullptr);
    }
    
    
    Impl()
    : fileHandler(gen)
    {

    }
}; // Engine::Impl

Engine::Engine()
: impl(std::make_unique<Impl>())
{
}

Engine::~Engine() = default;

Engine& Engine::instance()
{
    static Engine engine;
    return engine;
}

std::weak_ptr<Blueprint> Engine::blueprint(const QUrl& url)
{
    qDebug() << Q_FUNC_INFO << url.path();
    QString path = url.path();
    return impl->addBlueprint(path);
}