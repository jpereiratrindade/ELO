#include "elo/content/content_database.hpp"

#include <QJsonArray>
#include <QJsonObject>
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

    QJsonObject package{{"bundle_id", "package-test"}, {"title", "Package"},
                        {"atom_ids", QJsonArray{QStringLiteral("Resiliência")}}};
    database.upsert_package(package);
    CHECK(database.packages().size() == 1, "package plan persisted");

    database.export_workspace(root);
    CHECK(std::filesystem::exists(root / "catalog" / "atoms" / "Resiliência.json"), "JSON snapshot exported");
    CHECK(database.delete_atom("Resili%C3%AAncia"), "canonical atom deleted");
    CHECK(database.atoms().isEmpty(), "atom store empty after delete");

    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::cout << "SQLite/WAL editorial content database tests passed.\n";
    return 0;
}
