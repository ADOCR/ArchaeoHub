#include "TranscriptionController.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcessEnvironment>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
namespace workbench::transcription {
TranscriptionController::TranscriptionController(TaskManager &tasks,const QString &directory,const QString &legacyProgram,QObject *parent)
    :QObject(parent),m_tasks(tasks),m_preferences(QDir(directory).filePath("transcription-preferences.ini"),QSettings::IniFormat){
    if(!m_preferences.contains("modelsFolder")){
        const auto legacy=QDir(QFileInfo(legacyProgram).absolutePath()).filePath("models");
        m_preferences.setValue("modelsFolder",!legacyProgram.isEmpty()&&QFileInfo(legacy).isDir()?legacy:QDir(directory).filePath("transcription/models"));
    }
    connect(&m_process,&QProcess::readyReadStandardOutput,this,&TranscriptionController::receive);
    connect(&m_process,&QProcess::readyReadStandardError,this,[this]{m_process.readAllStandardError();});
    connect(&m_process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){if(e==QProcess::FailedToStart){m_message=tr("No se pudo iniciar el motor de transcripción incluido. Revisa la instalación.");finishTask(false,m_message);emit changed();}});
    connect(&m_process,&QProcess::finished,this,[this](int code,QProcess::ExitStatus status){receive();m_ready=false;if(code!=0||status!=QProcess::NormalExit)m_message=tr("El motor de transcripción se cerró inesperadamente. Los archivos originales se conservan.");m_state["busy"]=false;m_state["downloading"]=false;finishTask(false,m_message);emit changed();});
#ifdef Q_OS_WIN
    m_process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
}
TranscriptionController::~TranscriptionController(){m_process.disconnect(this);m_process.closeWriteChannel();if(m_process.state()!=QProcess::NotRunning&&!m_process.waitForFinished(1200)){m_process.kill();m_process.waitForFinished(500);}if(!m_task.isEmpty())m_tasks.endExternal(m_task,"interrupted");}
void TranscriptionController::initialize(){
    if(m_process.state()!=QProcess::NotRunning)return;
    auto path=QDir(QCoreApplication::applicationDirPath()).filePath("modules/transcription/engine/TranscriptionWorker");
#ifdef Q_OS_WIN
    path+=".exe";
#endif
    auto environment=QProcessEnvironment::systemEnvironment();environment.insert("ARCHAEOHUB_MODELS_DIR",modelsFolder());
    m_process.setProgram(path);m_process.setArguments({});m_process.setWorkingDirectory(QFileInfo(path).absolutePath());m_process.setProcessEnvironment(environment);m_message.clear();m_buffer.clear();m_process.start();
}
void TranscriptionController::command(QJsonObject object){if(!m_ready)return;object["schemaVersion"]=1;m_process.write(QJsonDocument(object).toJson(QJsonDocument::Compact)+'\n');}
void TranscriptionController::receive(){
    m_buffer+=m_process.readAllStandardOutput();if(m_buffer.size()>64*1024*1024){m_message=tr("La respuesta del motor excede el tamaño admitido.");m_process.kill();return;}
    const QByteArray prefix="@ARCHAEOHUB@";
    while(m_buffer.contains('\n')){const auto end=m_buffer.indexOf('\n');auto line=m_buffer.left(end);m_buffer.remove(0,end+1);if(!line.startsWith(prefix))continue;line.remove(0,prefix.size());
        QJsonParseError error;const auto reply=QJsonDocument::fromJson(line,&error).object();if(error.error!=QJsonParseError::NoError)continue;
        if(reply.contains("state")){m_state=reply["state"].toObject();m_ready=true;m_message=m_state["error"].toString();if(!m_task.isEmpty())m_tasks.externalProgress(m_task,m_state[m_downloadTask?"downloadProgress":"progress"].toDouble());}
        if(reply["type"]=="error")m_message=reply["error"].toString();
        if(reply["type"]=="downloadFinished"){finishTask(reply["ok"].toBool(),m_message);m_downloadTask=false;}
        if(reply["type"]=="finished"){finishTask(reply["ok"].toBool(),m_message);emit batchFinished(reply["ok"].toBool());}
        emit changed();
    }
}
void TranscriptionController::finishTask(bool ok,const QString &error){if(m_task.isEmpty())return;const auto task=m_task;m_task.clear();m_tasks.endExternal(task,m_cancelled?"cancelled":ok?"succeeded":"failed",m_state["resultFolder"].toString(),error);}
void TranscriptionController::addFiles(const QVariantList &files){QJsonArray paths;for(const auto &file:files){const auto url=file.toUrl();if(url.isLocalFile())paths.append(url.toLocalFile());}command({{"op","add"},{"paths",paths}});}
void TranscriptionController::addFolder(const QUrl &folder){if(folder.isLocalFile())command({{"op","add"},{"paths",QJsonArray{folder.toLocalFile()}}});}
void TranscriptionController::useModelsFolder(const QUrl &folder){if(busy()||!folder.isLocalFile()||!QFileInfo(folder.toLocalFile()).isDir())return;m_preferences.setValue("modelsFolder",folder.toLocalFile());command({{"op","models"},{"path",folder.toLocalFile()}});emit changed();}
void TranscriptionController::settings(const QVariantMap &settings){auto request=QJsonObject::fromVariantMap(settings);request["op"]="settings";command(request);}
void TranscriptionController::removeFile(int index){command({{"op","remove"},{"index",index}});}void TranscriptionController::clearFiles(){command({{"op","clear"}});}
void TranscriptionController::start(){if(!m_ready||busy()||!m_task.isEmpty())return;m_cancelled=false;m_task=m_tasks.beginExternal(tr("Transcripción local"),"transcription.convert",[this]{cancel();});command({{"op","start"}});}
void TranscriptionController::cancel(){m_cancelled=true;command({{"op","cancel"}});}void TranscriptionController::downloadModel(){if(!m_ready||busy()||!m_task.isEmpty())return;m_cancelled=false;m_downloadTask=true;m_task=m_tasks.beginExternal(tr("Descarga de modelo"),"transcription.download",[this]{cancel();});command({{"op","download"}});}
void TranscriptionController::openHistory(int index){command({{"op","history"},{"index",index}});}void TranscriptionController::dismissError(){m_message.clear();command({{"op","dismiss"}});}
}
