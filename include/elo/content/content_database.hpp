#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <filesystem>
#include <memory>

struct sqlite3;

namespace elo::content {

/// SQLite/WAL source of truth for the mutable editorial workspace.
/// Published bundles remain immutable JSON snapshots exported from this store.
class ContentDatabase {
public:
    explicit ContentDatabase(const std::filesystem::path& path);
    ~ContentDatabase();
    ContentDatabase(const ContentDatabase&) = delete;
    ContentDatabase& operator=(const ContentDatabase&) = delete;

    [[nodiscard]] bool is_empty() const;
    void import_legacy_workspace(const std::filesystem::path& root);
    void export_workspace(const std::filesystem::path& root) const;

    [[nodiscard]] QJsonArray atoms() const;
    [[nodiscard]] QJsonObject atom(const QString& id) const;
    void upsert_atom(const QJsonObject& value);
    [[nodiscard]] bool delete_atom(const QString& id);

    [[nodiscard]] QJsonArray relations() const;
    void replace_relations(const QJsonArray& values);
    [[nodiscard]] QJsonArray recipes() const;
    void replace_recipes(const QJsonArray& values);

    [[nodiscard]] QJsonArray packages() const;
    [[nodiscard]] QJsonObject package(const QString& id) const;
    void upsert_package(const QJsonObject& value);
    [[nodiscard]] bool delete_package(const QString& id);

    [[nodiscard]] QString journal_mode() const;

private:
    sqlite3* db_{nullptr};
};

} // namespace elo::content
