#ifndef __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__
#define __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__

#include <memory>
#include <QSet>

class QDir; class QUrl;

namespace svgt
{

struct Blueprint
{
    virtual ~Blueprint() {}
    virtual QString construct(const QMap<QString, QString>&) = 0;
}; // struct Blueprint

class Engine
{
    struct Impl;
    std::unique_ptr<Impl> impl;
    Engine();
public:
    ~Engine();
    static Engine& instance();
    std::weak_ptr<Blueprint> blueprint(const QUrl&);
}; // Engine

} // namespace svgt

#endif // __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__