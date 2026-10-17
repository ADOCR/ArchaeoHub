#include <QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QSqlQuery>
#include <QUuid>
#include <limits>
#include "core/Manifest.h"
#include "core/Paths.h"
#include "core/ProjectTemplate.h"
#include "services/Store.h"
#include "services/Settings.h"
#include "services/TaskManager.h"
#include "modules/process/ProcessModule.h"
using namespace workbench;

class Contracts final : public QObject {
    Q_OBJECT
    QJsonObject manifest() const {
        return {{"schemaVersion",1},{"id","test.transcription"},{"version","1.0.0"},{"name",QJsonObject{{"es","Prueba"}}},
            {"description","Local test"},{"runtime","external"},{"entrypoint",QJsonObject{{"binding","selected-installation"}}},
            {"categoryIds",QJsonArray{"documentation"}},{"capabilities",QJsonArray{"launch"}},{"supportedFileTypes",QJsonArray{"audio/*"}}};
    }
    ToolDescriptor tool() const{return parseManifest(QJsonDocument(manifest()).toJson()).tool;}
private slots:
    void locationSerializationInvariants(){
        QTemporaryDir source,target;Volume old;old.id="stable-volume";old.mount=source.path();old.strongIdentity=true;
        Volume moved=old;moved.mount=target.path();
        for(int i=0;i<100;++i){
            const auto relative=QString::fromUtf8("00_Proyecto á/01_Datos %1/archivo.%2.csv").arg(i).arg(i+1);
            const auto original=QDir(source.path()).filePath(relative);
            const auto reference=Paths::reference(original,{old});
            const auto serialized=QJsonDocument(reference.toJson()).toJson(QJsonDocument::Compact);
            const auto restored=LocationRef::fromJson(QJsonDocument::fromJson(serialized).object());
            QCOMPARE(Paths::resolve(restored,{old}),original);
            QCOMPARE(Paths::resolve(restored,{moved}),QDir(target.path()).filePath(relative));
            QVERIFY(Paths::resolve(restored,{}).isEmpty());
        }
    }
    void projectTemplateCreationAndPreservation(){
        QFile file(QString(WORKBENCH_SOURCE_DIR)+"/resources/project-templates/archaeology.json");QVERIFY(file.open(QIODevice::ReadOnly));
        const auto definition=ProjectTemplates::parse(file.readAll());QVERIFY(definition);QCOMPARE(definition.folders.size(),78);
        auto invalidSchema=QJsonDocument::fromJson(QByteArray("{\"schemaVersion\":1.5,\"id\":\"test\",\"folders\":[\"00_Data\"]}"));
        QVERIFY(!ProjectTemplates::parse(invalidSchema.toJson()));
        QTemporaryDir parent;const auto created=ProjectTemplates::create(definition,parent.path(),QString::fromUtf8("Proyecto prueba á"));QVERIFY(created.ok());
        for(const auto &relative:definition.folders)QVERIFY(QFileInfo(QDir(created.path).filePath(relative)).isDir());
        QVERIFY(!QFileInfo(QDir(created.path).filePath("Descargas")).exists());
        QFile keep(QDir(created.path).filePath("keep.txt"));QVERIFY(keep.open(QIODevice::WriteOnly));keep.write("preserved");keep.close();
        QCOMPARE(ProjectTemplates::create(definition,parent.path(),QString::fromUtf8("Proyecto prueba á")).error,QString("project.already_exists"));
        QVERIFY(keep.open(QIODevice::ReadOnly));QCOMPARE(keep.readAll(),QByteArray("preserved"));
        for(const auto &name:QStringList{"../escape","CON","a/b","trailing.","LPT1.txt"," x","x\\y"})QVERIFY(!ProjectTemplates::validName(name));
        for(const auto &folders:QList<QJsonArray>{QJsonArray{"../01_escape"},QJsonArray{"not-numbered"},QJsonArray{"00_A/01_B"},QJsonArray{"00_A","00_a"}}){
            const QJsonObject json{{"schemaVersion",1},{"id","test"},{"folders",folders}};
            QVERIFY(!ProjectTemplates::parse(QJsonDocument(json).toJson()));
        }
        auto bad=definition;bad.folders<<"../escape";
        QVERIFY(!ProjectTemplates::create(bad,parent.path(),"Rollback").ok());
        QVERIFY(!QFileInfo(QDir(parent.path()).filePath("Rollback")).exists());
        QCOMPARE(QDir(parent.path()).entryList({".workbench-project-*"},QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot).size(),0);
    }
    void manifestValidation(){
        const auto valid=parseManifest(QJsonDocument(manifest()).toJson());QVERIFY(valid);QCOMPARE(valid.tool.name,QString("Prueba"));
        for(const auto &key:QStringList{"id","name","version","runtime","categoryIds","capabilities","supportedFileTypes","entrypoint"}){
            auto json=manifest();json.remove(key);QVERIFY2(!parseManifest(QJsonDocument(json).toJson()),qPrintable(key));
        }
        auto json=manifest();json["schemaVersion"]=2;QVERIFY(!parseManifest(QJsonDocument(json).toJson()));
        json=manifest();json["categoryIds"]=QJsonArray{5};QVERIFY(!parseManifest(QJsonDocument(json).toJson()));
        QVERIFY(!parseManifest("[1,2]"));QVERIFY(!parseManifest(QByteArray(262145,' ')));
    }
    void unsafeEntrypoints(){
        for(const auto &path:QStringList{"../outside.exe","bin/../../evil","/usr/bin/app","C:\\app.exe","bin\\..\\evil.exe"}){
            auto json=manifest();json["entrypoint"]=QJsonObject{{"path",path}};QVERIFY2(!parseManifest(QJsonDocument(json).toJson()),qPrintable(path));
        }
        auto json=manifest();json["entrypoint"]=QJsonObject{{"path","bin/app.exe"}};QVERIFY(parseManifest(QJsonDocument(json).toJson()));
    }
    void volumeRelocationAndMissingDisk(){
        QTemporaryDir a,b;QVERIFY(a.isValid()&&b.isValid());
        Volume first;first.id="volume-a";first.mount=a.path();first.strongIdentity=true;
        const auto ref=Paths::reference(QDir(a.path()).filePath("data/file.wav"),{first});QCOMPARE(ref.volumeId,QString("volume-a"));
        Volume moved=first;moved.mount=b.path();
        QCOMPARE(Paths::resolve(ref,{moved}),QDir(b.path()).filePath("data/file.wav"));
        QCOMPARE(Paths::resolve(ref,{}),QString());
        Volume wrong=first;wrong.id="other-volume";QCOMPARE(Paths::resolve(ref,{wrong}),QString());
        QCOMPARE(LocationRef::fromJson(ref.toJson()).relativePath,ref.relativePath);
        QVERIFY(Paths::resolve({"volume-a","../escape",{}},{first}).isEmpty());
        QVERIFY(!Paths::contained(a.path(),a.path()+"-other/file"));
        QVERIFY(Paths::reference({},{}).absoluteHint.isEmpty());
    }
    void sqlitePersistenceAndRollback(){
        QTemporaryDir dir;const auto t=tool();
        {Store store(dir.path());store.registerTool(t);store.setFavorite(t.id,true);
            store.configure(t.id,{{},{},"/old/program"},{},t.manifestHash);
            bool failed=false;try{store.configure("missing.tool",{{},{},"/other"},{},"hash");}catch(const std::exception &){failed=true;}
            QVERIFY(failed);QVERIFY(store.isFavorite(t.id));QVERIFY(!store.installation(t.id).isEmpty());
            store.recordUse(t.id);store.saveTask({{"id","job-1"},{"kind","file.hash"},{"name","fixture"},{"state","running"},{"progress",.2}});
        }
        {Store store(dir.path());QVERIFY(store.isFavorite(t.id));QVERIFY(!store.lastUsed(t.id).isEmpty());QCOMPARE(store.tasks().first().toMap()["state"].toString(),QString("interrupted"));}
    }
    void settingsValidationAndPersistence(){
        QTemporaryDir dir;
        {Settings s(dir.path());s.setTheme("dark");s.setTheme("invalid");QCOMPARE(s.theme(),QString("dark"));s.setReduceTransparency(true);s.setTextScale(100);QCOMPARE(s.textScale(),1.4);s.setTextScale(std::numeric_limits<double>::quiet_NaN());QCOMPARE(s.textScale(),1.4);}
        {Settings s(dir.path());QVERIFY(s.reduceTransparency());QCOMPARE(s.theme(),QString("dark"));}
    }
    void taskAndVolumeBatchesRollbackTogether(){
        QTemporaryDir root;Store store(root.path());const auto connection=QUuid::createUuid().toString();
        {
            auto db=QSqlDatabase::addDatabase("QSQLITE",connection);db.setDatabaseName(store.databasePath());QVERIFY(db.open());
            QSqlQuery q(db);
            QVERIFY(q.exec("CREATE TRIGGER reject_failed_event BEFORE INSERT ON task_events WHEN NEW.state='failed' BEGIN SELECT RAISE(ABORT,'fixture failure'); END"));
            QVariantMap task{{"id","atomic-job"},{"name","fixture"},{"kind","test"},{"state","running"},{"progress",.2}};
            store.saveTask(task);task["state"]="failed";task["progress"]=.7;
            bool rejected=false;try{store.saveTask(task);}catch(const std::exception &){rejected=true;}QVERIFY(rejected);
            QCOMPARE(store.tasks().first().toMap()["state"].toString(),QString("running"));QCOMPARE(store.tasks().first().toMap()["progress"].toDouble(),.2);
            QVERIFY(q.exec("SELECT count(*) FROM task_events WHERE task_id='atomic-job'"));QVERIFY(q.next());QCOMPARE(q.value(0).toInt(),1);q.finish();
            task["state"]="succeeded";store.saveTask(task);QCOMPARE(store.tasks().first().toMap()["state"].toString(),QString("succeeded"));
            QVERIFY(q.exec("CREATE TRIGGER reject_volume BEFORE INSERT ON volumes WHEN NEW.identity_key='reject' BEGIN SELECT RAISE(ABORT,'fixture failure'); END"));
            Volume good;good.key="stable-test";good.mount="/test";good.identityKind="test";Volume bad=good;bad.key="reject";
            rejected=false;try{store.bindVolumes({good,bad});}catch(const std::exception &){rejected=true;}QVERIFY(rejected);QVERIFY(store.knownVolumes().isEmpty());
            const auto first=store.bindVolumes({good});QCOMPARE(first.size(),1);const auto second=store.bindVolumes({good});QCOMPARE(second.first().id,first.first().id);
            db.close();
        }
        QSqlDatabase::removeDatabase(connection);
    }
    void rejectFutureAndUnrecognizedDatabase(){
        for(const int version:{0,99}){
            QTemporaryDir dir;const auto connection=QUuid::createUuid().toString();
            {auto db=QSqlDatabase::addDatabase("QSQLITE",connection);db.setDatabaseName(QDir(dir.path()).filePath("workbench.sqlite"));QVERIFY(db.open());
                QSqlQuery q(db);QVERIFY(q.exec("CREATE TABLE keep_me(value TEXT)"));QVERIFY(q.exec("INSERT INTO keep_me VALUES('preserved')"));QVERIFY(q.exec(QString("PRAGMA user_version=%1").arg(version)));db.close();}
            QSqlDatabase::removeDatabase(connection);
            bool rejected=false;try{Store store(dir.path());}catch(const std::exception &){rejected=true;}QVERIFY(rejected);
            {auto db=QSqlDatabase::addDatabase("QSQLITE",connection);db.setDatabaseName(QDir(dir.path()).filePath("workbench.sqlite"));QVERIFY(db.open());QSqlQuery q(db);QVERIFY(q.exec("SELECT value FROM keep_me"));QVERIFY(q.next());QCOMPARE(q.value(0).toString(),QString("preserved"));db.close();}
            QSqlDatabase::removeDatabase(connection);
        }
    }
    void launchValidation(){
        auto t=tool();QVERIFY(ProcessModule::validate(t,{QCoreApplication::applicationFilePath(),{}, {}}).ok);
        QVERIFY(!ProcessModule::validate(t,{"relative.exe",{},{}}).ok);
        t.runtime="python";QCOMPARE(ProcessModule::validate(t,{QCoreApplication::applicationFilePath(),{}, {}}).code,QString("interpreter.not_found"));
        t.planned=true;QCOMPARE(ProcessModule::validate(t,{QCoreApplication::applicationFilePath(),{}, {}}).code,QString("module.not_available"));
    }
    void realHashAndCancellation(){
        QTemporaryDir dir;QFile input(QDir(dir.path()).filePath("abc.txt"));QVERIFY(input.open(QIODevice::WriteOnly));input.write("abc");input.close();
        Store store(dir.path());TaskManager tasks(store);QVERIFY(!tasks.hashFile(input.fileName()).isEmpty());
        QTRY_COMPARE_WITH_TIMEOUT(tasks.activeCount(),0,5000);
        QCOMPARE(tasks.items().first().toMap()["result"].toString(),QString("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
        QFile large(QDir(dir.path()).filePath("cancel.bin"));QVERIFY(large.open(QIODevice::WriteOnly));large.resize(128*1024*1024);large.close();
        const auto id=tasks.hashFile(large.fileName());tasks.cancel(id);QTRY_COMPARE_WITH_TIMEOUT(tasks.activeCount(),0,5000);
        const auto rows=tasks.items();bool found=false;for(const auto &r:rows)if(r.toMap()["id"]==id){QCOMPARE(r.toMap()["state"].toString(),QString("cancelled"));found=true;}QVERIFY(found);
    }
};
QTEST_MAIN(Contracts)
#include "core_tests.moc"
