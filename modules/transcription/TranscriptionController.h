#pragma once
#include "services/TaskManager.h"
#include <QProcess>
#include <QSettings>
#include <QJsonObject>
#include <QUrl>
namespace workbench::transcription {
class TranscriptionController final:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(bool ready READ ready NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString modelsFolder READ modelsFolder NOTIFY changed)
public:
    TranscriptionController(TaskManager &tasks,const QString &directory,const QString &legacyProgram,QObject *parent=nullptr);
    ~TranscriptionController() override;
    QVariantMap state() const{return m_state.toVariantMap();}bool ready() const{return m_ready;}
    bool busy() const{return m_state["busy"].toBool()||m_state["downloading"].toBool();}
    QString message() const{return m_message;}QString modelsFolder() const{return m_preferences.value("modelsFolder").toString();}
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void addFiles(const QVariantList &files);
    Q_INVOKABLE void addFolder(const QUrl &folder);
    Q_INVOKABLE void useModelsFolder(const QUrl &folder);
    Q_INVOKABLE void settings(const QVariantMap &settings);
    Q_INVOKABLE void removeFile(int index);Q_INVOKABLE void clearFiles();
    Q_INVOKABLE void start();Q_INVOKABLE void cancel();Q_INVOKABLE void downloadModel();
    Q_INVOKABLE void openHistory(int index);Q_INVOKABLE void dismissError();
    void command(QJsonObject request);
signals:
    void changed();void batchFinished(bool ok);
private:
    void receive();void finishTask(bool ok,const QString &error={});
    TaskManager &m_tasks;QSettings m_preferences;QProcess m_process;QJsonObject m_state;
    QByteArray m_buffer;QString m_message,m_task;bool m_ready=false,m_downloadTask=false,m_cancelled=false;
};
}
