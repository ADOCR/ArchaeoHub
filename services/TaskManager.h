#pragma once
#include "Store.h"
#include <QObject>
#include <QFutureWatcher>
#include <QHash>
#include <memory>
#include <functional>

namespace workbench {
struct HashResult {QString hash,error;};
class TaskManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY changed)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY changed)
public:
    explicit TaskManager(Store &store,QObject *parent=nullptr);
    ~TaskManager() override;
    QVariantList items() const {return m_store.tasks();}
    int activeCount() const {return int(m_running.size()+m_external.size());}
    QString hashFile(const QString &path);
    void recordLaunch(const QString &name,bool ok,const QString &code);
    QString beginExternal(const QString &name,const QString &kind,std::function<void()> cancel);
    void externalProgress(const QString &id,double progress);
    void endExternal(const QString &id,const QString &state,const QString &result={},const QString &error={});
    Q_INVOKABLE void cancel(const QString &id);
    Q_INVOKABLE void cancelAll();
signals:
    void changed();
private:
    Store &m_store;
    QHash<QString,QFutureWatcher<HashResult>*> m_running;
    QHash<QString,QVariantMap> m_states;
    QHash<QString,std::function<void()>> m_external;
};
}
