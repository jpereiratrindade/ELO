#include "elo/storage/sqlite_storage.hpp"

#include <sqlite3.h>

#include <cstddef>
#include <cstring>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace elo::storage {
namespace {

class SqliteFailure final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Statement final {
public:
    Statement(sqlite3* database, const char* sql) {
        if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) != SQLITE_OK) {
            throw SqliteFailure(sqlite3_errmsg(database));
        }
    }

    ~Statement() {
        sqlite3_finalize(statement_);
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    [[nodiscard]] sqlite3_stmt* get() const noexcept { return statement_; }

private:
    sqlite3_stmt* statement_{nullptr};
};

class Database final {
public:
    explicit Database(const std::filesystem::path& path) {
        const int result = sqlite3_open_v2(
            path.c_str(), &database_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
            nullptr);
        if (result != SQLITE_OK) {
            const std::string message = database_ != nullptr
                ? sqlite3_errmsg(database_)
                : "SQLite could not allocate a database handle";
            if (database_ != nullptr) {
                sqlite3_close(database_);
                database_ = nullptr;
            }
            throw SqliteFailure(message);
        }

        execute("PRAGMA journal_mode=WAL;");
        execute("PRAGMA synchronous=FULL;");
        execute("PRAGMA foreign_keys=ON;");
        execute(
            "CREATE TABLE IF NOT EXISTS biometric_templates ("
            " template_id TEXT PRIMARY KEY,"
            " person_id TEXT NOT NULL,"
            " model_id TEXT NOT NULL,"
            " model_version TEXT NOT NULL,"
            " representation BLOB NOT NULL,"
            " quality REAL NOT NULL,"
            " created_at INTEGER NOT NULL,"
            " integrity_digest TEXT NOT NULL"
            ");"
            "CREATE INDEX IF NOT EXISTS idx_biometric_person "
            " ON biometric_templates(person_id);"
            "CREATE TABLE IF NOT EXISTS experience_history ("
            " sequence INTEGER PRIMARY KEY AUTOINCREMENT,"
            " person_id TEXT NOT NULL,"
            " content_id TEXT NOT NULL"
            ");"
            "CREATE INDEX IF NOT EXISTS idx_experience_person "
            " ON experience_history(person_id, sequence);"
            "CREATE TABLE IF NOT EXISTS survey_responses ("
            " sequence INTEGER PRIMARY KEY AUTOINCREMENT,"
            " question_id TEXT NOT NULL,"
            " selected_option TEXT NOT NULL,"
            " timestamp INTEGER NOT NULL,"
            " linkage_policy INTEGER NOT NULL,"
            " person_id TEXT NULL"
            ");"
            "CREATE INDEX IF NOT EXISTS idx_survey_person "
            " ON survey_responses(person_id);"
            "CREATE TABLE IF NOT EXISTS jev_events ("
            " sequence INTEGER PRIMARY KEY AUTOINCREMENT,"
            " timestamp INTEGER NOT NULL,"
            " session_id TEXT NOT NULL,"
            " event_name TEXT NOT NULL,"
            " payload TEXT NOT NULL"
            ");"
            "CREATE INDEX IF NOT EXISTS idx_jev_session "
            " ON jev_events(session_id, sequence);"
        );
    }

    ~Database() {
        if (database_ != nullptr) {
            sqlite3_close(database_);
        }
    }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    [[nodiscard]] sqlite3* handle() const noexcept { return database_; }
    [[nodiscard]] std::mutex& mutex() noexcept { return mutex_; }

    void execute(const char* sql) {
        char* error = nullptr;
        if (sqlite3_exec(database_, sql, nullptr, nullptr, &error) != SQLITE_OK) {
            const std::string message = error != nullptr ? error : sqlite3_errmsg(database_);
            sqlite3_free(error);
            throw SqliteFailure(message);
        }
    }

private:
    sqlite3* database_{nullptr};
    std::mutex mutex_;
};

void bind_text(sqlite3_stmt* statement, int index, const std::string& value) {
    if (sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        throw SqliteFailure("Failed to bind SQLite text value");
    }
}

void expect_done(sqlite3* database, sqlite3_stmt* statement) {
    if (sqlite3_step(statement) != SQLITE_DONE) {
        throw SqliteFailure(sqlite3_errmsg(database));
    }
}

