#pragma once
#include <QJsonObject>
#include <QStringList>

namespace workbench {
struct ToolDescriptor {
    QString id, name, description, version, runtime, icon;
    QStringList categories, capabilities, fileTypes;
    QString entrypoint, manifestHash, source;
    bool planned = false;
    QJsonObject json;
};
struct ManifestResult {
    ToolDescriptor tool;
    QString error;
    explicit operator bool() const { return error.isEmpty(); }
};
ManifestResult parseManifest(const QByteArray &bytes, const QString &source = {});
}
