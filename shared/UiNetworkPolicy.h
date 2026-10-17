#pragma once
#include <QQmlNetworkAccessManagerFactory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QCoreApplication>
#include <atomic>
#include <memory>

namespace workbench {
class DeniedUiReply final:public QNetworkReply {
public:
    DeniedUiReply(const QNetworkRequest &request,QNetworkAccessManager::Operation operation,QObject *parent):QNetworkReply(parent){
        setRequest(request);setUrl(request.url());setOperation(operation);
        setError(QNetworkReply::ContentAccessDenied,QCoreApplication::translate("UiNetworkPolicy","Los recursos de la interfaz deben ser locales."));open(ReadOnly|Unbuffered);
        QTimer::singleShot(0,this,[this]{setFinished(true);emit errorOccurred(error());emit finished();});
    }
    void abort() override{}
protected:qint64 readData(char*,qint64) override{return -1;}
};
class LocalUiNetworkFactory final:public QQmlNetworkAccessManagerFactory {
    class Manager final:public QNetworkAccessManager {
        std::shared_ptr<std::atomic_int> m_blocked;
    public:Manager(QObject *parent,std::shared_ptr<std::atomic_int> blocked):QNetworkAccessManager(parent),m_blocked(std::move(blocked)){}
    protected:QNetworkReply *createRequest(Operation operation,const QNetworkRequest &request,QIODevice *outgoing=nullptr) override{
        const auto url=request.url();const auto path=url.toLocalFile();
        const bool remoteFile=url.scheme()=="file"&&(!url.host().isEmpty()||path.startsWith("//")||path.startsWith("\\\\"));
        if(remoteFile||!QStringList{"file","qrc"}.contains(url.scheme())){m_blocked->fetch_add(1);return new DeniedUiReply(request,operation,this);}
        return QNetworkAccessManager::createRequest(operation,request,outgoing);
    }};
public:
    std::shared_ptr<std::atomic_int> blocked=std::make_shared<std::atomic_int>(0);
    QNetworkAccessManager *create(QObject *parent) override{return new Manager(parent,blocked);}
};
}
