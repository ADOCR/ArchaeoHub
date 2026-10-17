#pragma once
#include <QJsonObject>
#include <QNetworkReply>

namespace workbench {
inline bool isRemoteModel(const QJsonObject &entry,const QString &name={}) {
    const auto remote=[](QJsonValue value){return !value.isUndefined()&&!value.isNull()&&(!value.isString()||!value.toString().isEmpty());};
    return remote(entry.value("remote_host"))||remote(entry.value("remote_model"))
        ||name.contains("cloud",Qt::CaseInsensitive)||entry.value("name").toString().contains("cloud",Qt::CaseInsensitive)
        ||entry.value("model").toString().contains("cloud",Qt::CaseInsensitive);
}
inline void boundLocalReply(QNetworkReply *reply,qint64 limit=4*1024*1024) {
    reply->setReadBufferSize(limit+1);
    QObject::connect(reply,&QNetworkReply::readyRead,reply,[reply,limit]{if(reply->bytesAvailable()>limit)reply->abort();});
}
}
