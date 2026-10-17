#pragma once
#include "services/TaskManager.h"
#include <QAbstractListModel>
#include <QProcess>
#include <QTimer>
#include <QSettings>
#include <QUrl>
#include <QJsonArray>

namespace workbench::coordinates {
QString coordinateError(const QString &code);
class CoordinateRows final : public QAbstractListModel {
    Q_OBJECT
public:
    explicit CoordinateRows(QObject *parent=nullptr):QAbstractListModel(parent){}
    int rowCount(const QModelIndex &parent={}) const override{return parent.isValid()?0:int(m_rows.size());}
    QVariant data(const QModelIndex &index,int role) const override;
    QHash<int,QByteArray> roleNames() const override;
    void replace(QJsonArray rows){beginResetModel();m_rows=std::move(rows);endResetModel();}
private:QJsonArray m_rows;
};
class CoordinateController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString technicalCode READ technicalCode NOTIFY changed)
    Q_PROPERTY(bool error READ error NOTIFY changed)
    Q_PROPERTY(QString sourceCode READ sourceCode NOTIFY changed)
    Q_PROPERTY(QString targetCode READ targetCode NOTIFY changed)
    Q_PROPERTY(QVariantMap sourceInfo READ sourceInfo NOTIFY changed)
    Q_PROPERTY(QVariantMap targetInfo READ targetInfo NOTIFY changed)
    Q_PROPERTY(QVariantMap engineInfo READ engineInfo NOTIFY changed)
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY changed)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY changed)
    Q_PROPERTY(int searchTotal READ searchTotal NOTIFY changed)
    Q_PROPERTY(int searchOffset READ searchOffset NOTIFY changed)
    Q_PROPERTY(QVariantList recentCrs READ recentCrs NOTIFY changed)
    Q_PROPERTY(QString tableText READ tableText WRITE setTableText NOTIFY changed)
    Q_PROPERTY(QString inputFile READ inputFile NOTIFY changed)
    Q_PROPERTY(QStringList sheets READ sheets NOTIFY changed)
    Q_PROPERTY(QString inputSheet READ inputSheet NOTIFY changed)
    Q_PROPERTY(CoordinateRows* rows READ rows CONSTANT)
public:
    CoordinateController(TaskManager &tasks,const QString &dataDirectory,QObject *parent=nullptr);
    ~CoordinateController() override;
    bool busy() const{return m_busy;}double progress() const{return m_progress;}
    QString message() const{return m_message;}QString technicalCode() const{return m_code;}bool error() const{return m_error;}
    QString sourceCode() const{return m_sourceCode;}QString targetCode() const{return m_targetCode;}
    QVariantMap sourceInfo() const{return m_source.toVariantMap();}QVariantMap targetInfo() const{return m_target.toVariantMap();}
    QVariantMap engineInfo() const{return m_engine.toVariantMap();}QVariantMap summary() const;
    QVariantList searchResults() const{return m_search.toVariantList();}QVariantList recentCrs() const;
    int searchTotal() const{return m_searchTotal;}int searchOffset() const{return m_searchOffset;}
    QString tableText() const{return m_tableText;}void setTableText(const QString &text){if(!m_busy&&text!=m_tableText){m_tableText=text;emit changed();}}
    QString inputFile() const{return m_inputFile;}CoordinateRows *rows(){return &m_rows;}
    QStringList sheets() const{return m_sheets;}QString inputSheet() const{return m_inputSheet;}
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void describe(const QString &source,const QString &target);
    Q_INVOKABLE void search(const QString &query,const QString &group=QString(),int offset=0);
    Q_INVOKABLE void convertSingle(const QString &id,const QString &x,const QString &y,const QVariantMap &options);
    Q_INVOKABLE void convertBatch(const QVariantMap &options);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void importText(const QUrl &file);
    Q_INVOKABLE void importSpreadsheet(const QUrl &file,const QString &sheet=QString());
    Q_INVOKABLE void selectSheet(const QString &sheet);
    Q_INVOKABLE void exportFile(const QUrl &file,const QString &format);
    Q_INVOKABLE QString exportText(const QString &format) const;
    Q_INVOKABLE void example();
    bool request(QJsonObject request);
    QJsonObject lastResult() const{return m_result;}
signals:
    void changed();
    void conversionCompleted();
    void spreadsheetSheetsAvailable();
    void spreadsheetImported();
private:
    QString csvText(bool protectFormulas) const;
    void receive();void finish(int exitCode,QProcess::ExitStatus status);
    void notice(const QString &code,bool error=true);
    void remember(const QString &code);
    TaskManager &m_tasks;QSettings m_preferences;CoordinateRows m_rows;QProcess m_process;QTimer m_deadline;
    bool m_busy=false,m_error=false,m_cancelled=false;double m_progress=0;
    int m_searchTotal=0,m_searchOffset=0;
    QString m_sourceCode,m_targetCode,m_message,m_code,m_operation,m_task,m_tableText,m_inputFile;
    QStringList m_sheets;QString m_sheetPath,m_inputSheet;QJsonObject m_spreadsheetInput;
    QJsonObject m_source,m_target,m_engine,m_result,m_reply;QJsonArray m_search;QByteArray m_buffer;
};
}
