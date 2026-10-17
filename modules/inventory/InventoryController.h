#pragma once
#include "services/TaskManager.h"
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <memory>
namespace workbench::inventory {
class InventoryController final:public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool error READ error NOTIFY changed)
    Q_PROPERTY(bool opened READ opened NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString databasePath READ databasePath NOTIFY changed)
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QVariantMap preview READ preview NOTIFY changed)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
public:
    InventoryController(TaskManager &tasks,const QString &profile,QObject *parent=nullptr);
    ~InventoryController() override;
    bool busy() const{return m_busy;}bool error() const{return m_error;}bool opened() const{return !m_database.isEmpty();}
    QString message() const{return m_message;}QString databasePath() const{return m_database;}
    QVariantMap state() const{return m_state;}QVariantMap preview() const{return m_preview;}QVariantList rows() const{return m_state.value("rows").toList();}
    Q_INVOKABLE void create(const QUrl &file,const QVariantMap &metadata);
    Q_INVOKABLE void open(const QUrl &file);
    Q_INVOKABLE void reviewImport(const QUrl &source);
    Q_INVOKABLE void importReviewed();
    Q_INVOKABLE void add(const QVariantMap &record);
    Q_INVOKABLE void search(const QString &query,bool pendingOnly=false,int page=0);
    Q_INVOKABLE void exportFile(const QUrl &file);
    Q_INVOKABLE void cancel();
    bool request(QJsonObject value);
    QJsonObject lastResult() const{return m_result;}
signals:
    void changed();void completed(bool ok);
private:
    void receive();void finish(bool ok,const QString &message);
    TaskManager &m_tasks;QString m_profile,m_database,m_message,m_task,m_operation;
    QVariantMap m_state,m_preview;QJsonObject m_request,m_result,m_reply;QProcess m_process;QTimer m_timer;
    std::unique_ptr<QTemporaryDir> m_cache;QByteArray m_buffer;bool m_busy=false,m_error=false,m_cancelled=false;
};
}
