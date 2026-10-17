#include "InventoryController.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QProcessEnvironment>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
namespace workbench::inventory {
InventoryController::InventoryController(TaskManager &tasks,const QString &profile,QObject *parent):QObject(parent),m_tasks(tasks),m_profile(profile){
    connect(&m_process,&QProcess::readyReadStandardOutput,this,&InventoryController::receive);
    connect(&m_process,&QProcess::readyReadStandardError,this,[this]{m_process.readAllStandardError();});
    connect(&m_process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)finish(false,tr("No se pudo iniciar el motor de inventario incluido. Revisa la instalación."));});
    connect(&m_process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
        receive();if(!m_busy)return;
        if(code!=0||status!=QProcess::NormalExit||!m_reply["ok"].toBool()){finish(false,m_reply["error"].toString(m_cancelled?tr("Operación cancelada."):tr("El motor de inventario terminó sin completar la operación.")));return;}
        m_result=m_reply["result"].toObject();
        if(m_operation=="preview")m_preview=m_result.toVariantMap();
        else if(m_result.contains("database")){m_database=m_result["database"].toString();m_state=m_result.toVariantMap();m_preview.clear();}
        finish(true,m_operation=="preview"?tr("Revisa cantidades, pendientes y colisiones antes de importar."):m_operation=="export"?tr("Tabla exportada en un archivo nuevo. Se conservaron los originales."):tr("Inventario actualizado. Los registros pendientes se excluyen del análisis."));
    });
    m_timer.setSingleShot(true);connect(&m_timer,&QTimer::timeout,this,[this]{cancel();m_message=tr("La unidad tarda en responder. Se solicitó cancelar; espera a que se cierre la operación.");emit changed();});
#ifdef Q_OS_WIN
    m_process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
}
InventoryController::~InventoryController(){if(m_busy)cancel();m_process.disconnect(this);if(m_process.state()!=QProcess::NotRunning&&!m_process.waitForFinished(1500)){m_process.kill();m_process.waitForFinished(500);}if(!m_task.isEmpty())m_tasks.endExternal(m_task,"interrupted");}
void InventoryController::create(const QUrl &file,const QVariantMap &metadata){if(file.isLocalFile())request({{"op","create"},{"database",file.toLocalFile()},{"metadata",QJsonObject::fromVariantMap(metadata)}});}
void InventoryController::open(const QUrl &file){if(file.isLocalFile())request({{"op","open"},{"database",file.toLocalFile()}});}
void InventoryController::reviewImport(const QUrl &file){if(file.isLocalFile()&&opened())request({{"op","preview"},{"source",file.toLocalFile()}});}
void InventoryController::importReviewed(){if(!m_preview.isEmpty()&&!m_preview.value("alreadyImported").toBool()&&m_preview.value("collisions").toList().isEmpty())request({{"op","import"},{"source",m_preview.value("source").toString()},{"sourceDigest",m_preview.value("sourceDigest").toString()}});}
void InventoryController::add(const QVariantMap &record){if(opened())request({{"op","add"},{"record",QJsonObject::fromVariantMap(record)}});}
void InventoryController::search(const QString &query,bool pendingOnly,int page){if(opened())request({{"op","list"},{"query",query},{"pendingOnly",pendingOnly},{"page",page}});}
void InventoryController::exportFile(const QUrl &file){if(file.isLocalFile()&&opened())request({{"op","export"},{"output",file.toLocalFile()}});}
bool InventoryController::request(QJsonObject value){
    if(m_busy||m_process.state()!=QProcess::NotRunning)return false;
    if(!value.contains("database"))value["database"]=m_database;
    if(value["database"].toString().isEmpty())return false;
    const auto root=QDir(m_profile).filePath("cache/inventory");QDir().mkpath(root);m_cache=std::make_unique<QTemporaryDir>(QDir(root).filePath("task-XXXXXX"));
    if(!m_cache->isValid()){m_error=true;m_message=tr("No se pudo crear la caché. Revisa espacio y permisos.");emit changed();return false;}
    m_operation=value["op"].toString();m_request=value;m_result={};m_reply={};m_buffer.clear();m_cancelled=false;m_busy=true;m_error=false;
    value["cancelFile"]=QDir(m_cache->path()).filePath("cancel");
    m_message=tr("Procesando inventario local…");m_task=m_tasks.beginExternal(tr("Inventario de materiales"),"inventory."+m_operation,[this]{cancel();});
    auto program=QDir(QCoreApplication::applicationDirPath()).filePath("modules/inventory/engine/InventoryWorker");
#ifdef Q_OS_WIN
    program+=".exe";
#endif
    auto env=QProcessEnvironment::systemEnvironment();env.remove("PYTHONHOME");env.remove("PYTHONPATH");env.insert("PYTHONNOUSERSITE","1");
    m_process.setProcessEnvironment(env);m_process.setProgram(program);m_process.setArguments({});m_process.start();m_process.write(QJsonDocument(value).toJson(QJsonDocument::Compact));m_process.closeWriteChannel();m_timer.start(180000);emit changed();return true;
}
void InventoryController::receive(){
    if(!m_busy){m_process.readAllStandardOutput();return;}m_buffer+=m_process.readAllStandardOutput();
    if(m_buffer.size()>32*1024*1024){m_process.kill();finish(false,tr("La respuesta supera el límite. Divide la fuente en lotes más pequeños."));return;}
    while(m_buffer.contains('\n')){const auto end=m_buffer.indexOf('\n');const auto line=m_buffer.left(end);m_buffer.remove(0,end+1);QJsonParseError error;const auto value=QJsonDocument::fromJson(line,&error).object();
        if(error.error!=QJsonParseError::NoError){m_process.kill();finish(false,tr("El motor devolvió una respuesta no compatible."));return;}
        if(value["type"]=="progress"){m_message=value["message"].toString();m_tasks.externalProgress(m_task,value["progress"].toDouble());emit changed();}else m_reply=value;
    }
}
void InventoryController::finish(bool ok,const QString &message){if(!m_busy)return;m_timer.stop();m_busy=false;m_error=!ok;m_message=message;const auto task=m_task;m_task.clear();if(!task.isEmpty())m_tasks.endExternal(task,ok?"succeeded":m_cancelled?"cancelled":"failed",ok?m_result["output"].toString():QString(),ok?QString():message);emit changed();emit completed(ok);}
void InventoryController::cancel(){if(!m_busy||!m_cache)return;m_cancelled=true;QFile file(QDir(m_cache->path()).filePath("cancel"));if(file.open(QIODevice::WriteOnly)){file.write("cancel");m_message=tr("Cancelando sin aplicar cambios incompletos…");}else m_message=tr("No se pudo solicitar la cancelación. Espera a que termine la operación.");emit changed();}
}
