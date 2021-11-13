#ifndef __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__
#define __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__

#include <memory>
#include <QSet>
#include <QObject>

class QDir; class QUrl;

namespace svgt
{

class Foreman
{
public:
    virtual ~Foreman() {}

    virtual QString destination(const QObject*) = 0;
};

class Engine : public QObject
{
    Q_OBJECT
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    Engine(QObject* parent = nullptr);
    ~Engine();
    std::shared_ptr<Foreman> foreman(const QString&, const QMap<QString, QMetaProperty>&);
}; // Engine

} // namespace svgt

#endif // __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__