core::Error storage_error(core::ErrorCode code, std::string_view action, const std::exception& error) {
    return core::make_error(code, action, error.what());
}

class SqliteBiometricStore final : public IBiometricStore {
public:
    explicit SqliteBiometricStore(std::shared_ptr<Database> database)
        : database_{std::move(database)} {}

    core::Result<void> save_template(const biometric::FaceTemplate& value) override {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(),
                "INSERT INTO biometric_templates "
                "(template_id, person_id, model_id, model_version, representation, quality, "
                " created_at, integrity_digest) VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
                "ON CONFLICT(template_id) DO UPDATE SET "
                " person_id = excluded.person_id, model_id = excluded.model_id,"
                " model_version = excluded.model_version, representation = excluded.representation,"
                " quality = excluded.quality, created_at = excluded.created_at,"
                " integrity_digest = excluded.integrity_digest;");
            bind_text(statement.get(), 1, value.template_id);
            bind_text(statement.get(), 2, value.person_local_id.str());
            bind_text(statement.get(), 3, value.model_id);
            bind_text(statement.get(), 4, value.model_version);
            const auto bytes = static_cast<int>(value.representation.size() * sizeof(float));
            sqlite3_bind_blob(statement.get(), 5, value.representation.data(), bytes, SQLITE_TRANSIENT);
            sqlite3_bind_double(statement.get(), 6, value.quality);
            sqlite3_bind_int64(statement.get(), 7, static_cast<sqlite3_int64>(value.created_at));
            bind_text(statement.get(), 8, value.integrity_digest);
            expect_done(database_->handle(), statement.get());
            return {};
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::BiometricStoreError, "Failed to persist biometric template", error));
        }
    }

    core::Result<std::vector<biometric::FaceTemplate>> get_all_templates() const override {
        return query_templates(std::nullopt);
    }

    core::Result<std::vector<biometric::FaceTemplate>> get_templates_for(
        const identity::PersonLocalId& person_id) const override {
        return query_templates(person_id.str());
    }

    core::Result<bool> forget_person(const identity::PersonLocalId& person_id) override {
        return delete_person("DELETE FROM biometric_templates WHERE person_id = ?;", person_id);
    }

    core::Result<void> clear() override {
        try {
            std::lock_guard lock(database_->mutex());
            database_->execute("DELETE FROM biometric_templates;");
            return {};
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::BiometricStoreError, "Failed to clear biometric templates", error));
        }
    }

private:
    core::Result<std::vector<biometric::FaceTemplate>> query_templates(
        const std::optional<std::string>& person_id) const {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(), person_id
                ? "SELECT template_id, person_id, model_id, model_version, representation, quality, "
                  "created_at, integrity_digest FROM biometric_templates WHERE person_id = ?;"
                : "SELECT template_id, person_id, model_id, model_version, representation, quality, "
                  "created_at, integrity_digest FROM biometric_templates;");
            if (person_id) {
                bind_text(statement.get(), 1, *person_id);
            }

            std::vector<biometric::FaceTemplate> templates;
            int result = SQLITE_OK;
            while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
                const auto* blob = static_cast<const float*>(sqlite3_column_blob(statement.get(), 4));
                const int blob_bytes = sqlite3_column_bytes(statement.get(), 4);
                if (blob_bytes < 0 || blob_bytes % static_cast<int>(sizeof(float)) != 0) {
                    throw SqliteFailure("Corrupt biometric representation");
                }
                std::vector<float> representation(static_cast<std::size_t>(blob_bytes) / sizeof(float));
                if (blob_bytes > 0) {
                    std::memcpy(representation.data(), blob, static_cast<std::size_t>(blob_bytes));
                }

                templates.push_back(biometric::FaceTemplate{
                    .template_id = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 0)),
                    .person_local_id = identity::PersonLocalId(
                        reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1))),
                    .model_id = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 2)),
                    .model_version = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 3)),
                    .representation = std::move(representation),
                    .quality = sqlite3_column_double(statement.get(), 5),
                    .created_at = static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(), 6)),
                    .integrity_digest = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 7))
                });
            }
            if (result != SQLITE_DONE) {
                throw SqliteFailure(sqlite3_errmsg(database_->handle()));
            }
            return templates;
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::BiometricStoreError, "Failed to read biometric templates", error));
        }
    }

    core::Result<bool> delete_person(const char* sql, const identity::PersonLocalId& person_id) {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(), sql);
            bind_text(statement.get(), 1, person_id.str());
            expect_done(database_->handle(), statement.get());
            return sqlite3_changes(database_->handle()) > 0;
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::BiometricStoreError, "Failed to forget biometric identity", error));
        }
    }

    std::shared_ptr<Database> database_;
};

