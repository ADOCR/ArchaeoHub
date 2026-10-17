#include "DocumentLibraryController.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <QProcessEnvironment>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
namespace workbench::documents {
DocumentLibraryController::DocumentLibraryController(TaskManager &tasks,const QString &profile,QObject *parent,quint16 localPort,bool conversionOnly):QObject(parent),m_tasks(tasks),m_profile(profile),m_port(localPort),m_conversionOnly(conversionOnly){
    connect(&m_process,&QProcess::readyReadStandardOutput,this,&DocumentLibraryController::receive);
    connect(&m_process,&QProcess::readyReadStandardError,this,[this]{m_process.readAllStandardError();});
    connect(&m_process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)finish(false,tr("No se pudo iniciar el lector incluido. Revisa la instalación del Hub."));});
    connect(&m_process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
        receive();if(!m_busy)return;
        if(code!=0||status!=QProcess::NormalExit||!m_reply["ok"].toBool()){
            finish(false,m_cancelled?tr("Lote cancelado. Los originales se conservaron."):m_reply["error"].toString(tr("El lector terminó antes de completar el lote. Los originales se conservaron.")));return;
        }
        if(m_operation=="library.analyze"){
            m_rows=m_reply["rows"].toArray().toVariantList();m_topics=m_reply["topics"].toArray().toVariantList();m_ready=m_reply["ready"].toInt();m_failed=m_reply["failed"].toInt();m_planDigest=m_reply["planDigest"].toString();
            finish(true,m_conversionOnly?tr("%1 documentos listos para convertir · %2 pendientes. Elige el destino y convierte a Markdown.").arg(m_ready).arg(m_failed):tr("%1 documentos convertibles · %2 pendientes. Prepara las fichas con el modelo local.").arg(m_ready).arg(m_failed));
        }else if(m_operation=="library.enrich"){
            m_analysis=m_reply["analysis"].toObject();m_topics=m_reply["topics"].toArray().toVariantList();m_planPath=m_pendingPlan;m_planDigest=m_reply["planDigest"].toString();
            finish(true,tr("Fichas listas: %1 documentos y %2 conexiones explicadas. Abre cada fila para revisar las citas antes de exportar.").arg(m_analysis["documents"].toArray().size()).arg(connections().size()));
        }else{
            m_resultFolder=m_reply["folder"].toString();
            finish(true,m_conversionOnly?tr("Conversión terminada: %1 archivos Markdown, %2 pendientes. Los originales se conservaron.").arg(m_reply["documents"].toInt()).arg(m_reply["pending"].toInt()):tr("Biblioteca creada: %1 documentos, %2 temas, %3 pendientes. Los originales se conservaron.").arg(m_reply["documents"].toInt()).arg(m_reply["topics"].toInt()).arg(m_reply["pending"].toInt()));
        }
    });
    m_deadline.setSingleShot(true);connect(&m_deadline,&QTimer::timeout,this,[this]{
        if(m_operation=="library.export"){m_message=tr("La unidad tarda en responder. Puedes cancelar; se espera a publicar o retirar el lote completo.");emit changed();}
        else{m_message=tr("El análisis tardó demasiado. Selecciona un lote más pequeño.");m_process.kill();}
    });
