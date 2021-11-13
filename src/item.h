#ifndef __UBEUYCBUYOECBUYEBUEYDDDXXVEDUTCEUT__
#define __UBEUYCBUYOECBUYEBUEYDDDXXVEDUTCEUT__

#include <QObject>
#include <QQuickItem>
#include <QQmlParserStatus>
#include <memory>
#include "engine.h"

namespace svgt
{

class Item : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(Engine* engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(QObject* object READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(QUrl source WRITE setSource READ source NOTIFY sourceChanged)
    Q_PROPERTY(QString destination READ destination NOTIFY destinationChanged)
    
public:

    Item(QObject* parent = nullptr);
    ~Item();

    void classBegin() override;
    void componentComplete() override;

    Engine* engine() const;
    void setEngine(Engine*);

    QObject* object() const;
    void setObject(QObject*);

    QUrl source() const;
    void setSource(const QUrl&);

    QString destination() const;

signals:
    void engineChanged();
    void objectChanged();
    void sourceChanged();
    void destinationChanged();

private slots:
    void propertiesUpdated();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

}; // class item

} // namespace svgt

#endif // __UBEUYCBUYOECBUYEBUEYDDDXXVEDUTCEUT__