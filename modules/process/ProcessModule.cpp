#include "ProcessModule.h"
#include <QFileInfo>
#include <QProcess>
#include <QDir>
namespace workbench {
LaunchResult ProcessModule::validate(const ToolDescriptor &t,const LaunchRequest &r){
    if(t.planned||!t.capabilities.contains("launch"))return {false,"module.not_available",{}};
    if(!QStringList{"external","python","script"}.contains(t.runtime))return {false,"runtime.not_supported",{}};
    if(!QFileInfo(r.program).isAbsolute()||!QFileInfo(r.program).isFile())return {false,"program.not_found",{}};
    if(t.runtime!="external"){
        if(!QFileInfo(r.interpreter).isAbsolute()||!QFileInfo(r.interpreter).isFile()||!QFileInfo(r.interpreter).isExecutable())return {false,"interpreter.not_found",{}};
    }else{
        if(!QFileInfo(r.program).isExecutable())return {false,"program.not_executable",{}};
#ifdef Q_OS_WIN
        if(QFileInfo(r.program).suffix().compare("exe",Qt::CaseInsensitive)!=0)return {false,"program.requires_interpreter",{}};
#endif
    }
    return {true,{},{}};
}
LaunchResult ProcessModule::open(const LaunchRequest &request){
    auto result=validate(m_tool,request);if(!result.ok)return result;
    QProcess process;
    auto arguments=request.arguments;
    if(m_tool.runtime=="external")process.setProgram(request.program);
    else{process.setProgram(request.interpreter);arguments.prepend(request.program);}
    process.setArguments(arguments);process.setWorkingDirectory(QFileInfo(request.program).absolutePath());
    process.setStandardInputFile(QProcess::nullDevice());
    process.setStandardOutputFile(QProcess::nullDevice());
    process.setStandardErrorFile(QProcess::nullDevice());
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *arguments){arguments->inheritHandles=false;});
#endif
    result.ok=process.startDetached(&result.processId);
    if(!result.ok){result.code="process.start_failed";result.detail=process.errorString();}
    if(m_context.log)m_context.log(m_tool.id,result.ok?"process.launched":result.code);
    return result;
}
}
