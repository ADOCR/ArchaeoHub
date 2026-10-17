#include "Volumes.h"
#include <QtConcurrentRun>
#include <QStorageInfo>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QXmlStreamReader>
#include <QHash>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <QScopeGuard>
#include <cwchar>
#endif

namespace workbench {
Volumes::Volumes(Store &store,QObject *parent):QObject(parent),m_store(store){
    connect(&m_watcher,&QFutureWatcher<QList<Volume>>::finished,this,[this]{
        try {
            auto snapshot=m_store.bindVolumes(m_watcher.result());
            m_volumes=std::move(snapshot);
            m_hasSnapshot=true;
        } catch(const std::exception &) { /* Conserva la última instantánea guardada correctamente. */ }
        emit changed();
    });
    m_timer.setInterval(10000);connect(&m_timer,&QTimer::timeout,this,&Volumes::refresh);m_timer.start();
    QTimer::singleShot(0,this,&Volumes::refresh);
}
void Volumes::refresh(){if(m_watcher.isRunning())return;m_watcher.setFuture(QtConcurrent::run(&Volumes::enumerate));emit changed();}
QList<Volume> Volumes::enumerate(){
    QList<Volume> result;
#ifdef Q_OS_WIN
    // Consulta directamente las unidades locales montadas: enumerar un recurso de red desconectado
    // no debe retrasar el inventario de discos arqueológicos.
    DWORD oldMode=0;const bool changedMode=SetThreadErrorMode(SEM_FAILCRITICALERRORS,&oldMode);
    const auto restoreMode=qScopeGuard([=]{if(changedMode)SetThreadErrorMode(oldMode,nullptr);});
    wchar_t roots[512]{};
    const auto length=GetLogicalDriveStringsW(512,roots);
    if(length==0||length>=512)return result;
    for(const wchar_t *root=roots;*root;root+=std::wcslen(root)+1){
        const auto type=GetDriveTypeW(root);if(type!=DRIVE_FIXED&&type!=DRIVE_REMOVABLE)continue;
        wchar_t label[261]{},filesystem[261]{},guid[100]{};DWORD serial=0,maximum=0,flags=0;
        ULARGE_INTEGER available{},total{},free{};
        if(!GetVolumeInformationW(root,label,261,&serial,&maximum,&flags,filesystem,261)||
            !GetDiskFreeSpaceExW(root,&available,&total,&free)||total.QuadPart==0)continue;
        Volume v;v.mount=QDir::fromNativeSeparators(QString::fromWCharArray(root));v.label=QString::fromWCharArray(label);
        v.fileSystem=QString::fromWCharArray(filesystem);v.total=qint64(total.QuadPart);v.available=qint64(available.QuadPart);
        v.readOnly=(flags&FILE_READ_ONLY_VOLUME)!=0;
        if(GetVolumeNameForVolumeMountPointW(root,guid,100)){
            v.identityKind="windows-volume-guid";v.key=v.identityKind+":"+QString::fromWCharArray(guid).toLower();v.strongIdentity=true;
        }else{v.identityKind="mount-hint";v.key="mount:"+v.mount+":"+v.fileSystem;}
        result<<v;
    }
#else
    for(auto storage:QStorageInfo::mountedVolumes()){
        if(!storage.isValid()||!storage.isReady()||storage.bytesTotal()<=0)continue;
        Volume v;v.mount=storage.rootPath();v.label=storage.name();v.fileSystem=QString::fromLatin1(storage.fileSystemType());
        v.total=storage.bytesTotal();v.available=storage.bytesAvailable();v.readOnly=storage.isReadOnly();
        const auto device=QString::fromLocal8Bit(storage.device());
#if defined(Q_OS_LINUX)
        const auto canonical=QFileInfo(device).canonicalFilePath();
        for(const auto &alias:QDir("/dev/disk/by-uuid").entryInfoList(QDir::Files|QDir::System)){
            if(!canonical.isEmpty()&&alias.canonicalFilePath()==canonical){v.identityKind="filesystem-uuid";v.key=v.identityKind+":"+alias.fileName();v.strongIdentity=true;break;}
        }
#elif defined(Q_OS_MACOS)
        QProcess p;p.setProgram("/usr/sbin/diskutil");p.setArguments({"info","-plist",device});p.start();
        if(p.waitForFinished(1200)){
            QXmlStreamReader xml(p.readAllStandardOutput());
            while(!xml.atEnd()){
                xml.readNext();
                if(xml.isStartElement()&&xml.name()==QLatin1String("key")&&xml.readElementText()=="VolumeUUID"){
                    if(xml.readNextStartElement()) { const auto id=xml.readElementText(); if(!id.isEmpty()){v.identityKind="macos-volume-uuid";v.key=v.identityKind+":"+id;v.strongIdentity=true;} }break;
                }
            }
        } else {p.kill();p.waitForFinished(200);}
#endif
        if(v.key.isEmpty()){v.identityKind="mount-hint";v.key="mount:"+v.mount+":"+v.fileSystem;}
        result<<v;
    }
#endif
    QHash<QString,int> counts;for(const auto &v:result)++counts[v.key];
    for(auto &v:result)if(counts[v.key]>1){v.strongIdentity=false;v.key+="@"+v.mount;v.identityKind="ambiguous";}
    return result;
}
QVariantList Volumes::items() const {
    if(!m_hasSnapshot)return {};
    QVariantList rows;QStringList online;
    for(const auto &v:m_volumes){
        online<<v.id;rows<<QVariantMap{{"id",v.id},{"label",v.label.isEmpty()?v.mount:v.label},{"mount",v.mount},
            {"available",v.available},{"total",v.total},{"online",true},{"strongIdentity",v.strongIdentity},{"readOnly",v.readOnly}};
    }
    for(auto value:m_store.knownVolumes()){
        auto v=value.toMap();if(online.contains(v["id"].toString()))continue;
        v["online"]=false;v["available"]=0;v["total"]=0;v["strongIdentity"]=false;rows<<v;
    }
    return rows;
}
}
