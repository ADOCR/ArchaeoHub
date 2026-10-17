#pragma once
#include "services/TaskManager.h"
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QJsonObject>
#include <QUrl>

namespace workbench::catalog {
class CatalogController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool opened READ opened NOTIFY changed)
    Q_PROPERTY(bool error READ error NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QVariantList items READ items NOTIFY changed)
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QVariantMap selection READ selection NOTIFY changed)
    Q_PROPERTY(QVariantMap remembered READ remembered CONSTANT)
    Q_PROPERTY(bool migrationRequired READ migrationRequired NOTIFY changed)
    Q_PROPERTY(int matched READ matched NOTIFY changed)
    Q_PROPERTY(int offset READ offset NOTIFY changed)
    Q_PROPERTY(QString proposedName READ proposedName NOTIFY changed)
    Q_PROPERTY(QVariantMap draft READ draft WRITE setDraft)
    Q_PROPERTY(QVariantMap project READ project NOTIFY changed)
    Q_PROPERTY(bool awaitingInput READ awaitingInput NOTIFY changed)
    Q_PROPERTY(QString activeFilter READ activeFilter NOTIFY changed)
public:
    CatalogController(TaskManager &tasks,const QString &directory,QObject *parent=nullptr);
    ~CatalogController() override;
    bool busy() const{return m_busy;}bool opened() const{return !m_state.isEmpty();}bool error() const{return m_error;}
    QString message() const{return m_message;}QVariantList items() const{return m_items;}
    QVariantMap state() const{return m_state.toVariantMap();}QVariantMap selection() const{return m_selection.toVariantMap();}
    QVariantMap remembered() const;bool migrationRequired() const{return m_migrationRequired;}
    int matched() const{return m_matched;}int offset() const{return m_offset;}QString proposedName() const{return m_proposed;}
    QVariantMap draft() const{return m_draft;}void setDraft(const QVariantMap &draft){m_draft=draft;}
    QVariantMap project() const{return m_project.toVariantMap();}bool awaitingInput() const{return m_awaitingInput;}
    QString activeFilter() const{return m_filter;}
    Q_INVOKABLE void openCatalog(const QString &source,const QString &output,const QString &site,const QString &operation,bool confirmMigration=false);
    Q_INVOKABLE void loadCatalog(const QUrl &location,const QString &source=QString(),const QString &site=QString(),const QString &operation=QString(),bool confirmMigration=false);
    Q_INVOKABLE void createCatalog(const QString &source,const QString &output,const QString &site,const QString &operation);
    Q_INVOKABLE void page(const QString &filter,const QString &query,int offset=0);
    Q_INVOKABLE void select(const QString &id);
    Q_INVOKABLE void navigate(int delta);
    Q_INVOKABLE void propose(const QVariantMap &form);
    Q_INVOKABLE void save(const QVariantMap &form,const QVariantList &manualGps);
    Q_INVOKABLE void saveAndNext(const QVariantMap &form,const QVariantList &manualGps);
    Q_INVOKABLE void nextGps();
    Q_INVOKABLE void correct(const QVariantMap &form);
    Q_INVOKABLE void skip();
    Q_INVOKABLE void withdraw(bool confirmed);
    Q_INVOKABLE void discard(bool confirmed);
    Q_INVOKABLE void manualGps(const QVariantList &values);
    Q_INVOKABLE void exportGps(const QUrl &file);
    Q_INVOKABLE void closeCatalog();
    bool request(QJsonObject object);
signals:
    void changed();
    void selected();
    void saved();
    void completed();
    void projectDiscovered();
    void sessionReset();
private:
    void receive();void fail(const QString &message);void act(const QString &op,QJsonObject extra={});
    TaskManager &m_tasks;QSettings m_preferences;QProcess m_process;QTimer m_deadline;
    QString m_cache,m_message,m_operation,m_task,m_filter="Todas",m_query,m_proposed;
    QByteArray m_buffer;QJsonObject m_state,m_selection,m_pending,m_project;QVariantList m_items;QVariantMap m_draft;
    bool m_busy=false,m_error=false,m_migrationRequired=false,m_awaitingInput=false;int m_requestId=0,m_matched=0,m_offset=0;
};
}
