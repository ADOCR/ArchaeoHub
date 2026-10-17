#pragma once
#include "services/Registry.h"
#include "services/TaskManager.h"
#include "services/Logger.h"
#include "core/ProjectTemplate.h"
#include <QObject>
#include <QUrl>

namespace workbench {
class Hub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString productName READ productName CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString route READ route NOTIFY changed)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY changed)
    Q_PROPERTY(QString notice READ notice NOTIFY changed)
    Q_PROPERTY(bool noticeError READ noticeError NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QVariantList projects READ projects NOTIFY changed)
    Q_PROPERTY(QVariantList recentFiles READ recentFiles NOTIFY changed)
    Q_PROPERTY(QVariantMap pendingManifest READ pendingManifest NOTIFY changed)
    Q_PROPERTY(QVariantList commands READ commands NOTIFY changed)
    Q_PROPERTY(QStringList projectFolders READ projectFolders CONSTANT)
    Q_PROPERTY(bool creatingProject READ creatingProject NOTIFY changed)
public:
    Hub(Store &store,Registry &registry,Volumes &volumes,TaskManager &tasks,Logger &logger,const QString &name,const QString &version,QObject *parent=nullptr);
    QString productName() const{return m_name;}
    QString version() const{return m_version;}
    QString route() const{return m_history.at(m_historyIndex);}
    bool canGoBack() const{return m_historyIndex>0;}
    QString notice() const{return m_notice;}
    bool noticeError() const{return m_noticeError;}
    int revision() const{return m_revision;}
    QVariantList projects() const{return m_store.projects();}
    QVariantList recentFiles() const{return m_store.recentFiles();}
    QVariantMap pendingManifest() const{return m_pending;}
    QVariantList commands() const;
    QStringList projectFolders() const{return m_projectTemplate.folders;}
    bool creatingProject() const{return m_creatingProject;}
    Q_INVOKABLE void navigate(const QString &route);
    Q_INVOKABLE void goBack();
    Q_INVOKABLE QVariantMap tool(const QString &id) const{return m_registry.details(id);}
    Q_INVOKABLE QVariantMap openTool(const QString &id);
    Q_INVOKABLE bool configureTool(const QString &id,const QUrl &program,const QUrl &interpreter=QUrl());
    Q_INVOKABLE bool configureToolPaths(const QString &id,const QString &program,const QString &interpreter){return configureTool(id,QUrl::fromLocalFile(program),interpreter.isEmpty()?QUrl():QUrl::fromLocalFile(interpreter));}
    Q_INVOKABLE QString localPath(const QUrl &url) const{return url.toLocalFile();}
    Q_INVOKABLE QUrl fileUrl(const QString &path) const{return QUrl::fromLocalFile(path);}
    Q_INVOKABLE void toggleFavorite(const QString &id){m_registry.toggleFavorite(id);}
    Q_INVOKABLE void addProject(const QUrl &folder);
    Q_INVOKABLE bool createProject(const QUrl &parent,const QString &name);
    Q_INVOKABLE void openProject(int index);
    Q_INVOKABLE void openFile(const QUrl &file);
    Q_INVOKABLE void openFolder(const QUrl &folder);
    Q_INVOKABLE void openLicenses();
    Q_INVOKABLE void openRecentFile(int index);
    Q_INVOKABLE void hashFile(const QUrl &file);
    Q_INVOKABLE void reviewManifest(const QUrl &file);
    Q_INVOKABLE void approveManifest();
    Q_INVOKABLE void dismissManifest(){m_pending.clear();emit changed();}
    Q_INVOKABLE QString statusText(const QString &code) const;
    Q_INVOKABLE QString taskStateText(const QString &state) const;
    Q_INVOKABLE QString diagnosticLog() const{return m_logger.tail();}
    Q_INVOKABLE void openDataFolder();
    Q_INVOKABLE void dismissNotice(){m_notice.clear();emit changed();}
    Q_INVOKABLE void copyText(const QString &text);
signals:
    void changed();
private:
    void message(const QString &text,bool error=false);
    void openLocation(const QVariantMap &location);
    Store &m_store;Registry &m_registry;Volumes &m_volumes;TaskManager &m_tasks;Logger &m_logger;
    QString m_name,m_version,m_notice;
    bool m_noticeError=false;
    int m_revision=0,m_historyIndex=0;
    QStringList m_history{"home"};
    QVariantMap m_pending;
    ProjectTemplate m_projectTemplate;
    QFutureWatcher<ProjectCreation> m_projectCreation;
    bool m_creatingProject=false;
};
}
