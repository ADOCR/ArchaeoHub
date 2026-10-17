#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>

namespace workbench {
struct Volume {
    QString id, key, label, mount, fileSystem, identityKind;
    qint64 available = 0, total = 0;
    bool readOnly = false, strongIdentity = false;
};
struct LocationRef {
    QString volumeId, relativePath, absoluteHint;
    QJsonObject toJson() const;
    static LocationRef fromJson(const QJsonObject &value);
};
class Paths {
public:
    static bool contained(const QString &root, const QString &path);
    static LocationRef reference(const QString &path, const QList<Volume> &volumes);
    static QString resolve(const LocationRef &ref, const QList<Volume> &volumes);
};
}
