#pragma once
#include "services/TaskManager.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QJsonArray>
namespace workbench::documents {
class DocumentController final:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList files READ files NOTIFY changed)
    Q_PROPERTY(QStringList models READ models NOTIFY changed)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY changed)
public:
    DocumentController(TaskManager &tasks,const QString &directory,QObject *parent=nullptr,quint16 localPort=11434);
    ~DocumentController() override;
    QVariantList files() const;QStringList models() const{return m_models;}
    QString model() const{return m_preferences.value("model","qwen3:8b").toString();}void setModel(const QString &model);
    bool busy() const{return m_busy;}double progress() const{return m_progress;}
    QString message() const{return m_message;}QString summary() const{return m_summary;}QStringList warnings() const{return m_warnings;}
    Q_INVOKABLE void initialize();Q_INVOKABLE void addFiles(const QVariantList &files);Q_INVOKABLE void removeFile(int index);
    Q_INVOKABLE void generate(bool detailed=false);Q_INVOKABLE void cancel();Q_INVOKABLE void exportSummary(const QUrl &file);
signals:void changed();void completed(bool ok);
private:
    void readDocuments();
    void extracted(int exitCode,QProcess::ExitStatus status);void nextChunk();void finish(bool ok,const QString &message);
    QStringList chunkText(const QStringList &blocks) const;
    TaskManager &m_tasks;QSettings m_preferences;QNetworkAccessManager m_network;QPointer<QNetworkReply> m_reply;
    QProcess m_process;QTimer m_deadline;QStringList m_paths,m_models,m_chunks,m_partials,m_warnings,m_sources;
    QJsonArray m_documents;QString m_message,m_summary,m_task;QByteArray m_buffer;int m_index=0,m_level=0,m_done=0,m_retry=0;
    bool m_busy=false,m_cancelled=false,m_detailed=false;double m_progress=0;quint16 m_port;
};
}
