#pragma once
#include "core/Manifest.h"
#include "core/Paths.h"
#include <QSqlDatabase>
#include <QVariantList>

namespace workbench {
class Store {
public:
    explicit Store(const QString &directory);
    ~Store();
    Store(const Store &) = delete;
    Store &operator=(const Store &) = delete;
    void registerTool(const ToolDescriptor &tool);
    QVariantMap installation(const QString &toolId) const;
    void configure(const QString &toolId, const LocationRef &program, const LocationRef &interpreter, const QString &hash);
    void setFavorite(const QString &id, bool favorite);
    bool isFavorite(const QString &id) const;
    void recordUse(const QString &id);
    QString lastUsed(const QString &id) const;
    QString bindVolume(const Volume &volume);
    QList<Volume> bindVolumes(QList<Volume> volumes);
    QVariantList knownVolumes() const;
    QVariantList projects() const;
    void addProject(const QString &name, const LocationRef &folder);
    QVariantList recentFiles() const;
    void recordFile(const QString &name, const LocationRef &location);
    void saveTask(const QVariantMap &task);
    QVariantList tasks() const;
    QString databasePath() const;
private:
    void migrate();
    QSqlDatabase m_db;
};
}
