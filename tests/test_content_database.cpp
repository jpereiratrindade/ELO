#include "elo/content/content_database.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <chrono>
#include <filesystem>
#include <iostream>

#define CHECK(value, message) do { if (!(value)) { std::cerr << "FAILED: " << message << '\n'; return 1; } } while (0)

int main() {
    const auto root = std::filesystem::temp_directory_path() /
        ("elo_content_db_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);

    elo::content::ContentDatabase database(root / "editorial.sqlite3");
    CHECK(database.journal_mode().compare(QStringLiteral("wal"), Qt::CaseInsensitive) == 0, "WAL mode enabled");

    QJsonObject atom{{"content_id", "Resiliência"}, {"type", "concept"}, {"title", "Original"}};
    database.upsert_atom(atom);
    atom["title"] = "Alterado";
    database.upsert_atom(atom);
    CHECK(database.atoms().size() == 1, "upsert does not duplicate accented id");
    CHECK(database.atom("Resili%C3%AAncia").value("title").toString() == "Alterado", "encoded id resolves canonically");

    std::filesystem::create_directories(root / "current");
    QFile activeManifest(QString::fromStdString((root / "current" / "manifest.json").string()));
    CHECK(activeManifest.open(QIODevice::WriteOnly), "active legacy manifest opens");
    activeManifest.write(QJsonDocument(QJsonObject{{"bundle_id", "active-package"}, {"title", "Active package"}, {"version", "1.0.0"}}).toJson());
    activeManifest.close();
    database.import_legacy_workspace(root);
    CHECK(!database.package("active-package").isEmpty(), "incremental migration recovers active package when atoms already exist");

    QJsonObject package{{"bundle_id", "package-test"}, {"title", "Package"},
                        {"atom_ids", QJsonArray{QStringLiteral("Resiliência")}}};
    database.upsert_package(package);
    CHECK(database.packages().size() == 2, "package plan persisted alongside recovered active package");

    database.export_workspace(root);
    CHECK(std::filesystem::exists(root / "catalog" / "atoms" / "Resiliência.json"), "JSON snapshot exported");
    CHECK(database.delete_atom("Resili%C3%AAncia"), "canonical atom deleted");
    CHECK(database.atoms().isEmpty(), "atom store empty after delete");

    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::cout << "SQLite/WAL editorial content database tests passed.\n";
    return 0;
}
