#include "elo/content/content_database.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUrl>
#include <sqlite3.h>
#include <stdexcept>

namespace elo::content {
namespace {
void check(int code, sqlite3* db) {
    if (code != SQLITE_OK && code != SQLITE_DONE && code != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db));
}
void exec(sqlite3* db, const char* sql) {
    char* error = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error ? error : sqlite3_errmsg(db);
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}
QString canonical_id(QString id) {
    for (int i = 0; i < 3; ++i) {
        const auto decoded = QUrl::fromPercentEncoding(id.toUtf8());
        if (decoded == id) break;
        id = decoded;
    }
    return id.trimmed();
}
QJsonObject parse_object(const QByteArray& bytes) {
    const auto doc = QJsonDocument::fromJson(bytes);
    return doc.isObject() ? doc.object() : QJsonObject{};
}
void write_json(const QString& path, const QJsonDocument& document) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(document.toJson(QJsonDocument::Indented)) < 0 || !file.commit())
        throw std::runtime_error("Failed to export editorial JSON snapshot");
}
}

ContentDatabase::ContentDatabase(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    if (sqlite3_open_v2(path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK)
        throw std::runtime_error(db_ ? sqlite3_errmsg(db_) : "Cannot open editorial SQLite database");
    sqlite3_busy_timeout(db_, 5000);
    exec(db_, "PRAGMA journal_mode=WAL; PRAGMA synchronous=FULL; PRAGMA foreign_keys=ON;"
              "CREATE TABLE IF NOT EXISTS content_atoms(content_id TEXT PRIMARY KEY, document_json TEXT NOT NULL, updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
              "CREATE TABLE IF NOT EXISTS content_relations(relation_id TEXT PRIMARY KEY, document_json TEXT NOT NULL, updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
              "CREATE TABLE IF NOT EXISTS content_recipes(recipe_id TEXT PRIMARY KEY, document_json TEXT NOT NULL, updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
              "CREATE TABLE IF NOT EXISTS package_plans(bundle_id TEXT PRIMARY KEY, document_json TEXT NOT NULL, updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
              "CREATE TABLE IF NOT EXISTS package_atoms(bundle_id TEXT NOT NULL REFERENCES package_plans(bundle_id) ON DELETE CASCADE, content_id TEXT NOT NULL REFERENCES content_atoms(content_id) ON DELETE CASCADE, position INTEGER NOT NULL, PRIMARY KEY(bundle_id,content_id));"
              "CREATE TABLE IF NOT EXISTS editorial_revisions(sequence INTEGER PRIMARY KEY AUTOINCREMENT, entity_kind TEXT NOT NULL, entity_id TEXT NOT NULL, operation TEXT NOT NULL, occurred_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
              "CREATE INDEX IF NOT EXISTS idx_package_atoms_content ON package_atoms(content_id);");
}
ContentDatabase::~ContentDatabase() { if (db_) sqlite3_close(db_); }

bool ContentDatabase::is_empty() const {
    sqlite3_stmt* s{}; check(sqlite3_prepare_v2(db_, "SELECT NOT EXISTS(SELECT 1 FROM content_atoms) AND NOT EXISTS(SELECT 1 FROM package_plans);", -1, &s, nullptr), db_);
    const bool result = sqlite3_step(s) == SQLITE_ROW && sqlite3_column_int(s, 0); sqlite3_finalize(s); return result;
}

static QJsonArray rows(sqlite3* db, const char* sql) {
    sqlite3_stmt* s{}; check(sqlite3_prepare_v2(db, sql, -1, &s, nullptr), db); QJsonArray out;
    while (sqlite3_step(s) == SQLITE_ROW) out.append(parse_object(reinterpret_cast<const char*>(sqlite3_column_text(s, 0))));
    sqlite3_finalize(s); return out;
}
QJsonArray ContentDatabase::atoms() const { return rows(db_, "SELECT document_json FROM content_atoms ORDER BY content_id;"); }
QJsonArray ContentDatabase::relations() const { return rows(db_, "SELECT document_json FROM content_relations ORDER BY relation_id;"); }
QJsonArray ContentDatabase::recipes() const { return rows(db_, "SELECT document_json FROM content_recipes ORDER BY recipe_id;"); }
QJsonArray ContentDatabase::packages() const { return rows(db_, "SELECT document_json FROM package_plans ORDER BY bundle_id;"); }

static QJsonObject one(sqlite3* db, const char* sql, const QString& id) {
    sqlite3_stmt* s{}; check(sqlite3_prepare_v2(db, sql, -1, &s, nullptr), db); const auto utf = canonical_id(id).toUtf8(); sqlite3_bind_text(s,1,utf.constData(),-1,SQLITE_TRANSIENT);
    QJsonObject out; if (sqlite3_step(s)==SQLITE_ROW) out=parse_object(reinterpret_cast<const char*>(sqlite3_column_text(s,0))); sqlite3_finalize(s); return out;
}
QJsonObject ContentDatabase::atom(const QString& id) const { return one(db_, "SELECT document_json FROM content_atoms WHERE content_id=?;", id); }
QJsonObject ContentDatabase::package(const QString& id) const { return one(db_, "SELECT document_json FROM package_plans WHERE bundle_id=?;", id); }

static void upsert(sqlite3* db, const char* table, const char* key, const QString& id, const QJsonObject& value, const char* kind) {
    const std::string sql="INSERT INTO "+std::string(table)+"("+key+",document_json) VALUES(?,?) ON CONFLICT("+key+") DO UPDATE SET document_json=excluded.document_json,updated_at=CURRENT_TIMESTAMP;";
    sqlite3_stmt* s{}; check(sqlite3_prepare_v2(db,sql.c_str(),-1,&s,nullptr),db); const auto i=id.toUtf8(), j=QJsonDocument(value).toJson(QJsonDocument::Compact); sqlite3_bind_text(s,1,i.constData(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(s,2,j.constData(),j.size(),SQLITE_TRANSIENT); check(sqlite3_step(s),db); sqlite3_finalize(s);
    s=nullptr; check(sqlite3_prepare_v2(db,"INSERT INTO editorial_revisions(entity_kind,entity_id,operation) VALUES(?,?,'upsert');",-1,&s,nullptr),db); sqlite3_bind_text(s,1,kind,-1,SQLITE_STATIC); sqlite3_bind_text(s,2,i.constData(),-1,SQLITE_TRANSIENT); check(sqlite3_step(s),db); sqlite3_finalize(s);
}
void ContentDatabase::upsert_atom(const QJsonObject& input) { auto v=input; const auto id=canonical_id(v.value("content_id").toString()); v["content_id"]=id; upsert(db_,"content_atoms","content_id",id,v,"atom"); }

bool ContentDatabase::delete_atom(const QString& input) {
    const QString canonical=canonical_id(input); const auto id=canonical.toUtf8();
    exec(db_,"BEGIN IMMEDIATE;");
    try {
        for (const auto& value : packages()) {
            auto plan=value.toObject(); QJsonArray kept;
            for(const auto& atomId:plan.value("atom_ids").toArray()) if(canonical_id(atomId.toString())!=canonical) kept.append(atomId);
            if(kept.size()!=plan.value("atom_ids").toArray().size()) { plan["atom_ids"]=kept; const auto bundle=plan.value("bundle_id").toString().toUtf8(), json=QJsonDocument(plan).toJson(QJsonDocument::Compact); sqlite3_stmt* u{};check(sqlite3_prepare_v2(db_,"UPDATE package_plans SET document_json=?,updated_at=CURRENT_TIMESTAMP WHERE bundle_id=?;",-1,&u,nullptr),db_);sqlite3_bind_text(u,1,json.constData(),json.size(),SQLITE_TRANSIENT);sqlite3_bind_text(u,2,bundle.constData(),-1,SQLITE_TRANSIENT);check(sqlite3_step(u),db_);sqlite3_finalize(u); }
        }
        sqlite3_stmt* s{}; check(sqlite3_prepare_v2(db_,"DELETE FROM content_atoms WHERE content_id=?;",-1,&s,nullptr),db_); sqlite3_bind_text(s,1,id.constData(),-1,SQLITE_TRANSIENT); check(sqlite3_step(s),db_); const bool changed=sqlite3_changes(db_)>0; sqlite3_finalize(s); exec(db_,"COMMIT;"); return changed;
    } catch(...) { exec(db_,"ROLLBACK;"); throw; }
}
void ContentDatabase::replace_relations(const QJsonArray& values) { exec(db_,"BEGIN IMMEDIATE; DELETE FROM content_relations;"); try { for(const auto& x:values){auto o=x.toObject();upsert(db_,"content_relations","relation_id",o.value("relation_id").toString(),o,"relation");} exec(db_,"COMMIT;"); } catch(...) { exec(db_,"ROLLBACK;"); throw; } }
void ContentDatabase::replace_recipes(const QJsonArray& values) { exec(db_,"BEGIN IMMEDIATE; DELETE FROM content_recipes;"); try { for(const auto& x:values){auto o=x.toObject();upsert(db_,"content_recipes","recipe_id",o.value("recipe_id").toString(),o,"recipe");} exec(db_,"COMMIT;"); } catch(...) { exec(db_,"ROLLBACK;"); throw; } }
void ContentDatabase::upsert_package(const QJsonObject& input) {
    const auto id=input.value("bundle_id").toString(); exec(db_,"BEGIN IMMEDIATE;"); try { upsert(db_,"package_plans","bundle_id",id,input,"package"); sqlite3_stmt* s{}; check(sqlite3_prepare_v2(db_,"DELETE FROM package_atoms WHERE bundle_id=?;",-1,&s,nullptr),db_); auto u=id.toUtf8();sqlite3_bind_text(s,1,u.constData(),-1,SQLITE_TRANSIENT);check(sqlite3_step(s),db_);sqlite3_finalize(s); int p=0; for(const auto& x:input.value("atom_ids").toArray()){check(sqlite3_prepare_v2(db_,"INSERT INTO package_atoms(bundle_id,content_id,position) SELECT ?,?,? WHERE EXISTS(SELECT 1 FROM content_atoms WHERE content_id=?);",-1,&s,nullptr),db_);auto a=canonical_id(x.toString()).toUtf8();sqlite3_bind_text(s,1,u.constData(),-1,SQLITE_TRANSIENT);sqlite3_bind_text(s,2,a.constData(),-1,SQLITE_TRANSIENT);sqlite3_bind_int(s,3,p++);sqlite3_bind_text(s,4,a.constData(),-1,SQLITE_TRANSIENT);check(sqlite3_step(s),db_);sqlite3_finalize(s);} exec(db_,"COMMIT;"); } catch(...) { exec(db_,"ROLLBACK;"); throw; }
}
bool ContentDatabase::delete_package(const QString& id) {
    sqlite3_stmt* statement{};
    check(sqlite3_prepare_v2(db_, "DELETE FROM package_plans WHERE bundle_id=?;", -1, &statement, nullptr), db_);
    const auto utf8 = id.toUtf8();
    sqlite3_bind_text(statement, 1, utf8.constData(), -1, SQLITE_TRANSIENT);
    check(sqlite3_step(statement), db_);
    const bool changed = sqlite3_changes(db_) > 0;
    sqlite3_finalize(statement);
    return changed;
}

void ContentDatabase::import_legacy_workspace(const std::filesystem::path& root) {
    try {
        if (atoms().isEmpty()) {
            for (const auto& folder : {"objects", "atoms"}) {
                QDir directory(QString::fromStdString((root / "catalog" / folder).string()));
                for (const auto& info : directory.entryInfoList({"*.json"}, QDir::Files, QDir::Time | QDir::Reversed)) {
                    QFile file(info.absoluteFilePath());
                    if (file.open(QIODevice::ReadOnly)) {
                        const auto value = parse_object(file.readAll());
                        if (!value.isEmpty()) upsert_atom(value);
                    }
                }
            }
        }

        const auto load_array = [&](const std::filesystem::path& path) {
            QFile file(QString::fromStdString(path.string()));
            if (!file.open(QIODevice::ReadOnly)) return QJsonArray{};
            const auto document = QJsonDocument::fromJson(file.readAll());
            return document.isArray() ? document.array() : QJsonArray{};
        };
        if (relations().isEmpty()) {
            const auto legacy = load_array(root / "catalog" / "relations" / "relations.json");
            if (!legacy.isEmpty()) replace_relations(legacy);
        }
        if (recipes().isEmpty()) {
            const auto legacy = load_array(root / "catalog" / "recipes" / "recipes.json");
            if (!legacy.isEmpty()) replace_recipes(legacy);
        }

        const auto import_package = [&](QJsonObject value) {
            const QString id = value.value(QStringLiteral("bundle_id")).toString();
            if (id.isEmpty() || !package(id).isEmpty()) return;
            if (value.value(QStringLiteral("atom_ids")).toArray().isEmpty()) {
                QJsonArray ids;
                for (const auto& atomValue : atoms()) ids.append(atomValue.toObject().value(QStringLiteral("content_id")));
                value[QStringLiteral("atom_ids")] = ids;
            }
            upsert_package(value);
        };
        const auto import_manifest_file = [&](const std::filesystem::path& path) {
            QFile file(QString::fromStdString(path.string()));
            if (file.open(QIODevice::ReadOnly)) import_package(parse_object(file.readAll()));
        };

        QDir plans(QString::fromStdString((root / "packages").string()));
        for (const auto& info : plans.entryInfoList({"*.json"}, QDir::Files)) import_manifest_file(info.absoluteFilePath().toStdString());
        import_manifest_file(root / "current" / "manifest.json");
        import_manifest_file(root / "manifest.json");

        QDir bundles(QString::fromStdString((root / "bundles").string()));
        for (const auto& directory : bundles.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time)) {
            import_manifest_file(std::filesystem::path(directory.absoluteFilePath().toStdString()) / "manifest.json");
        }
    } catch(...) { throw; }
}

void ContentDatabase::export_workspace(const std::filesystem::path& root) const {
    QDir atomDir(QString::fromStdString((root/"catalog"/"atoms").string())); atomDir.mkpath("."); for(const auto& f:atomDir.entryList({"*.json"},QDir::Files)) atomDir.remove(f);
    for(const auto& v:atoms()){auto o=v.toObject();write_json(atomDir.filePath(o.value("content_id").toString()+".json"),QJsonDocument(o));}
    QDir rel(QString::fromStdString((root/"catalog"/"relations").string()));rel.mkpath(".");write_json(rel.filePath("relations.json"),QJsonDocument(relations()));
    QDir rec(QString::fromStdString((root/"catalog"/"recipes").string()));rec.mkpath(".");write_json(rec.filePath("recipes.json"),QJsonDocument(recipes()));
}
QString ContentDatabase::journal_mode() const { sqlite3_stmt* s{};check(sqlite3_prepare_v2(db_,"PRAGMA journal_mode;",-1,&s,nullptr),db_);QString r;if(sqlite3_step(s)==SQLITE_ROW)r=QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(s,0)));sqlite3_finalize(s);return r; }
} // namespace elo::content
