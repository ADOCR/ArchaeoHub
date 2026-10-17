#pragma once
#include <QString>
#include <QVariantList>
#include <QVariantMap>
namespace Exports {
QString plain(const QVariantList &segments);
QString stamp(qint64 milliseconds, bool comma = true);
QVariantMap write(const QString &folder, const QString &name, const QVariantList &segments);
void writeMinutes(const QString &folder, const QString &text);
}
