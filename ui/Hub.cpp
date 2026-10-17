#include "Hub.h"
#include "modules/process/ProcessModule.h"
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>
#include <QClipboard>
#include <QtConcurrentRun>

namespace workbench {
void Hub::openLicenses(){openFile(QUrl::fromLocalFile(QDir(QCoreApplication::applicationDirPath()).filePath("THIRD_PARTY.md")));}
Hub::Hub(Store &store,Registry &registry,Volumes &volumes,TaskManager &tasks,Logger &logger,const QString &name,const QString &version,QObject *parent)
    :QObject(parent),m_store(store),m_registry(registry),m_volumes(volumes),m_tasks(tasks),m_logger(logger),m_name(name),m_version(version){
    connect(&m_registry,&Registry::revised,this,[this]{++m_revision;emit changed();});
    QFile definition(":/resources/project-templates/archaeology.json");definition.open(QIODevice::ReadOnly);
    m_projectTemplate=ProjectTemplates::parse(definition.readAll());
    connect(&m_projectCreation,&QFutureWatcher<ProjectCreation>::finished,this,[this]{
        m_creatingProject=false;
        const auto result=m_projectCreation.result();
        if(!result.ok()){message(statusText(result.error),true);return;}
        try{
            m_store.addProject(QFileInfo(result.path).fileName(),Paths::reference(result.path,m_volumes.snapshot()));
            ++m_revision;m_logger.write("INFO","project.created",{{"folders",m_projectTemplate.folders.size()}});
            message(tr("Proyecto creado con %1 carpetas vacías.").arg(m_projectTemplate.folders.size()));navigate("projects");
        }catch(const std::exception &){message(tr("La carpeta se creó, pero no se pudo registrar. Agrégala desde Proyectos: %1").arg(result.path),true);}
        emit changed();
    });
}
void Hub::message(const QString &text,bool error){m_notice=text;m_noticeError=error;emit changed();}
QVariantList Hub::commands() const {
    QVariantList rows{{QVariantMap{{"name",tr("Ir a Inicio")},{"route","home"},{"icon","home"}}},
        {QVariantMap{{"name",tr("Ver actividad")},{"route","activity"},{"icon","activity"}}},
        {QVariantMap{{"name",tr("Configuración")},{"route","settings"},{"icon","settings"}}},
        {QVariantMap{{"name",tr("Proyectos")},{"route","projects"},{"icon","folder"}}}};
    for(int i=0;i<m_registry.rowCount();++i){const auto t=m_registry.descriptor(m_registry.data(m_registry.index(i),Registry::IdRole).toString());
        rows<<QVariantMap{{"name",t.name},{"route","tool/"+t.id},{"icon",t.icon}};}
    return rows;
}
void Hub::navigate(const QString &value){
    if(!QStringList{"home","tools","projects","recent","favorites","activity","settings","about"}.contains(value)&&
        !(value.startsWith("tool/")&&!m_registry.descriptor(value.mid(5)).id.isEmpty()))return;
    if(value==route())return;
    while(m_history.size()>m_historyIndex+1)m_history.removeLast();m_history<<value;++m_historyIndex;emit changed();
}
void Hub::goBack(){if(canGoBack()){--m_historyIndex;emit changed();}}
QVariantMap Hub::openTool(const QString &id){
    const auto state=m_registry.availability(id);
    if(state!="ready"){message(statusText(state),true);navigate("tool/"+id);return {{"ok",false},{"error",state}};}
    if(!m_registry.workspace(id).isEmpty()){m_registry.recordUse(id);navigate("tool/"+id);return {{"ok",true},{"workspace",m_registry.workspace(id)}};}
    ProcessModule module(m_registry.descriptor(id));
    module.initialize({[this](const QString &tool,const QString &code){m_logger.write("INFO",code,{{"toolId",tool}});}});
    const auto result=module.open(m_registry.launchRequest(id));module.shutdown();
    m_tasks.recordLaunch(module.metadata().name,result.ok,result.code);
    if(result.ok){m_registry.recordUse(id);message(tr("Se abrió %1 en su propia ventana.").arg(module.metadata().name));}
    else{m_logger.write("ERROR",result.code);message(statusText(result.code),true);}
    return {{"ok",result.ok},{"processId",result.processId},{"error",result.code}};
}
bool Hub::configureTool(const QString &id,const QUrl &program,const QUrl &interpreter){
    if(!program.isLocalFile()){message(tr("Selecciona un archivo de este equipo."),true);return false;}
    const auto error=m_registry.configure(id,program.toLocalFile(),interpreter.isLocalFile()?interpreter.toLocalFile():QString());
    if(!error.isEmpty()){message(statusText(error),true);return false;}
    m_logger.write("INFO","installation.configured",{{"toolId",id}});message(tr("Instalación guardada. Ya puedes abrir la herramienta."));return true;
}
void Hub::addProject(const QUrl &folder){
    if(!m_volumes.hasSnapshot()){message(statusText("checking_volumes"));return;}
    if(!folder.isLocalFile()||!QFileInfo(folder.toLocalFile()).isDir()){message(tr("La carpeta del proyecto no está disponible."),true);return;}
    const QFileInfo info(folder.toLocalFile());m_store.addProject(info.fileName().isEmpty()?info.absoluteFilePath():info.fileName(),Paths::reference(info.absoluteFilePath(),m_volumes.snapshot()));
    ++m_revision;message(tr("Proyecto vinculado. Sus archivos permanecen en su carpeta."));
}
bool Hub::createProject(const QUrl &parent,const QString &name){
    if(creatingProject())return false;
    if(!m_volumes.hasSnapshot()){message(statusText("checking_volumes"));return false;}
    if(!parent.isLocalFile()){message(statusText("project.parent_unavailable"),true);return false;}
    if(!ProjectTemplates::validName(name)){message(statusText("project.invalid_name"),true);return false;}
    m_creatingProject=true;
    m_projectCreation.setFuture(QtConcurrent::run(&ProjectTemplates::create,m_projectTemplate,parent.toLocalFile(),name));
    emit changed();return true;
}
void Hub::openLocation(const QVariantMap &location){
    const auto path=Paths::resolve(LocationRef::fromJson(QJsonObject::fromVariantMap(location)),m_volumes.snapshot());
    if(path.isEmpty()||!QFileInfo::exists(path)){message(tr("La unidad o el archivo no están disponibles. La referencia se conserva."),true);return;}
    if(!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))message(tr("No se pudo abrir el archivo con su aplicación asociada."),true);
}
void Hub::openProject(int index){const auto rows=projects();if(index>=0&&index<rows.size())openLocation(rows[index].toMap()["location"].toMap());}
void Hub::openFile(const QUrl &file){
    if(!file.isLocalFile()||!QFileInfo(file.toLocalFile()).isFile()){message(tr("El archivo seleccionado no está disponible."),true);return;}
    const auto ref=Paths::reference(file.toLocalFile(),m_volumes.snapshot());
    m_store.recordFile(QFileInfo(file.toLocalFile()).fileName(),ref);++m_revision;emit changed();openLocation(ref.toJson().toVariantMap());
}
void Hub::openRecentFile(int index){const auto rows=recentFiles();if(index>=0&&index<rows.size())openLocation(rows[index].toMap()["location"].toMap());}
void Hub::openFolder(const QUrl &folder){
    if(!folder.isLocalFile()||!QFileInfo(folder.toLocalFile()).isDir()){message(tr("La carpeta no está disponible. Revisa su unidad y permisos."),true);return;}
    if(!QDesktopServices::openUrl(folder))message(tr("No se pudo abrir la carpeta con el explorador del sistema."),true);
}
void Hub::hashFile(const QUrl &file){
    if(!file.isLocalFile()||!QFileInfo(file.toLocalFile()).isFile()){message(tr("Selecciona un archivo disponible para verificar."),true);return;}
    if(m_tasks.hashFile(file.toLocalFile()).isEmpty())message(tr("Ya hay dos verificaciones en curso. Espera o cancela una."),true);
    else navigate("activity");
}
void Hub::reviewManifest(const QUrl &url){
    if(!url.isLocalFile())return;QFile file(url.toLocalFile());
    if(!file.open(QIODevice::ReadOnly)){message(tr("No se pudo leer el manifiesto."),true);return;}
    const auto parsed=parseManifest(file.read(262145),url.toLocalFile());
    if(!parsed){message(tr("El manifiesto no es compatible: %1").arg(parsed.error),true);return;}
    if(!m_registry.descriptor(parsed.tool.id).id.isEmpty()){message(tr("Esta herramienta ya está registrada. Revisa su instalación en Herramientas."),true);return;}
    m_pending={{"name",parsed.tool.name},{"description",parsed.tool.description},{"runtime",parsed.tool.runtime},
               {"source",url.toLocalFile()},{"hash",parsed.tool.manifestHash},{"id",parsed.tool.id}};emit changed();
}
void Hub::approveManifest(){
    if(m_pending.isEmpty())return;const auto id=m_pending["id"].toString();
    const auto error=m_registry.importManifest(m_pending["source"].toString(),m_pending["hash"].toString());
    m_pending.clear();
    if(!error.isEmpty())message(tr("No se pudo registrar el manifiesto: %1").arg(error),true);
    else{message(tr("Herramienta registrada. Selecciona y revisa su ejecutable antes de abrirla."));navigate("tool/"+id);}emit changed();
}
QString Hub::statusText(const QString &code) const{
    if(code=="project.invalid_name")return tr("Escribe un nombre de carpeta válido, sin barras ni caracteres reservados.");
    if(code=="project.parent_unavailable")return tr("Selecciona una carpeta de destino disponible.");
    if(code=="project.already_exists")return tr("Ya existe una carpeta con ese nombre. Elige otro nombre; no se modificó la existente.");
    if(code=="project.create_failed"||code=="project.finalize_failed")return tr("No se pudo crear el proyecto. Revisa permisos y disponibilidad de la unidad.");
    if(code=="project.invalid_template")return tr("La plantilla de carpetas no es válida. Revisa la instalación del Hub.");
    if(code=="ready")return tr("Disponible");if(code=="planned")return tr("Planificado");
    if(code=="checking_volumes")return tr("Comprobando las unidades del equipo…");
    if(code=="needs_configuration")return tr("Selecciona la instalación de esta herramienta.");
    if(code=="needs_review")return tr("El manifiesto cambió. Revisa y guarda de nuevo la instalación.");
    if(code=="volume_unavailable")return tr("Unidad no disponible. Se conserva la ubicación anterior.");
    if(code=="program.not_found")return tr("No se encuentra el ejecutable o script. Revisa su ubicación.");
    if(code=="interpreter.not_found")return tr("Falta un intérprete ejecutable. Selecciona el entorno de esta herramienta.");
    if(code=="program.not_executable")return tr("El archivo seleccionado no tiene permiso de ejecución.");
    if(code=="program.requires_interpreter")return tr("Un script requiere un intérprete explícito; selecciona un ejecutable para este módulo.");
    if(code=="runtime.not_supported")return tr("Este runtime requiere un adaptador que aún no está disponible.");
    if(code=="process.start_failed")return tr("No se pudo iniciar la aplicación. Revisa sus dependencias y permisos.");
    return tr("Esta herramienta todavía no está disponible.");
}
QString Hub::taskStateText(const QString &state) const{
    if(state=="queued")return tr("En cola");if(state=="running")return tr("Procesando");if(state=="cancelling")return tr("Cancelando");
    if(state=="succeeded")return tr("Completado");if(state=="cancelled")return tr("Cancelado");if(state=="interrupted")return tr("Interrumpido");return tr("Error");
}
void Hub::openDataFolder(){QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_store.databasePath()).absolutePath()));}
void Hub::copyText(const QString &text){QGuiApplication::clipboard()->setText(text);message(tr("Texto copiado."));}
}
