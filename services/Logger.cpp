#include "Logger.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QMutexLocker>
namespace workbench {
Logger::Logger(const QString &directory) {
    const auto folder=QDir(directory).filePath("logs"); QDir().mkpath(folder); m_path=QDir(folder).filePath("workbench.jsonl");
}
void Logger::write(const QString &severity,const QString &code,const QJsonObject &context){
    QMutexLocker guard(&m_mutex);
    if(QFileInfo(m_path).size()>524288){
        QFile::remove(m_path+".3"); QFile::rename(m_path+".2",m_path+".3");
        QFile::rename(m_path+".1",m_path+".2"); QFile::rename(m_path,m_path+".1");
    }
    QFile file(m_path); if(file.open(QIODevice::Append)) file.write(QJsonDocument(QJsonObject{
        {"at",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"severity",severity},{"code",code},{"context",context}}).toJson(QJsonDocument::Compact)+'\n');
}
QString Logger::tail() const {
    QMutexLocker guard(&m_mutex); QFile file(m_path); if(!file.open(QIODevice::ReadOnly))return {};
    if(file.size()>65536){file.seek(file.size()-65536);file.readLine();}return QString::fromUtf8(file.readAll());
}
}
