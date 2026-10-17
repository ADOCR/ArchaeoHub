#pragma once
#include "Store.h"
#include <QObject>
#include <QFutureWatcher>
#include <QTimer>

namespace workbench {
class Volumes : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY changed)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY changed)
public:
    explicit Volumes(Store &store,QObject *parent=nullptr);
    const QList<Volume> &snapshot() const {return m_volumes;}
    QVariantList items() const;
    bool refreshing() const {return m_watcher.isRunning();}
    bool hasSnapshot() const {return m_hasSnapshot;}
    Q_INVOKABLE void refresh();
signals:
    void changed();
private:
    static QList<Volume> enumerate();
    Store &m_store;
    QList<Volume> m_volumes;
    QFutureWatcher<QList<Volume>> m_watcher;
    QTimer m_timer;
    bool m_hasSnapshot=false;
};
}
