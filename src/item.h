#ifndef __UBEUYCBUYOECBUYEBUEYDDDXXVEDUTCEUT__
#define __UBEUYCBUYOECBUYEBUEYDDDXXVEDUTCEUT__

#include <QObject>
#include <QQuickItem>
#include <QQmlParserStatus>
#include <memory>

namespace svgt
{

class Item : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQuickItem* item READ item WRITE setItem NOTIFY itemChanged)
    Q_PROPERTY(QString destination READ destination NOTIFY destinationChanged)
    Q_PROPERTY(QString source WRITE setSource READ source NOTIFY sourceChanged)
    
public:

    Item(QObject* parent = nullptr);
    ~Item();

    void classBegin() override;
    void componentComplete() override;

    QQuickItem* item() const;
    void setItem(QQuickItem*);

    QString source() const;
    void setSource(const QString&);

    QString destination() const;

signals:
    void itemChanged();
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