#ifdef Q_OS_WIN
    m_process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
}
DocumentLibraryController::~DocumentLibraryController(){
    if(m_busy)cancel();m_process.disconnect(this);if(m_process.state()!=QProcess::NotRunning&&!m_process.waitForFinished(1500)){m_process.kill();m_process.waitForFinished(500);}if(!m_task.isEmpty())m_tasks.endExternal(m_task,"interrupted");
}
QVariantList DocumentLibraryController::inputs() const{QVariantList values;for(const auto &path:m_inputs)values<<QVariantMap{{"name",QFileInfo(path).fileName()},{"path",path},{"directory",QFileInfo(path).isDir()}};return values;}
void DocumentLibraryController::invalidate(){m_rows.clear();m_topics.clear();m_planDigest.clear();m_analysis={};m_planPath.clear();m_ready=0;m_failed=0;m_error=false;m_progress=0;}
void DocumentLibraryController::addInputs(const QVariantList &urls){if(m_busy)return;bool added=false;for(const auto &value:urls){const auto url=value.toUrl();if(!url.isLocalFile())continue;const QFileInfo info(url.toLocalFile());const auto path=info.canonicalFilePath();if(info.exists()&&!info.isSymLink()&&!path.isEmpty()&&!m_inputs.contains(path)&&m_inputs.size()<1000){m_inputs<<path;added=true;}}if(added)invalidate();emit changed();}
void DocumentLibraryController::removeInput(int index){if(!m_busy&&index>=0&&index<m_inputs.size()){m_inputs.removeAt(index);invalidate();emit changed();}}
void DocumentLibraryController::analyze(){
    if(m_busy||m_process.state()!=QProcess::NotRunning)return;if(m_inputs.isEmpty()){m_message=tr("Añade archivos o una carpeta para analizar.");m_error=true;emit changed();return;}
    invalidate();const auto cacheRoot=QDir(m_profile).filePath("cache/document-library");QDir().mkpath(cacheRoot);m_cache=std::make_unique<QTemporaryDir>(QDir(cacheRoot).filePath("batch-XXXXXX"));
    if(!m_cache->isValid()){m_message=tr("No se pudo crear la caché del lote. Revisa espacio y permisos.");m_error=true;emit changed();return;}
    QJsonArray paths;for(const auto &path:m_inputs)paths<<path;
    m_planPath=QDir(m_cache->path()).filePath("plan.json");QJsonObject request{{"paths",paths},{"planFile",m_planPath}};
    if(m_conversionOnly){request["formats"]=QJsonArray{".pdf",".docx"};request["topicCandidates"]=false;request["conversionOnly"]=true;}
    start("library.analyze",request);
}
void DocumentLibraryController::enrich(const QString &model){
    if(m_conversionOnly||m_busy||!hasPlan()||!m_cache||m_process.state()!=QProcess::NotRunning)return;
    if(model.trimmed().isEmpty()){m_error=true;m_message=tr("Selecciona un modelo local instalado para preparar las fichas.");emit changed();return;}
    const auto cancelPath=QDir(m_cache->path()).filePath("cancel");QFile::remove(cancelPath);
    m_pendingPlan=QDir(m_cache->path()).filePath("research-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".json");
    start("library.enrich",{{"planFile",m_planPath},{"planDigest",m_planDigest},{"analysisFile",m_pendingPlan},{"model",model},{"ollamaPort",m_port},{"cancelFile",cancelPath}});
}
void DocumentLibraryController::exportLibrary(const QUrl &parent,bool automaticLinks){
    if(m_busy||!hasPlan()||!m_cache||m_process.state()!=QProcess::NotRunning)return;
    if(!parent.isLocalFile()||!QFileInfo(parent.toLocalFile()).isDir()){m_error=true;m_message=tr("Elige una carpeta de destino disponible.");emit changed();return;}
    const auto cancelPath=QDir(m_cache->path()).filePath("cancel");QFile::remove(cancelPath);
    start("library.export",{{"planFile",m_planPath},{"planDigest",m_planDigest},{"destination",parent.toLocalFile()},{"automaticLinks",automaticLinks&&!m_conversionOnly},{"cancelFile",cancelPath}});
}
void DocumentLibraryController::start(const QString &op,QJsonObject request){
    m_operation=op;m_busy=true;m_error=false;m_cancelled=false;m_buffer.clear();m_reply={};m_progress=0;m_message=op=="library.analyze"?tr("Revisando documentos locales…"):op=="library.enrich"?tr("Preparando fichas de lectura y evidencias con el modelo local…"):tr("Preparando una biblioteca nueva sin reemplazar tus notas…");
    m_task=m_tasks.beginExternal(op=="library.analyze"?tr("Analizar biblioteca documental"):op=="library.enrich"?tr("Redactar fichas y contrastar conceptos"):tr("Exportar biblioteca Markdown"),op,[this]{cancel();});
    auto program=QDir(QCoreApplication::applicationDirPath()).filePath("modules/documents/engine/DocumentWorker");
#ifdef Q_OS_WIN
    program+=".exe";
#endif
    auto environment=QProcessEnvironment::systemEnvironment();environment.remove("PYTHONHOME");environment.remove("PYTHONPATH");environment.insert("PYTHONNOUSERSITE","1");
    m_process.setProcessEnvironment(environment);m_process.setProgram(program);m_process.setArguments({});m_process.start();request["schemaVersion"]=1;request["op"]=op;
    m_process.write(QJsonDocument(request).toJson(QJsonDocument::Compact));m_process.closeWriteChannel();m_deadline.start(op=="library.enrich"?600000:240000);emit changed();
}
void DocumentLibraryController::receive(){
    if(!m_busy){m_process.readAllStandardOutput();return;}
    m_buffer+=m_process.readAllStandardOutput();if(m_buffer.size()>16*1024*1024){m_process.kill();finish(false,tr("El lote supera el tamaño de respuesta admitido. Divide la selección."));return;}
    while(m_buffer.contains('\n')){const auto end=m_buffer.indexOf('\n');const auto line=m_buffer.left(end);m_buffer.remove(0,end+1);QJsonParseError error;const auto value=QJsonDocument::fromJson(line,&error).object();
        if(error.error!=QJsonParseError::NoError){m_process.kill();finish(false,tr("El lector devolvió una respuesta no compatible."));return;}
        if(value["type"]=="progress"){m_progress=qBound(0.0,value["progress"].toDouble(),1.0);m_message=value["message"].toString();if(m_operation=="library.enrich")m_deadline.start(600000);m_tasks.externalProgress(m_task,m_progress);emit changed();}
        else m_reply=value;
    }
}
void DocumentLibraryController::finish(bool ok,const QString &message){if(!m_busy)return;m_deadline.stop();m_busy=false;m_error=!ok;m_message=message;if(ok)m_progress=1;const auto task=m_task;m_task.clear();if(!task.isEmpty())m_tasks.endExternal(task,ok?"succeeded":m_cancelled?"cancelled":"failed",m_operation=="library.export"&&ok?m_resultFolder:QString(),ok?QString():message);emit changed();emit completed(ok);}
void DocumentLibraryController::cancel(){if(!m_busy)return;m_cancelled=true;if((m_operation=="library.export"||m_operation=="library.enrich")&&m_cache){QFile flag(QDir(m_cache->path()).filePath("cancel"));if(flag.open(QIODevice::WriteOnly)){flag.write("cancel");flag.close();m_message=tr("Cancelando sin publicar un lote incompleto…");if(m_operation=="library.enrich"){const auto task=m_task;QTimer::singleShot(2000,this,[this,task]{if(m_busy&&m_task==task)m_process.kill();});}}else m_message=tr("No se pudo solicitar la cancelación. Espera a que termine para conservar el lote.");emit changed();}else m_process.kill();}

QString DocumentLibraryController::preview(const QString &kind,int index) const{
    if(!hasAnalysis())return tr("Prepara las fichas con el modelo local para revisar resúmenes y citas.");
    auto evidence=[this](const QJsonArray &values){QString text;for(const auto &entry:values){const auto e=entry.toObject();QString name;for(const auto &row:m_rows){const auto r=row.toMap();if(r["id"].toString()==e["document"].toString())name=r["name"].toString();}text+="\n    ["+name+" · "+e["locator"].toString()+"] «"+e["quote"].toString()+"»\n";}return text;};
    if(kind=="connections"){
        const auto list=m_analysis["relations"].toArray();if(index<0||index>=list.size())return {};
        const auto relation=list[index].toObject();return tr("Conexión propuesta · pendiente de revisión\n\n%1\n\nEvidencias de las fuentes:\n%2").arg(relation["explanation"].toString(),evidence(relation["evidence"].toArray()));
    }
    if(kind=="concepts"){
        if(index<0||index>=m_topics.size())return {};const auto topic=m_topics[index].toMap();QString text=topic["name"].toString()+"\n\n"+topic["definition"].toString()+"\n\n";
        for(const auto &document:m_analysis["documents"].toArray())for(const auto &entry:document.toObject()["concepts"].toArray()){const auto c=entry.toObject();if(c["id"].toString()==topic["id"].toString())text+=evidence(c["evidence"].toArray());}
        return text;
    }
    if(index<0||index>=m_rows.size())return {};const auto id=m_rows[index].toMap()["id"].toString();
    for(const auto &entry:m_analysis["documents"].toArray()){
        const auto doc=entry.toObject();if(doc["id"].toString()!=id)continue;
        QString text=m_rows[index].toMap()["name"].toString()+"\n\n"+tr("Síntesis propuesta; las citas se cotejaron, pero la interpretación necesita revisión.")+"\n\n";
        for(const auto &claim:doc["summary"].toArray()){const auto c=claim.toObject();text+=c["text"].toString()+"\n"+evidence(c["evidence"].toArray())+"\n";}
        if(doc["summary"].toArray().isEmpty())text+=tr("No se obtuvieron afirmaciones con evidencia suficiente. Revisa el original o prueba otro modelo.");
        return text;
    }
    return tr("Este archivo está pendiente de extracción; no se generó una ficha.");
}
}
