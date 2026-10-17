#include "Paths.h"
#include <QDir>
#include <QFileInfo>

namespace workbench {
QJsonObject LocationRef::toJson() const {
    return {{"volumeId", volumeId}, {"relativePath", relativePath}, {"absoluteHint", absoluteHint}};
}
LocationRef LocationRef::fromJson(const QJsonObject &v) {
    return {v.value("volumeId").toString(), v.value("relativePath").toString(), v.value("absoluteHint").toString()};
}
bool Paths::contained(const QString &root, const QString &path) {
    auto r = QDir::cleanPath(QDir::fromNativeSeparators(root));
    auto p = QDir::cleanPath(QDir::fromNativeSeparators(path));
#ifdef Q_OS_WIN
    constexpr auto sensitivity = Qt::CaseInsensitive;
#else
    constexpr auto sensitivity = Qt::CaseSensitive;
#endif
    if (p.compare(r, sensitivity) == 0) return true;
    if (!r.endsWith('/')) r += '/';
    return p.startsWith(r, sensitivity);
}
LocationRef Paths::reference(const QString &path, const QList<Volume> &volumes) {
    if(path.isEmpty())return {};
    const auto absolute = QFileInfo(path).absoluteFilePath();
    const Volume *best = nullptr;
    for (const auto &volume : volumes) {
        if (volume.strongIdentity && contained(volume.mount, absolute) && (!best || volume.mount.size() > best->mount.size())) best = &volume;
    }
    return best ? LocationRef{best->id, QDir(best->mount).relativeFilePath(absolute), absolute}
                : LocationRef{{}, {}, absolute};
}
QString Paths::resolve(const LocationRef &ref, const QList<Volume> &volumes) {
    if (ref.volumeId.isEmpty()) return QDir::isAbsolutePath(ref.absoluteHint) ? QDir::cleanPath(ref.absoluteHint) : QString();
    for (const auto &volume : volumes) {
        if (volume.id != ref.volumeId || !volume.strongIdentity) continue;
        if (QDir::isAbsolutePath(ref.relativePath)) return {};
        const auto path = QDir::cleanPath(QDir(volume.mount).filePath(ref.relativePath));
        if (!contained(volume.mount, path)) return {};
        const auto canonical = QFileInfo(path).canonicalFilePath();
        const auto rootCanonical = QFileInfo(volume.mount).canonicalFilePath();
        if (!canonical.isEmpty() && !rootCanonical.isEmpty() && !contained(rootCanonical, canonical)) return {};
        return path;
    }
    return {}; // No resuelvas un volumen ausente mediante una letra de unidad anterior.
}
}
