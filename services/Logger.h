#pragma once
#include <QString>
#include <QMutex>
#include <QJsonObject>
namespace workbench {
class Logger {
public:
    explicit Logger(const QString &directory);
    void write(const QString &severity,const QString &code,const QJsonObject &context = {});
    QString tail() const;
    QString path() const { return m_path; }
private:
    QString m_path;
    mutable QMutex m_mutex;
};
}
