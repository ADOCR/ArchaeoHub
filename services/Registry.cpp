#include "Registry.h"
#include "modules/process/ProcessModule.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <algorithm>

namespace workbench {
Registry::Registry(Store &store,Volumes &volumes,const QString &directory,QObject *parent)
    :QAbstractListModel(parent),m_store(store),m_volumes(volumes),m_directory(QDir(directory).filePath("tool-manifests")){
    QDir().mkpath(m_directory);m_watcher.addPath(m_directory);
    connect(&m_watcher,&QFileSystemWatcher::directoryChanged,this,&Registry::reload);
    connect(&m_watcher,&QFileSystemWatcher::fileChanged,this,&Registry::reload);
    connect(&m_volumes,&Volumes::changed,this,[this]{if(rowCount())emit dataChanged(index(0),index(rowCount()-1));emit revised();});
    reload();
}
void Registry::reload(){
    QList<ToolDescriptor> tools;QStringList paths;
    for(const auto &name:QDir(":/modules").entryList(QDir::Dirs|QDir::NoDotAndDotDot))paths<<":/modules/"+name+"/tool.json";
    const auto local=QDir(m_directory).entryList({"*.json"},QDir::Files,QDir::Name);
    for(const auto &name:local.mid(0,128))paths<<QDir(m_directory).filePath(name);
    QStringList ids;
    for(const auto &path:paths){
        QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>262144)continue;
        auto parsed=parseManifest(file.readAll(),path);if(!parsed||ids.contains(parsed.tool.id))continue;
        ids<<parsed.tool.id;m_store.registerTool(parsed.tool);tools<<parsed.tool;
        if(!path.startsWith(':')&&!m_watcher.files().contains(path))m_watcher.addPath(path);
    }
    std::stable_sort(tools.begin(),tools.end(),[](const auto &a,const auto &b){return a.planned<b.planned;});
    beginResetModel();m_tools=tools;endResetModel();emit revised();
}
int Registry::rowCount(const QModelIndex &parent) const{return parent.isValid()?0:int(m_tools.size());}
QHash<int,QByteArray> Registry::roleNames() const{return {{IdRole,"toolId"},{NameRole,"name"},{DescriptionRole,"description"},{VersionRole,"version"},
    {RuntimeRole,"runtime"},{IconRole,"iconName"},{CategoryRole,"category"},{PlannedRole,"planned"},{FavoriteRole,"favorite"},{AvailabilityRole,"availability"},{LastUsedRole,"lastUsed"}};}
