#pragma once
#include "Store.h"
#include "Volumes.h"
#include "core/Module.h"
#include <QAbstractListModel>
#include <QFileSystemWatcher>

namespace workbench {
class Registry : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {IdRole=Qt::UserRole+1,NameRole,DescriptionRole,VersionRole,RuntimeRole,
               IconRole,CategoryRole,PlannedRole,FavoriteRole,AvailabilityRole,LastUsedRole};
    Registry(Store &store,Volumes &volumes,const QString &directory,QObject *parent=nullptr);
    int rowCount(const QModelIndex &parent={}) const override;
    QVariant data(const QModelIndex &index,int role) const override;
    QHash<int,QByteArray> roleNames() const override;
    QVariantMap details(const QString &id) const;
    ToolDescriptor descriptor(const QString &id) const;
    QString availability(const QString &id) const;
    QString workspace(const QString &id) const;
    LaunchRequest launchRequest(const QString &id) const;
    QString configure(const QString &id,const QString &program,const QString &interpreter);
    QString importManifest(const QString &source,const QString &expectedHash);
    void toggleFavorite(const QString &id);
    void recordUse(const QString &id);
    void reload();
signals:
    void revised();
private:
    Store &m_store;
    Volumes &m_volumes;
    QString m_directory;
    QList<ToolDescriptor> m_tools;
    QFileSystemWatcher m_watcher;
};
}
