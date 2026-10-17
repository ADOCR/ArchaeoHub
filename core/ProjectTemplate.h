#pragma once
#include <QStringList>

namespace workbench {
struct ProjectTemplate {
    QString id;
    QStringList folders;
    QString error;
    explicit operator bool() const {return error.isEmpty()&&!folders.isEmpty();}
};
struct ProjectCreation {
    QString path;
    QString error;
    bool ok() const {return error.isEmpty()&&!path.isEmpty();}
};
class ProjectTemplates {
public:
    static ProjectTemplate parse(const QByteArray &json);
    static bool validName(const QString &name);
    static ProjectCreation create(const ProjectTemplate &definition,const QString &parent,const QString &name);
};
}
