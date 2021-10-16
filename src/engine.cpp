#include "engine.h"
#include <private/qabstractfileengine_p.h>
#include <QUrl>
#include <QDebug>

using namespace svgt;

class Generator
{

}; // class Generator

class BlueprintDetail : public Blueprint
{
public:
    BlueprintDetail(const QString& path)
    {

    }

    QString getFileName(const QMap<QString, QString>& props) const
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

    QString construct(const QMap<QString, QString>& map) const override
    {
        return getFileName(map);
    }
}; // class Blueprint

// class FileEngine : public QAbstractFileEngine
// {
//     const QByteArray& d;
//     qint64 pos;
// public:
//     FileEngine(generator& g, const QString& n) : g(g), d(g.data(n)), pos(0)
//     {
//         if (d.isEmpty()) {qCCritical() << "No svgt file or it is malformed" << n;}
//     }

//     bool open(QIODevice::OpenMode) override {return !d.isEmpty();}

//     qint64 read(char* data, qint64 maxlen) override
//     {
//         if (d.isEmpty()) return 0;
//         qint64 r = qMin(maxlen, d.size() - pos);
//         memcpy(data, (d.data() + pos), r);
//         return r;
//     }
//     bool seek(qint64 i) override
//     {
//         if (i >= 0 && i < d.size()) {pos = i; return true;}
//         return false;
//     }
// }; // class FileEngine



class FileHandler : public QAbstractFileEngineHandler
{
public:
    FileHandler()
    : QAbstractFileEngineHandler()
    {
        qDebug() << Q_FUNC_INFO;
    }

    QAbstractFileEngine* create(const QString& name) const
    {
        if (name.endsWith(".svgt")) {
            qDebug() << Q_FUNC_INFO << name;
        }
        //if (name.endsWith(".svgt")) return new detail::file_engine(g, name);
        return nullptr;
    }
}; // FileHandler

struct Engine::Impl
{
    FileHandler fileHandler;
    QSet<QString> emptySet;
    QMap<QString, std::shared_ptr<Blueprint>> blueprints;

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
    if (!impl->blueprints.contains(path)) {
        std::shared_ptr<Blueprint> ptr(new BlueprintDetail(path));
        impl->blueprints.insert(path, ptr);
    }
    return impl->blueprints[path];
}