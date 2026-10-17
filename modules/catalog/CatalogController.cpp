#include "CatalogController.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcessEnvironment>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace workbench::catalog {
CatalogController::CatalogController(TaskManager &tasks,const QString &directory,QObject *parent)
    :QObject(parent),m_tasks(tasks),m_preferences(QDir(directory).filePath("catalog-preferences.ini"),QSettings::IniFormat),m_cache(QDir(directory).filePath("cache/catalog")){
    connect(&m_process,&QProcess::readyReadStandardOutput,this,&CatalogController::receive);
    connect(&m_process,&QProcess::readyReadStandardError,this,[this]{m_process.readAllStandardError();});
    connect(&m_process,&QProcess::finished,this,[this](int,QProcess::ExitStatus){
        receive();if(m_busy)fail(tr("El motor del catálogo se cerró antes de responder. Reabre el catálogo para comprobar sus transacciones pendientes."));
        m_state={};m_selection={};m_items.clear();emit changed();
    });
    connect(&m_process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){
        if(error==QProcess::FailedToStart)fail(tr("No se pudo iniciar el motor del catálogo. Revisa la instalación del Hub."));
    });
    m_deadline.setSingleShot(true);connect(&m_deadline,&QTimer::timeout,this,[this]{
        // No interrumpas una escritura en curso sólo porque el disco sea lento.
        m_message=tr("La unidad tarda en responder. Se espera a que termine la operación para conservar la recuperación de cambios.");emit changed();
    });
