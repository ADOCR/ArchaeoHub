#include "Store.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <stdexcept>

namespace workbench {
namespace {
QString now() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
QString uuid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
QString json(const QJsonObject &o) { return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)); }
QSqlQuery execute(const QSqlDatabase &db, const QString &sql, const QVariantList &values = {}) {
    QSqlQuery q(db);
    if (!q.prepare(sql)) throw std::runtime_error(q.lastError().text().toStdString());
    for (const auto &v : values) q.addBindValue(v);
    if (!q.exec()) throw std::runtime_error(q.lastError().text().toStdString());
    return q;
}
}
Store::Store(const QString &directory) {
    if (!QDir().mkpath(directory)) throw std::runtime_error("Cannot create application data directory");
    m_db = QSqlDatabase::addDatabase("QSQLITE", "workbench-" + uuid());
    m_db.setDatabaseName(QDir(directory).filePath("workbench.sqlite"));
    try {
        if (!m_db.open()) throw std::runtime_error(m_db.lastError().text().toStdString());
        execute(m_db, "PRAGMA foreign_keys=ON"); execute(m_db, "PRAGMA busy_timeout=1000");
        // WAL evita las operaciones repetidas del diario borrable en discos lentos; conserva la durabilidad FULL.
        execute(m_db, "PRAGMA journal_mode=WAL");execute(m_db, "PRAGMA synchronous=FULL");
        migrate();
        execute(m_db, "UPDATE tasks SET state='interrupted' WHERE state IN ('queued','running','cancelling')");
    } catch (...) {
        const auto connection=m_db.connectionName();m_db.close();m_db=QSqlDatabase();QSqlDatabase::removeDatabase(connection);throw;
    }
}
Store::~Store() {
    const auto name = m_db.connectionName(); m_db.close(); m_db = QSqlDatabase(); QSqlDatabase::removeDatabase(name);
}
void Store::migrate() {
    auto q = execute(m_db, "PRAGMA user_version"); q.next(); const int version = q.value(0).toInt(); q.finish();
    if (version > 1) throw std::runtime_error("Database schema is newer than this application");
    if (version == 1) return;
    auto existing = execute(m_db, "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
    if (existing.next()) throw std::runtime_error("Unrecognized database; existing tables were preserved");
    if (!m_db.transaction()) throw std::runtime_error("Cannot start schema transaction");
    try {
        const QStringList sql = {
            "CREATE TABLE tools(id TEXT PRIMARY KEY, metadata TEXT NOT NULL, hash TEXT NOT NULL)",
            "CREATE TABLE installations(id TEXT PRIMARY KEY, tool_id TEXT NOT NULL REFERENCES tools(id), program TEXT NOT NULL, interpreter TEXT NOT NULL, approved_hash TEXT NOT NULL, preferred INTEGER NOT NULL, updated_at TEXT NOT NULL)",
            "CREATE UNIQUE INDEX preferred_installation ON installations(tool_id) WHERE preferred=1",
            "CREATE TABLE favorites(tool_id TEXT PRIMARY KEY REFERENCES tools(id))",
            "CREATE TABLE recent_tools(tool_id TEXT PRIMARY KEY REFERENCES tools(id), opened_at TEXT NOT NULL, uses INTEGER NOT NULL DEFAULT 1)",
            "CREATE TABLE volumes(id TEXT PRIMARY KEY, identity_key TEXT NOT NULL UNIQUE, label TEXT, mount_hint TEXT, identity_kind TEXT, last_seen TEXT NOT NULL)",
            "CREATE TABLE projects(id TEXT PRIMARY KEY, name TEXT NOT NULL, location TEXT NOT NULL UNIQUE, created_at TEXT NOT NULL)",
            "CREATE TABLE recent_files(id TEXT PRIMARY KEY, name TEXT NOT NULL, location TEXT NOT NULL UNIQUE, opened_at TEXT NOT NULL)",
            "CREATE TABLE tasks(id TEXT PRIMARY KEY, kind TEXT NOT NULL, name TEXT NOT NULL, state TEXT NOT NULL, progress REAL, result TEXT, error TEXT, updated_at TEXT NOT NULL)",
            "CREATE TABLE task_events(id INTEGER PRIMARY KEY, task_id TEXT NOT NULL REFERENCES tasks(id), state TEXT NOT NULL, at TEXT NOT NULL)",
            "PRAGMA user_version=1"
        };
        for (const auto &statement : sql) execute(m_db, statement);
        if (!m_db.commit()) throw std::runtime_error("Cannot commit schema");
    } catch (...) { m_db.rollback(); throw; }
}
QString Store::databasePath() const { return m_db.databaseName(); }
void Store::registerTool(const ToolDescriptor &tool) {
    execute(m_db, "INSERT INTO tools VALUES(?,?,?) ON CONFLICT(id) DO UPDATE SET metadata=excluded.metadata,hash=excluded.hash",
            {tool.id, json(tool.json), tool.manifestHash});
}
QVariantMap Store::installation(const QString &id) const {
    auto q = execute(m_db, "SELECT program,interpreter,approved_hash FROM installations WHERE tool_id=? AND preferred=1", {id});
    if (!q.next()) return {};
    return {{"program", QJsonDocument::fromJson(q.value(0).toByteArray()).object().toVariantMap()},
            {"interpreter", QJsonDocument::fromJson(q.value(1).toByteArray()).object().toVariantMap()}, {"hash", q.value(2)}};
}
void Store::configure(const QString &id, const LocationRef &program, const LocationRef &interpreter, const QString &hash) {
    if (!m_db.transaction()) throw std::runtime_error("Cannot start installation transaction");
    try {
        execute(m_db, "UPDATE installations SET preferred=0 WHERE tool_id=?", {id});
        execute(m_db, "INSERT INTO installations VALUES(?,?,?,?,?,1,?)", {uuid(), id, json(program.toJson()), json(interpreter.toJson()), hash, now()});
        if (!m_db.commit()) throw std::runtime_error("Cannot save installation");
    } catch (...) { m_db.rollback(); throw; }
}
void Store::setFavorite(const QString &id, bool enabled) {
    execute(m_db, enabled ? "INSERT OR IGNORE INTO favorites VALUES(?)" : "DELETE FROM favorites WHERE tool_id=?", {id});
}
bool Store::isFavorite(const QString &id) const { auto q = execute(m_db, "SELECT 1 FROM favorites WHERE tool_id=?", {id}); return q.next(); }
void Store::recordUse(const QString &id) {
    execute(m_db, "INSERT INTO recent_tools(tool_id,opened_at) VALUES(?,?) ON CONFLICT(tool_id) DO UPDATE SET opened_at=excluded.opened_at,uses=uses+1", {id, now()});
}
QString Store::lastUsed(const QString &id) const { auto q = execute(m_db, "SELECT opened_at FROM recent_tools WHERE tool_id=?", {id}); return q.next() ? q.value(0).toString() : QString(); }
QString Store::bindVolume(const Volume &v) {
    auto q = execute(m_db, "SELECT id FROM volumes WHERE identity_key=?", {v.key});
    const auto id = q.next() ? q.value(0).toString() : uuid(); q.finish();
    execute(m_db, "INSERT INTO volumes VALUES(?,?,?,?,?,?) ON CONFLICT(identity_key) DO UPDATE SET label=excluded.label,mount_hint=excluded.mount_hint,last_seen=excluded.last_seen",
            {id, v.key, v.label, v.mount, v.identityKind, now()});
    return id;
}
QList<Volume> Store::bindVolumes(QList<Volume> volumes) {
    if(!m_db.transaction())throw std::runtime_error("Cannot start volume snapshot transaction");
    try {
        for(auto &volume:volumes)volume.id=bindVolume(volume);
        if(!m_db.commit())throw std::runtime_error("Cannot save volume snapshot");
        return volumes;
    } catch (...) {m_db.rollback();throw;}
}
QVariantList Store::knownVolumes() const {
    QVariantList rows; auto q = execute(m_db, "SELECT id,label,mount_hint,last_seen FROM volumes");
    while(q.next()) rows << QVariantMap{{"id",q.value(0)},{"label",q.value(1)},{"mount",q.value(2)},{"lastSeen",q.value(3)}};
    return rows;
}
QVariantList Store::projects() const {
    QVariantList rows; auto q=execute(m_db, "SELECT id,name,location FROM projects ORDER BY created_at DESC LIMIT 12");
    while(q.next()) rows << QVariantMap{{"id",q.value(0)},{"name",q.value(1)},{"location",QJsonDocument::fromJson(q.value(2).toByteArray()).object().toVariantMap()}};
    return rows;
}
void Store::addProject(const QString &name,const LocationRef &folder) {
    execute(m_db, "INSERT OR IGNORE INTO projects VALUES(?,?,?,?)", {uuid(),name,json(folder.toJson()),now()});
}
QVariantList Store::recentFiles() const {
    QVariantList rows; auto q=execute(m_db,"SELECT name,location,opened_at FROM recent_files ORDER BY opened_at DESC LIMIT 12");
    while(q.next()) rows << QVariantMap{{"name",q.value(0)},{"location",QJsonDocument::fromJson(q.value(1).toByteArray()).object().toVariantMap()},{"date",q.value(2)}};
    return rows;
}
void Store::recordFile(const QString &name,const LocationRef &location) {
    execute(m_db, "INSERT INTO recent_files VALUES(?,?,?,?) ON CONFLICT(location) DO UPDATE SET opened_at=excluded.opened_at", {uuid(),name,json(location.toJson()),now()});
}
void Store::saveTask(const QVariantMap &t) {
    if(!m_db.transaction())throw std::runtime_error("Cannot start task transaction");
    try {
        auto old=execute(m_db,"SELECT state FROM tasks WHERE id=?",{t["id"]}); const bool had=old.next(); const auto previous=had?old.value(0).toString():QString(); old.finish();
        execute(m_db,"INSERT INTO tasks VALUES(?,?,?,?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET state=excluded.state,progress=excluded.progress,result=excluded.result,error=excluded.error,updated_at=excluded.updated_at",
            {t["id"],t["kind"],t["name"],t["state"],t["progress"],t.value("result",""),t.value("error",""),now()});
        if (previous!=t["state"].toString()) execute(m_db,"INSERT INTO task_events(task_id,state,at) VALUES(?,?,?)",{t["id"],t["state"],now()});
        if(!m_db.commit())throw std::runtime_error("Cannot save task and history");
    } catch (...) {m_db.rollback();throw;}
}
QVariantList Store::tasks() const {
    QVariantList rows; auto q=execute(m_db,"SELECT id,kind,name,state,progress,result,error FROM tasks ORDER BY updated_at DESC LIMIT 30");
    while(q.next()) rows << QVariantMap{{"id",q.value(0)},{"kind",q.value(1)},{"name",q.value(2)},{"state",q.value(3)},{"progress",q.value(4)},{"result",q.value(5)},{"error",q.value(6)}};
    return rows;
}
}
