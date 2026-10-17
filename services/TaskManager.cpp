#include "TaskManager.h"
#include <QtConcurrentRun>
#include <QPromise>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

namespace workbench {
TaskManager::TaskManager(Store &store,QObject *parent):QObject(parent),m_store(store){}
TaskManager::~TaskManager(){for(auto watcher:m_running)watcher->cancel();}
QString TaskManager::hashFile(const QString &path){
    if(activeCount()>=2)return {};
    const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVariantMap state{{"id",id},{"kind","file.hash"},{"name",QFileInfo(path).fileName()},{"state","queued"},{"progress",0.0}};
    m_store.saveTask(state);m_states[id]=state;
    auto watcher=new QFutureWatcher<HashResult>(this);m_running[id]=watcher;
    connect(watcher,&QFutureWatcher<HashResult>::progressValueChanged,this,[this,id](int progress){
        if(!m_states.contains(id))return;
        auto &s=m_states[id];if(s["state"]!="cancelling")s["state"]="running";s["progress"]=progress/100.0;m_store.saveTask(s);emit changed();
    });
    connect(watcher,&QFutureWatcher<HashResult>::finished,this,[this,id,watcher]{
        auto s=m_states.take(id);
        if(watcher->isCanceled())s["state"]="cancelled";
        else {
            const auto result=watcher->result();s["state"]=result.error.isEmpty()?"succeeded":"failed";
            s["result"]=result.hash;s["error"]=result.error;if(result.error.isEmpty())s["progress"]=1.0;
        }
        m_store.saveTask(s);m_running.remove(id);watcher->deleteLater();emit changed();
    });
    watcher->setFuture(QtConcurrent::run([path](QPromise<HashResult> &promise){
        promise.setProgressRange(0,100);QFile file(path);
        if(!file.open(QIODevice::ReadOnly)){promise.addResult(HashResult{{},"file.unreadable"});return;}
        const auto size=file.size();qint64 read=0;QCryptographicHash hash(QCryptographicHash::Sha256);
        while(!file.atEnd()){
            if(promise.isCanceled())return;
            const auto data=file.read(1048576);
            if(data.isEmpty()&&file.error()!=QFileDevice::NoError){promise.addResult(HashResult{{},"file.read_failed"});return;}
            hash.addData(data);read+=data.size();promise.setProgressValue(size>0?int(qMin(qint64(99),read*100/size)):99);
        }
        promise.addResult(HashResult{QString::fromLatin1(hash.result().toHex()),{}});promise.setProgressValue(100);
    }));
    emit changed();return id;
}
void TaskManager::cancel(const QString &id){
    if(m_external.contains(id)){auto callback=m_external.value(id);auto &s=m_states[id];s["state"]="cancelling";m_store.saveTask(s);callback();emit changed();return;}
    if(auto watcher=m_running.value(id)){watcher->cancel();auto &s=m_states[id];s["state"]="cancelling";m_store.saveTask(s);emit changed();}
}
void TaskManager::cancelAll(){const auto ids=m_running.keys()+m_external.keys();for(const auto &id:ids)cancel(id);}
QString TaskManager::beginExternal(const QString &name,const QString &kind,std::function<void()> cancel){
    const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);m_external[id]=std::move(cancel);
    m_states[id]={{"id",id},{"kind",kind},{"name",name},{"state","running"},{"progress",0.0}};
    m_store.saveTask(m_states[id]);emit changed();return id;
}
void TaskManager::externalProgress(const QString &id,double progress){
    if(!m_external.contains(id))return;auto &s=m_states[id];s["progress"]=qBound(0.0,progress,1.0);m_store.saveTask(s);emit changed();
}
void TaskManager::endExternal(const QString &id,const QString &state,const QString &result,const QString &error){
    if(!m_external.remove(id))return;auto s=m_states.take(id);s["state"]=state;s["result"]=result;s["error"]=error;if(state=="succeeded")s["progress"]=1.0;
    m_store.saveTask(s);emit changed();
}
void TaskManager::recordLaunch(const QString &name,bool ok,const QString &code){
    m_store.saveTask({{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"kind","tool.launch"},{"name",name},
                     {"state",ok?"succeeded":"failed"},{"progress",1.0},{"error",code}});emit changed();
}
}
