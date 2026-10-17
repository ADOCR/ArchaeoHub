#include "Controller.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QTimer>
#include <QFile>
#include <cstdio>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace {
QJsonObject snapshot(const Controller &c){
    QJsonArray history;
    for(const auto &item:c.history()){auto entry=QJsonObject::fromVariantMap(item.toMap());entry.remove("segments");entry.remove("minutes");history.append(entry);}
    return {{"files",QJsonArray::fromVariantList(c.files())},{"models",QJsonArray::fromVariantList(c.models())},
        {"segments",QJsonArray::fromVariantList(c.segments())},{"history",history},{"busy",c.busy()},
        {"downloading",c.downloading()},{"progress",c.progress()},{"downloadProgress",c.downloadProgress()},
        {"status",c.status()},{"error",c.error()},{"transcript",c.transcript()},{"minutes",c.minutes()},
        {"currentFile",c.currentFile()},{"engine",c.engine()},{"resultFolder",c.resultFolder()},
        {"modelIndex",c.modelIndex()},{"language",c.language()},{"device",c.device()},
        {"createMinutes",c.createMinutes()},{"ollamaModel",c.ollamaModel()},{"outputFolder",c.outputFolder()}};
}
void publish(const QJsonObject &object){const auto line="@ARCHAEOHUB@"+QJsonDocument(object).toJson(QJsonDocument::Compact)+'\n';std::fwrite(line.constData(),1,size_t(line.size()),stdout);std::fflush(stdout);}
}
int main(int argc,char **argv){
    QApplication app(argc,argv);app.setOrganizationName("Gamboa Desarrollos");app.setApplicationName("GamboaGlass");
    const auto config=qEnvironmentVariable("GAMBOA_CONFIG_DIR");
    if(!config.isEmpty()){QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,config);}
    Controller controller;bool downloadRequested=false;QTimer snapshots; snapshots.setSingleShot(true);snapshots.setInterval(80);
    QObject::connect(&snapshots,&QTimer::timeout,&app,[&]{publish({{"type","state"},{"state",snapshot(controller)}});});
    QObject::connect(&controller,&Controller::changed,&app,[&]{if(downloadRequested&&!controller.downloading()){downloadRequested=false;publish({{"type","downloadFinished"},{"ok",controller.error().isEmpty()},{"state",snapshot(controller)}});}if(!snapshots.isActive())snapshots.start();});
    QObject::connect(&controller,&Controller::batchFinished,&app,[&](bool ok){snapshots.stop();publish({{"type","finished"},{"ok",ok},{"state",snapshot(controller)}});});
    QByteArray input;QTimer poll;poll.setInterval(25);
#ifndef Q_OS_WIN
    fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL)|O_NONBLOCK);
#endif
    QObject::connect(&poll,&QTimer::timeout,&app,[&]{
        char bytes[16384];qint64 count=0;
#ifdef Q_OS_WIN
        DWORD available=0,read=0;const auto pipe=GetStdHandle(STD_INPUT_HANDLE);
        if(!PeekNamedPipe(pipe,nullptr,0,nullptr,&available,nullptr)){controller.cancel();if(!controller.busy())app.quit();return;}
        if(available&&ReadFile(pipe,bytes,qMin<DWORD>(available,sizeof(bytes)),&read,nullptr))count=read;
#else
        count=::read(STDIN_FILENO,bytes,sizeof(bytes));if(count==0){controller.cancel();if(!controller.busy())app.quit();return;}
#endif
        if(count>0)input.append(bytes,qsizetype(count));
        if(input.size()>2*1024*1024){publish({{"type","error"},{"error","La solicitud de transcripción es demasiado grande."}});app.quit();return;}
        while(input.contains('\n')){
            const auto end=input.indexOf('\n');const auto line=input.left(end);input.remove(0,end+1);
            QJsonParseError error;const auto request=QJsonDocument::fromJson(line,&error).object();const auto op=request["op"].toString();
            if(error.error!=QJsonParseError::NoError||request["schemaVersion"].toInt()!=1){publish({{"type","error"},{"error","Protocolo de transcripción no compatible."}});continue;}
            if(op=="add"){QStringList paths;for(const auto &path:request["paths"].toArray())paths<<path.toString();controller.addPaths(paths);}
            else if(op=="remove")controller.removeFile(request["index"].toInt(-1));
            else if(op=="clear")controller.clearFiles();
            else if(op=="start")controller.start();
            else if(op=="cancel")controller.cancel();
            else if(op=="download"){downloadRequested=true;controller.downloadModel();}
            else if(op=="history")controller.openHistory(request["index"].toInt(-1));
            else if(op=="dismiss")controller.dismissError();
            else if(op=="models"&&!controller.busy()&&!controller.downloading())app.setProperty("modelsRoot",request["path"].toString());
            else if(op=="settings"){
                if(request.contains("modelIndex"))controller.setModelIndex(request["modelIndex"].toInt());
                if(request.contains("language"))controller.setLanguage(request["language"].toString());
                if(request.contains("device"))controller.setDevice(request["device"].toString());
                if(request.contains("createMinutes"))controller.setCreateMinutes(request["createMinutes"].toBool());
                if(request.contains("ollamaModel"))controller.setOllamaModel(request["ollamaModel"].toString());
                if(request.contains("outputFolder"))controller.setOutputFolder(request["outputFolder"].toString());
            }else if(op=="close"){if(controller.busy()||controller.downloading())publish({{"type","error"},{"error","Espera a que termine o cancela la tarea antes de cerrar."}});else app.quit();}
            else{publish({{"type","error"},{"error","Acción de transcripción no reconocida."}});continue;}
            snapshots.start();
        }
    });poll.start();snapshots.start(0);return app.exec();
}