class SqliteExperienceStore final : public IExperienceStore {
public:
    explicit SqliteExperienceStore(std::shared_ptr<Database> database)
        : database_{std::move(database)} {}

    core::Result<void> record_content_served(
        const identity::PersonLocalId& person_id, const std::string& content_id) override {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(),
                "INSERT INTO experience_history (person_id, content_id) VALUES (?, ?);");
            bind_text(statement.get(), 1, person_id.str());
            bind_text(statement.get(), 2, content_id);
            expect_done(database_->handle(), statement.get());
            return {};
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::ExperienceStoreError, "Failed to persist experience history", error));
        }
    }

    core::Result<std::vector<std::string>> get_history(
        const identity::PersonLocalId& person_id) const override {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(),
                "SELECT content_id FROM experience_history WHERE person_id = ? ORDER BY sequence;");
            bind_text(statement.get(), 1, person_id.str());
            std::vector<std::string> history;
            int result = SQLITE_OK;
            while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
                history.emplace_back(
                    reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 0)));
            }
            if (result != SQLITE_DONE) {
                throw SqliteFailure(sqlite3_errmsg(database_->handle()));
            }
            return history;
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::ExperienceStoreError, "Failed to read experience history", error));
        }
    }

    core::Result<bool> forget_person(const identity::PersonLocalId& person_id) override {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(),
                "DELETE FROM experience_history WHERE person_id = ?;");
            bind_text(statement.get(), 1, person_id.str());
            expect_done(database_->handle(), statement.get());
            return sqlite3_changes(database_->handle()) > 0;
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::ExperienceStoreError, "Failed to forget experience history", error));
        }
    }

    core::Result<void> clear() override {
        try {
            std::lock_guard lock(database_->mutex());
            database_->execute("DELETE FROM experience_history;");
            return {};
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::ExperienceStoreError, "Failed to clear experience history", error));
        }
    }

private:
    std::shared_ptr<Database> database_;
};

class SqliteSurveyStore final : public ISurveyStore {
public:
    explicit SqliteSurveyStore(std::shared_ptr<Database> database)
        : database_{std::move(database)} {}

    core::Result<void> record_response(const survey::SurveyResponse& response) override {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(),
                "INSERT INTO survey_responses "
                "(question_id, selected_option, timestamp, linkage_policy, person_id) "
                "VALUES (?, ?, ?, ?, ?);");
            bind_text(statement.get(), 1, response.question_id);
            bind_text(statement.get(), 2, response.selected_option);
            sqlite3_bind_int64(statement.get(), 3, static_cast<sqlite3_int64>(response.timestamp));
            sqlite3_bind_int(statement.get(), 4, static_cast<int>(response.policy));
            if (response.linked_person) {
                bind_text(statement.get(), 5, response.linked_person->str());
            } else {
                sqlite3_bind_null(statement.get(), 5);
            }
            expect_done(database_->handle(), statement.get());
            return {};
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::SubstrateFailure, "Failed to persist survey response", error));
        }
    }

    core::Result<std::vector<survey::SurveyResponse>> get_all_responses() const override {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(),
                "SELECT question_id, selected_option, timestamp, linkage_policy, person_id "
                "FROM survey_responses ORDER BY sequence;");
            std::vector<survey::SurveyResponse> responses;
            int result = SQLITE_OK;
            while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
                std::optional<identity::PersonLocalId> person;
                if (sqlite3_column_type(statement.get(), 4) != SQLITE_NULL) {
                    person.emplace(reinterpret_cast<const char*>(
                        sqlite3_column_text(statement.get(), 4)));
                }
                responses.push_back(survey::SurveyResponse{
                    .question_id = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 0)),
                    .selected_option = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1)),
                    .timestamp = static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(), 2)),
                    .policy = static_cast<survey::LinkagePolicy>(sqlite3_column_int(statement.get(), 3)),
                    .linked_person = std::move(person)
                });
            }
            if (result != SQLITE_DONE) {
                throw SqliteFailure(sqlite3_errmsg(database_->handle()));
            }
            return responses;
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::SubstrateFailure, "Failed to read survey responses", error));
        }
    }

    core::Result<bool> forget_person(const identity::PersonLocalId& person_id) override {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(),
                "UPDATE survey_responses SET person_id = NULL, linkage_policy = ? WHERE person_id = ?;");
            sqlite3_bind_int(statement.get(), 1,
                static_cast<int>(survey::LinkagePolicy::UnlinkedAnonymous));
            bind_text(statement.get(), 2, person_id.str());
            expect_done(database_->handle(), statement.get());
            return sqlite3_changes(database_->handle()) > 0;
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::SubstrateFailure, "Failed to unlink survey responses", error));
        }
    }

    core::Result<void> clear() override {
        try {
            std::lock_guard lock(database_->mutex());
            database_->execute("DELETE FROM survey_responses;");
            return {};
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::SubstrateFailure, "Failed to clear survey responses", error));
        }
    }