#ifdef Q_OS_WIN
    m_process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
}
CatalogController::~CatalogController(){
    m_process.disconnect(this);m_process.closeWriteChannel();
    if(m_process.state()!=QProcess::NotRunning&&!m_process.waitForFinished(700)){m_process.kill();m_process.waitForFinished(500);}
    if(!m_task.isEmpty())m_tasks.endExternal(m_task,"interrupted",{},"catalog.session_interrupted");
}
QVariantMap CatalogController::remembered() const{
    return {{"source",m_preferences.value("source")},{"output",m_preferences.value("output")},{"site",m_preferences.value("site")},{"operation",m_preferences.value("operation")}};
}
void CatalogController::fail(const QString &message){
    m_deadline.stop();m_busy=false;m_error=true;m_migrationRequired=false;m_message=message;
    if(!m_task.isEmpty()){m_tasks.endExternal(m_task,"failed",{},message);m_task.clear();}
    emit changed();emit completed();
}
bool CatalogController::request(QJsonObject object){
    if(m_busy)return false;
    const auto root=QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    const auto program=QDir(root).filePath("modules/catalog/engine/CatalogWorker.exe");
#else
    const auto program=QDir(root).filePath("modules/catalog/engine/CatalogWorker");
#endif
    if(!QFileInfo::exists(program)){fail(tr("Falta el motor local del catálogo. Revisa la instalación del Hub."));return false;}
    object["schemaVersion"]=1;object["requestId"]=++m_requestId;m_pending=object;m_operation=object["op"].toString();
    m_busy=true;m_error=false;m_message=tr("Procesando catálogo…");m_buffer.clear();
    m_task=m_tasks.beginExternal(tr("Catálogo · %1").arg(m_operation),"catalog."+m_operation,[this]{
        m_message=tr("Se completará la operación actual antes de cerrar, para proteger las fotografías y el Excel.");emit changed();
    });
    if(m_process.state()==QProcess::NotRunning){
        m_process.setProgram(program);m_process.setArguments({});m_process.setWorkingDirectory(QFileInfo(program).absolutePath());
        auto environment=QProcessEnvironment::systemEnvironment();environment.insert("PYTHONNOUSERSITE","1");environment.remove("PYTHONPATH");environment.remove("PYTHONHOME");
        m_process.setProcessEnvironment(environment);m_process.start();
    }
    m_process.write(QJsonDocument(object).toJson(QJsonDocument::Compact)+'\n');m_deadline.start(120000);emit changed();return true;
}
void CatalogController::receive(){
    m_buffer+=m_process.readAllStandardOutput();
    if(m_buffer.size()>8*1024*1024){fail(tr("La respuesta del catálogo supera el límite permitido."));m_process.kill();return;}
    while(m_buffer.contains('\n')){
        const auto split=m_buffer.indexOf('\n');const auto bytes=m_buffer.left(split);m_buffer.remove(0,split+1);
        QJsonParseError parse;const auto document=QJsonDocument::fromJson(bytes,&parse);const auto reply=document.object();
        if(parse.error!=QJsonParseError::NoError||reply["schemaVersion"].toInt()!=1||reply["requestId"].toInt()!=m_requestId){fail(tr("El motor respondió con un mensaje no compatible."));return;}
        if(!reply["ok"].toBool()){fail(reply["error"].toString(tr("No se pudo completar la operación del catálogo.")));return;}
        m_deadline.stop();m_busy=false;m_error=false;const auto result=reply["result"].toObject();
        m_migrationRequired=result["migrationRequired"].toBool();
        m_awaitingInput=result["needsInput"].toBool();
        if(result.contains("project"))m_project=result["project"].toObject();
        const bool sessionOpened=(m_operation=="open"||m_operation=="load")&&result.contains("state");
        if(sessionOpened){m_selection={};m_proposed.clear();m_draft.clear();m_filter="Todas";m_query.clear();}
        if(result.contains("state"))m_state=result["state"].toObject();
        if(result.contains("filter"))m_filter=result["filter"].toString();
        if(result.contains("items")){m_items=result["items"].toArray().toVariantList();m_matched=result["matched"].toInt();m_offset=result["offset"].toInt();}
        if(result.contains("selection")){
            auto selection=result["selection"].toObject();
            // Los cambios de metadatos conservan la vista previa ya decodificada.
            if(selection["id"]==m_selection["id"]&&!selection.contains("preview")){selection["preview"]=m_selection["preview"];selection["gps"]=m_selection["gps"];selection["gpsStatus"]=m_selection["gpsStatus"];}
            m_selection=selection;
        }
        if(sessionOpened){
            for(const auto &key:{"source","output","site"})m_preferences.setValue(key,m_state[key].toVariant());
            m_preferences.setValue("operation",m_state["initialOperation"].toVariant());
        }
        if(m_operation=="close"){m_state={};m_items.clear();m_selection={};}
        if(m_operation=="discard"&&!result.contains("selection"))m_selection={};
        if(result.contains("selection"))m_proposed.clear();
        if(result.contains("proposedName"))m_proposed=result["proposedName"].toString();
        m_message=result["warning"].toString(result["message"].toString());
        if(m_message.isEmpty())m_message=m_operation=="save"||m_operation=="correct"?tr("Fotografía y Excel guardados."):m_operation=="manualGps"?tr("GPS guardado en la copia. El original y el Excel se conservaron."):m_operation=="exportGps"?tr("Reporte GPS creado fuera de la entrega del Museo."):tr("Catálogo listo.");
        if(!m_task.isEmpty()){m_tasks.endExternal(m_task,"succeeded",m_operation=="exportGps"?result["report"].toObject()["path"].toString():QString());m_task.clear();}
        emit changed();if(sessionOpened)emit sessionReset();if(result.contains("project"))emit projectDiscovered();if(result.contains("selection")&&m_operation!="save"&&m_operation!="correct")emit selected();if(m_operation=="save"||m_operation=="correct")emit saved();emit completed();
    }
}
void CatalogController::openCatalog(const QString &source,const QString &output,const QString &site,const QString &operation,bool confirmMigration){request({{"op","open"},{"source",source},{"output",output},{"site",site},{"operation",operation},{"confirmMigration",confirmMigration},{"cache",m_cache}});}
void CatalogController::loadCatalog(const QUrl &location,const QString &source,const QString &site,const QString &operation,bool confirmMigration){if(location.isLocalFile())request({{"op","load"},{"location",location.toLocalFile()},{"source",source},{"site",site},{"operation",operation},{"confirmMigration",confirmMigration},{"cache",m_cache}});}
void CatalogController::createCatalog(const QString &source,const QString &output,const QString &site,const QString &operation){request({{"op","open"},{"create",true},{"source",source},{"output",output},{"site",site},{"operation",operation},{"cache",m_cache}});}
void CatalogController::page(const QString &filter,const QString &query,int offset){if(m_busy)return;m_filter=filter;m_query=query;request({{"op","page"},{"filter",filter},{"query",query},{"offset",offset}});}
void CatalogController::select(const QString &id){request({{"op","select"},{"id",id}});}
void CatalogController::navigate(int delta){act("navigate",{{"delta",delta}});}
void CatalogController::act(const QString &op,QJsonObject extra){extra["op"]=op;extra["id"]=m_selection["id"];extra["filter"]=m_filter;extra["query"]=m_query;extra["offset"]=m_offset;request(extra);}
void CatalogController::propose(const QVariantMap &form){act("propose",{{"form",QJsonObject::fromVariantMap(form)}});}
void CatalogController::save(const QVariantMap &form,const QVariantList &gps){act("save",{{"form",QJsonObject::fromVariantMap(form)},{"manualGps",QJsonArray::fromVariantList(gps)}});}
void CatalogController::saveAndNext(const QVariantMap &form,const QVariantList &gps){act("save",{{"advance",true},{"form",QJsonObject::fromVariantMap(form)},{"manualGps",QJsonArray::fromVariantList(gps)}});}
void CatalogController::nextGps(){act("nextGps");}
void CatalogController::correct(const QVariantMap &form){act("correct",{{"form",QJsonObject::fromVariantMap(form)}});}
void CatalogController::skip(){const bool skip=!m_selection["skipped"].toBool();act("skip",{{"skipped",skip},{"advance",skip}});}
void CatalogController::withdraw(bool confirmed){act("withdraw",{{"confirmed",confirmed}});}
void CatalogController::discard(bool confirmed){act("discard",{{"confirmed",confirmed}});}
void CatalogController::manualGps(const QVariantList &values){act("manualGps",{{"manualGps",QJsonArray::fromVariantList(values)}});}
void CatalogController::exportGps(const QUrl &file){if(file.isLocalFile())request({{"op","exportGps"},{"destination",file.toLocalFile()}});}
void CatalogController::closeCatalog(){request({{"op","close"}});}
}
