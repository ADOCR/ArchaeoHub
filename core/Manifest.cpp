#include "Manifest.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>

namespace workbench {
ManifestResult parseManifest(const QByteArray &bytes, const QString &source) {
    auto fail = [](const QString &message) { ManifestResult r; r.error = message; return r; };
    if (bytes.size() > 262144) return fail("manifest.too_large");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return fail("manifest.invalid_json");
    const auto o = document.object();
    if (!o.value("schemaVersion").isDouble() || o.value("schemaVersion").toDouble() != 1)
        return fail("manifest.unsupported_schema");
    ToolDescriptor t;
    t.id = o.value("id").toString();
    if (!QRegularExpression("^[a-z][a-z0-9]*(?:[.-][a-z0-9]+)+$").match(t.id).hasMatch() || t.id.size() > 128)
        return fail("manifest.invalid_id");
    auto localized = [&](const char *key) {
        auto value = o.value(QLatin1String(key));
        return value.isObject() ? value.toObject().value("es").toString(value.toObject().value("en").toString()) : value.toString();
    };
    t.name = localized("name"); t.description = localized("description");
    if (t.name.trimmed().isEmpty() || t.name.size() > 200 || t.description.size() > 4000)
        return fail("manifest.invalid_text");
    t.version = o.value("version").toString();
    if (!QRegularExpression("^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)(?:-[0-9A-Za-z.-]+)?(?:\\+[0-9A-Za-z.-]+)?$").match(t.version).hasMatch())
        return fail("manifest.invalid_version");
    t.runtime = o.value("runtime").toString();
    if (!QStringList{"external", "python", "script", "native", "future-plugin"}.contains(t.runtime))
        return fail("manifest.invalid_runtime");
    for (auto pair : {qMakePair("categoryIds", &t.categories), qMakePair("capabilities", &t.capabilities),
                      qMakePair("supportedFileTypes", &t.fileTypes)}) {
        if (!o.value(QLatin1String(pair.first)).isArray()) return fail("manifest.invalid_list");
        for (const auto &value : o.value(QLatin1String(pair.first)).toArray()) {
            if (!value.isString() || value.toString().isEmpty() || value.toString().size() > 128)
                return fail("manifest.invalid_list_item");
            pair.second->append(value.toString());
        }
    }
    if (t.categories.isEmpty()) return fail("manifest.missing_category");
    t.planned = o.value("status").toString() == "planned";
    const auto entry = o.value("entrypoint").toObject();
    t.entrypoint = entry.value("path").toString();
    if (!t.planned && entry.value("binding").toString() != "selected-installation" && t.entrypoint.isEmpty())
        return fail("manifest.missing_entrypoint");
    if (!t.entrypoint.isEmpty()) {
        const auto path = t.entrypoint;
        if (path.startsWith('/') || path.startsWith('\\') || path.contains(':') ||
            path.split(QRegularExpression("[/\\\\]")).contains("..")) return fail("manifest.unsafe_entrypoint");
    }
    t.icon = o.value("icon").toString("tools");
    t.manifestHash = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
    t.source = source; t.json = o;
    return {t, {}};
}
}
