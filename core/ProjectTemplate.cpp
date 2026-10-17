#include "ProjectTemplate.h"
#include "Paths.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

namespace workbench {
bool ProjectTemplates::validName(const QString &name){
    static const QRegularExpression invalid(R"([<>:"/\\|?*\x00-\x1F])");
    static const QRegularExpression reserved(R"(^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$))",QRegularExpression::CaseInsensitiveOption);
    return !name.isEmpty()&&name.size()<=120&&name==name.trimmed()&&!name.endsWith('.')&&
        name!="."&&name!=".."&&!invalid.match(name).hasMatch()&&!reserved.match(name).hasMatch();
}
ProjectTemplate ProjectTemplates::parse(const QByteArray &bytes){
    ProjectTemplate result;
    auto fail=[&]{result.error="project.invalid_template";return result;};
    if(bytes.size()>131072)return fail();
    const auto doc=QJsonDocument::fromJson(bytes);if(!doc.isObject())return fail();const auto root=doc.object();
    if(!root["schemaVersion"].isDouble()||root["schemaVersion"].toDouble()!=1||!root["id"].isString()||!root["folders"].isArray())return fail();
    result.id=root["id"].toString();QSet<QString> seen;
    if(!QRegularExpression("^[a-z][a-z0-9-]{0,63}$").match(result.id).hasMatch())return fail();
    const auto folders=root["folders"].toArray();if(folders.isEmpty()||folders.size()>500)return fail();
    static const QRegularExpression numbered("^[0-9]{2}_.+$");
    for(const auto value:folders){
        if(!value.isString())return fail();const auto path=value.toString();const auto parts=path.split('/');
        if(parts.size()>8||path.size()>900)return fail();
        for(const auto &part:parts)if(!validName(part)||!numbered.match(part).hasMatch())return fail();
        const auto key=path.toCaseFolded();if(seen.contains(key))return fail();
        if(parts.size()>1&&!seen.contains(path.left(path.lastIndexOf('/')).toCaseFolded()))return fail();
        seen.insert(key);result.folders<<path;
    }
    return result;
}
ProjectCreation ProjectTemplates::create(const ProjectTemplate &definition,const QString &parent,const QString &name){
    if(!definition)return {{},"project.invalid_template"};
    if(!validName(name))return {{},"project.invalid_name"};
    const QFileInfo parentInfo(parent);
    if(!parentInfo.isAbsolute()||!parentInfo.isDir())return {{},"project.parent_unavailable"};
    const auto base=parentInfo.canonicalFilePath();QDir directory(base);
    if(QFileInfo(directory.filePath(name)).exists()||QFileInfo(directory.filePath(name)).isSymLink())return {{},"project.already_exists"};
    const auto temporaryName=".workbench-project-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto staging=directory.filePath(temporaryName);
    if(!directory.mkdir(temporaryName))return {{},"project.create_failed"};
    QStringList created;
    auto rollback=[&]{
        // Elimina sólo los directorios vacíos creados por esta operación, sin borrado recursivo.
        for(auto it=created.crbegin();it!=created.crend();++it)
            if(Paths::contained(staging,*it))QDir().rmdir(*it);
        if(Paths::contained(base,staging))QDir().rmdir(staging);
    };
    for(const auto &relative:definition.folders){
        const auto destination=QDir(staging).filePath(relative);
        if(!Paths::contained(staging,destination)||!QDir().mkdir(destination)){rollback();return {{},"project.create_failed"};}
        created<<destination;
    }
    // Renombrar entre directorios hermanos publica el árbol completo y rechaza un destino existente.
    if(!directory.rename(temporaryName,name)){rollback();return {{},"project.finalize_failed"};}
    return {directory.filePath(name),{}};
}
}
