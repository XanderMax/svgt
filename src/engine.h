#ifndef __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__
#define __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__

#include <memory>
#include <QObject>

namespace svgt
{
class Engine : public QObject
{
    Q_OBJECT
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    struct FileId {virtual ~FileId() {}};
    using FileIdPtr = std::shared_ptr<FileId>;
    using FileIdWPtr = std::weak_ptr<FileId>;
    
    Engine(QObject* parent = nullptr);
    ~Engine();

    Q_INVOKABLE void clearCache();

    FileIdPtr getFileId(const QString&);

    QVector<QString> getRequiredProperties(const FileIdPtr&);

    QString getDestination(const FileIdPtr&, const QVector<QMetaProperty>&, const QObject*);
}; // Engine
} // namespace svgt

#endif // __ODIEOIDNEODINEOIDNEOIDNEOINDOEINDN__