QVariant Registry::data(const QModelIndex &idx,int role) const{
    if(!idx.isValid()||idx.row()<0||idx.row()>=m_tools.size())return {};
    const auto &t=m_tools[idx.row()];
    switch(role){case IdRole:return t.id;case NameRole:return t.name;case DescriptionRole:return t.description;case VersionRole:return t.version;
    case RuntimeRole:return t.runtime;case IconRole:return t.icon;case CategoryRole:return t.categories.join(", ");case PlannedRole:return t.planned;
    case FavoriteRole:return m_store.isFavorite(t.id);case AvailabilityRole:return availability(t.id);case LastUsedRole:return m_store.lastUsed(t.id);default:return {};}
}
ToolDescriptor Registry::descriptor(const QString &id) const{for(const auto &t:m_tools)if(t.id==id)return t;return {};}
LaunchRequest Registry::launchRequest(const QString &id) const{
    const auto i=m_store.installation(id);
    return {Paths::resolve(LocationRef::fromJson(QJsonObject::fromVariantMap(i.value("program").toMap())),m_volumes.snapshot()),
            Paths::resolve(LocationRef::fromJson(QJsonObject::fromVariantMap(i.value("interpreter").toMap())),m_volumes.snapshot()),{}};
}
QString Registry::availability(const QString &id) const{
    const auto t=descriptor(id);if(t.id.isEmpty())return "unknown";if(t.planned)return "planned";
    if(!workspace(id).isEmpty()){
        const auto root=QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
        const QString suffix=".exe";
#else
        const QString suffix;
#endif
        if(workspace(id)=="catalog")return QFileInfo::exists(QDir(root).filePath("modules/catalog/engine/CatalogWorker"+suffix))?"ready":"program.not_found";
        if(workspace(id)=="transcription")return QFileInfo::exists(QDir(root).filePath("modules/transcription/engine/TranscriptionWorker"+suffix))?"ready":"program.not_found";
        if(workspace(id)=="inventory")return QFileInfo::exists(QDir(root).filePath("modules/inventory/engine/InventoryWorker"+suffix))?"ready":"program.not_found";
        if(workspace(id)=="documents")return QFileInfo::exists(QDir(root).filePath("modules/documents/engine/DocumentWorker"+suffix))?"ready":"program.not_found";
        return QFileInfo::exists(QDir(root).filePath("CoordinateWorker"+suffix))&&QFileInfo::exists(QDir(root).filePath("modules/coordinates/engine/engine.json"))?"ready":"program.not_found";
    }
    const auto i=m_store.installation(id);if(i.isEmpty())return "needs_configuration";
    if(i.value("hash").toString()!=t.manifestHash)return "needs_review";
    if(!m_volumes.hasSnapshot())return "checking_volumes";
    const auto request=launchRequest(id);
    if(request.program.isEmpty())return "volume_unavailable";
    const auto valid=ProcessModule::validate(t,request);
    return valid.ok?"ready":valid.code;
}
QString Registry::workspace(const QString &id) const{
    // Sólo los módulos compilados e incluidos pueden elegir una vista QML. Los manifiestos importados no ejecutan QML.
    const auto t=descriptor(id);if(!t.source.startsWith(":/modules/")||!t.capabilities.contains("workspace"))return {};
    if(id=="workbench.photo-catalog")return "catalog";
    if(id=="workbench.coordinates")return "coordinates";
    if(id=="workbench.transcription")return "transcription";
    if(id=="workbench.inventory")return "inventory";
    if(id=="workbench.document-summary")return "documents";
    return {};
}
QVariantMap Registry::details(const QString &id) const{
    const auto t=descriptor(id);if(t.id.isEmpty())return {};
    const auto r=launchRequest(id);const auto install=m_store.installation(id);
    return {{"id",t.id},{"name",t.name},{"description",t.description},{"version",t.version},{"runtime",t.runtime},{"icon",t.icon},
        {"categories",t.categories},{"planned",t.planned},{"workspace",workspace(id)},{"favorite",m_store.isFavorite(id)},{"availability",availability(id)},
        {"lastUsed",m_store.lastUsed(id)},{"program",r.program},{"interpreter",r.interpreter},{"manifest",t.source},
        {"previousPath",install.value("program").toMap().value("absoluteHint")}};
}
QString Registry::configure(const QString &id,const QString &program,const QString &interpreter){
    if(!m_volumes.hasSnapshot())return "checking_volumes";
    const auto t=descriptor(id);if(t.id.isEmpty())return "module.not_found";
    const auto check=ProcessModule::validate(t,{program,interpreter,{}});if(!check.ok)return check.code;
    m_store.configure(id,Paths::reference(program,m_volumes.snapshot()),Paths::reference(interpreter,m_volumes.snapshot()),t.manifestHash);
    if(rowCount())emit dataChanged(index(0),index(rowCount()-1));emit revised();return {};
}
QString Registry::importManifest(const QString &source,const QString &expectedHash){
    QFile file(source);if(!file.open(QIODevice::ReadOnly))return "manifest.unreadable";
    const auto bytes=file.read(262145);const auto parsed=parseManifest(bytes,source);if(!parsed)return parsed.error;
    if(parsed.tool.manifestHash!=expectedHash)return "manifest.changed_during_review";
    if(!descriptor(parsed.tool.id).id.isEmpty())return "manifest.duplicate_id";
    QSaveFile target(QDir(m_directory).filePath(parsed.tool.id+".json"));
    if(!target.open(QIODevice::WriteOnly)||target.write(bytes)!=bytes.size()||!target.commit())return "manifest.save_failed";
    reload();return {};
}
void Registry::toggleFavorite(const QString &id){if(descriptor(id).id.isEmpty())return;m_store.setFavorite(id,!m_store.isFavorite(id));
    if(rowCount())emit dataChanged(index(0),index(rowCount()-1));emit revised();}
void Registry::recordUse(const QString &id){m_store.recordUse(id);if(rowCount())emit dataChanged(index(0),index(rowCount()-1));emit revised();}
}
