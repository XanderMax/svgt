#ifndef __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__
#define __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__

#include <memory>
#include <QSet>
#include <QObject>

class QDir; class QUrl;

namespace svgt
{

struct Blueprint
{
    virtual ~Blueprint() {}
    virtual QString construct(const QMap<QString, QByteArray>&) = 0;
}; // struct Blueprint

class Engine : public QObject
{
    Q_OBJECT
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    Engine(QObject* parent = nullptr);
    ~Engine();
    QString getDestination(const QUrl&, const QVector<QMetaProperty>&, const QObject*);
}; // Engine

} // namespace svgt

#endif // __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__