private:
    std::shared_ptr<Database> database_;
};

class SqliteJevEventStore final : public IJevEventStore {
public:
    explicit SqliteJevEventStore(std::shared_ptr<Database> database)
        : database_{std::move(database)} {}

    core::Result<void> record_event(const judgment::JevEvent& event) override {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(),
                "INSERT INTO jev_events (timestamp, session_id, event_name, payload) "
                "VALUES (?, ?, ?, ?);");
            sqlite3_bind_int64(statement.get(), 1, static_cast<sqlite3_int64>(event.timestamp_ms));
            bind_text(statement.get(), 2, event.session_id);
            bind_text(statement.get(), 3, event.event_name);
            bind_text(statement.get(), 4, event.payload);
            expect_done(database_->handle(), statement.get());
            return {};
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::SubstrateFailure, "Failed to record JEV event", error));
        }
    }

    core::Result<std::vector<judgment::JevEvent>> get_events_for_session(
        const std::string& session_id) const override {
        return query_events("SELECT sequence, timestamp, session_id, event_name, payload "
                            "FROM jev_events WHERE session_id = ? ORDER BY sequence ASC;",
                            &session_id);
    }

    core::Result<std::vector<judgment::JevEvent>> get_all_events() const override {
        return query_events("SELECT sequence, timestamp, session_id, event_name, payload "
                            "FROM jev_events ORDER BY sequence ASC;",
                            nullptr);
    }

    core::Result<void> clear() override {
        try {
            std::lock_guard lock(database_->mutex());
            database_->execute("DELETE FROM jev_events;");
            return {};
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::SubstrateFailure, "Failed to clear JEV events", error));
        }
    }

private:
    std::shared_ptr<Database> database_;

    core::Result<std::vector<judgment::JevEvent>> query_events(
        const char* sql, const std::string* session_id) const {
        try {
            std::lock_guard lock(database_->mutex());
            Statement statement(database_->handle(), sql);
            if (session_id) {
                bind_text(statement.get(), 1, *session_id);
            }
            std::vector<judgment::JevEvent> events;
            int result = SQLITE_OK;
            while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
                events.push_back(judgment::JevEvent{
                    .event_name = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 3)),
                    .timestamp_ms = static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(), 1)),
                    .session_id = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 2)),
                    .payload = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 4)),
                    .sequence = static_cast<std::uint64_t>(sqlite3_column_int64(statement.get(), 0))
                });
            }
            if (result != SQLITE_DONE) {
                throw SqliteFailure(sqlite3_errmsg(database_->handle()));
            }
            return events;
        } catch (const std::exception& error) {
            return std::unexpected(storage_error(
                core::ErrorCode::SubstrateFailure, "Failed to query JEV events", error));
        }
    }
};

} // namespace

core::Result<LocalStores> open_sqlite_stores(const std::filesystem::path& database_path) {
    try {
        auto database = std::make_shared<Database>(database_path);
        return LocalStores{
            .biometric = std::make_shared<SqliteBiometricStore>(database),
            .experience = std::make_shared<SqliteExperienceStore>(database),
            .survey = std::make_shared<SqliteSurveyStore>(database),
            .jev_events = std::make_shared<SqliteJevEventStore>(database)
        };
    } catch (const std::exception& error) {
        return std::unexpected(core::make_error(
            core::ErrorCode::SubstrateFailure,
            "Failed to open local SQLite storage",
            error.what()));
    }
}

} // namespace elo::storage
