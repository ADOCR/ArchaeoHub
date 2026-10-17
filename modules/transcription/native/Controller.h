#pragma once
#include <QObject>
#include <QVariantList>
#include <QThread>
#include <QNetworkAccessManager>
#include <QFile>
#include <atomic>
#include <functional>

class Controller : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList files READ files NOTIFY changed)
    Q_PROPERTY(QVariantList models READ models NOTIFY changed)
    Q_PROPERTY(QVariantList segments READ segments NOTIFY changed)
    Q_PROPERTY(QVariantList history READ history NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool downloading READ downloading NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString transcript READ transcript NOTIFY changed)
    Q_PROPERTY(QString minutes READ minutes NOTIFY changed)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY changed)
    Q_PROPERTY(QString engine READ engine NOTIFY changed)
    Q_PROPERTY(QString resultFolder READ resultFolder NOTIFY changed)
    Q_PROPERTY(int modelIndex READ modelIndex WRITE setModelIndex NOTIFY changed)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)
    Q_PROPERTY(QString device READ device WRITE setDevice NOTIFY changed)
    Q_PROPERTY(bool createMinutes READ createMinutes WRITE setCreateMinutes NOTIFY changed)
    Q_PROPERTY(QString ollamaModel READ ollamaModel WRITE setOllamaModel NOTIFY changed)
    Q_PROPERTY(QString outputFolder READ outputFolder WRITE setOutputFolder NOTIFY changed)
public:
    explicit Controller(QObject *parent=nullptr);
    ~Controller();
    QVariantList files() const{return m_files;}
    QVariantList models() const;
    QVariantList segments() const{return m_segments;}
    QVariantList history() const{return m_history;}
    bool busy() const{return m_busy;}
    bool downloading() const{return m_downloading;}
    double progress() const{return m_progress;}
    double downloadProgress() const{return m_downloadProgress;}
    QString status() const{return m_status;}
    QString error() const{return m_error;}
    QString transcript() const;
    QString minutes() const{return m_minutes;}
    QString currentFile() const{return m_currentFile;}
    QString engine() const{return m_engine;}
    QString resultFolder() const{return m_resultFolder;}
    int modelIndex() const{return m_modelIndex;}
    QString language() const{return m_language;}
    QString device() const{return m_device;}
    bool createMinutes() const{return m_createMinutes;}
    QString ollamaModel() const{return m_ollamaModel;}
    QString outputFolder() const{return m_outputFolder;}
    void setModelIndex(int); void setLanguage(const QString&); void setDevice(const QString&);
    void setCreateMinutes(bool); void setOllamaModel(const QString&); void setOutputFolder(const QString&);
    Q_INVOKABLE void pickFiles();
    Q_INVOKABLE void pickFolder();
    Q_INVOKABLE void pickOutput();
    Q_INVOKABLE void addUrls(const QVariantList&);
    void addPaths(const QStringList&);
    Q_INVOKABLE void removeFile(int);
    Q_INVOKABLE void clearFiles();
    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void copyText(bool minute=false);
    Q_INVOKABLE void openResults();
    Q_INVOKABLE void openHistory(int);
    Q_INVOKABLE void downloadModel();
    Q_INVOKABLE void dismissError();
    QVariantMap result() const;
signals:
    void changed();
    void batchFinished(bool success);
private:
    void persist(); void saveHistory(); void downloadNext();
    void post(const std::function<void()> &fn);
    QString modelPath(int index) const;
    QVariantList m_files,m_segments,m_history;
    bool m_busy=false,m_downloading=false,m_createMinutes=false;
    double m_progress=0,m_downloadProgress=0;
    QString m_status="Listo para escuchar",m_error,m_minutes,m_currentFile,m_engine,m_resultFolder;
    int m_modelIndex=0;
    QString m_language="es",m_device="auto",m_ollamaModel="qwen3:8b",m_outputFolder;
    std::atomic_bool m_cancel{false};
    QThread *m_thread=nullptr;
    QNetworkAccessManager m_network;
    QList<QPair<QString,QString>> m_downloadQueue;
    QFile m_downloadFile;
};
