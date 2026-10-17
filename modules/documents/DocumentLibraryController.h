#pragma once
#include "services/TaskManager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <memory>
namespace workbench::documents {
class DocumentLibraryController final:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList inputs READ inputs NOTIFY changed)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QVariantList topics READ topics NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool hasPlan READ hasPlan NOTIFY changed)
    Q_PROPERTY(bool hasAnalysis READ hasAnalysis NOTIFY changed)
    Q_PROPERTY(QVariantList connections READ connections NOTIFY changed)
    Q_PROPERTY(bool error READ error NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(int readyCount READ readyCount NOTIFY changed)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString resultFolder READ resultFolder NOTIFY changed)
    Q_PROPERTY(QString destination READ destination WRITE setDestination NOTIFY changed)
public:
    DocumentLibraryController(TaskManager &tasks,const QString &profile,QObject *parent=nullptr,quint16 localPort=11434,bool conversionOnly=false);
    ~DocumentLibraryController() override;
    QVariantList inputs() const;QVariantList rows() const{return m_rows;}QVariantList topics() const{return m_topics;}
    bool busy() const{return m_busy;}bool hasPlan() const{return !m_planDigest.isEmpty()&&m_ready>0;}
    bool hasAnalysis() const{return m_analysis.contains("documents");}
    QVariantList connections() const{return m_analysis["relations"].toArray().toVariantList();}
    bool error() const{return m_error;}double progress() const{return m_progress;}
    int readyCount() const{return m_ready;}int failedCount() const{return m_failed;}
    QString message() const{return m_message;}QString resultFolder() const{return m_resultFolder;}
    QString destination() const{return m_destination;}void setDestination(const QString &path){if(!m_busy){m_destination=path;emit changed();}}
    Q_INVOKABLE void addInputs(const QVariantList &urls);Q_INVOKABLE void removeInput(int index);
    Q_INVOKABLE void analyze();Q_INVOKABLE void exportLibrary(const QUrl &parent,bool automaticLinks=true);Q_INVOKABLE void cancel();
    Q_INVOKABLE void enrich(const QString &model);
    Q_INVOKABLE QString preview(const QString &kind,int index) const;
signals:
    void changed();void completed(bool ok);
private:
    void invalidate();void start(const QString &op,QJsonObject request);void receive();void finish(bool ok,const QString &message);
    TaskManager &m_tasks;QString m_profile,m_message,m_resultFolder,m_planDigest,m_task,m_operation,m_destination;
    QStringList m_inputs;QVariantList m_rows,m_topics;QProcess m_process;QTimer m_deadline;
    std::unique_ptr<QTemporaryDir> m_cache;QByteArray m_buffer;QJsonObject m_reply;
    QJsonObject m_analysis;QString m_planPath,m_pendingPlan;
    quint16 m_port;
    bool m_conversionOnly;
    bool m_busy=false,m_error=false,m_cancelled=false;double m_progress=0;int m_ready=0,m_failed=0;
};
}
