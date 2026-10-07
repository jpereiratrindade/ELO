#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <cstdint>
#include <fstream>
#include <sstream>

#ifdef ELO_HAS_QT
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#endif

namespace elo::application {

/// @brief Represents a registered decoupled application or domain experience in the ELO ecosystem.
struct ApplicationProfile {
    std::string app_id;               // e.g. "app.elo.bioma-pampa", "app.elo.arte-contemporanea", "app.elo.patrimonio-historico"
    std::string name;                 // e.g. "Biodiversidade do Pampa Gaúcho"
    std::string domain_category;      // e.g. "environmental_sciences", "cultural_heritage", "fine_arts", "civic_engagement"
    std::string version;              // e.g. "1.2.0"
    std::string description;          // Curatorial purpose of this application
    std::string active_bundle_id;     // Linked Content Bundle ID
    std::string target_audience{"general"}; // e.g. "general", "students", "researchers"
    std::string default_theme{"default"};   // Theme styling preference
    std::vector<std::string> tags;    // Categorical tags
    std::unordered_map<std::string, std::string> metadata_schema; // Extensible key-value metadata
    bool is_active{false};            // Whether this application is currently active on the totem
    bool telemetry_enabled{true};     // Anonymized analytics collection toggle
    std::uint64_t registered_at{0};   // Epoch timestamp
};

/// @brief Registry for managing multi-domain applications decoupled from the platform core.
class ApplicationRegistry {
public:
    ApplicationRegistry() = default;

    void register_application(ApplicationProfile profile) {
        if (profile.registered_at == 0) {
            profile.registered_at = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
        }
        if (profile.is_active) {
            for (auto& [_, app] : applications_) {
                app.is_active = false;
            }
        }
        applications_[profile.app_id] = std::move(profile);
    }

    [[nodiscard]] std::optional<ApplicationProfile> find(const std::string& app_id) const {
        auto it = applications_.find(app_id);
        if (it != applications_.end()) return it->second;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<ApplicationProfile> get_active() const {
        for (const auto& [_, app] : applications_) {
            if (app.is_active) return app;
        }
        if (!applications_.empty()) return applications_.begin()->second;
        return std::nullopt;
    }

    bool activate(const std::string& app_id) {
        auto it = applications_.find(app_id);
        if (it == applications_.end()) return false;
        for (auto& [_, app] : applications_) {
            app.is_active = (app.app_id == app_id);
        }
        return true;
    }

    [[nodiscard]] std::vector<ApplicationProfile> list_all() const {
        std::vector<ApplicationProfile> list;
        list.reserve(applications_.size());
        for (const auto& [_, app] : applications_) {
            list.push_back(app);
        }
        return list;
    }

    bool remove(const std::string& app_id) {
        return applications_.erase(app_id) > 0;
    }

    bool load_from_file(const std::string& file_path) {
#ifdef ELO_HAS_QT
        QFile file(QString::fromStdString(file_path));
        if (!file.open(QIODevice::ReadOnly)) return false;
        auto doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isArray()) return false;

        applications_.clear();
        for (const auto& val : doc.array()) {
            if (!val.isObject()) continue;
            auto obj = val.toObject();
            ApplicationProfile p;
            p.app_id = obj.value(QStringLiteral("app_id")).toString().toStdString();
            p.name = obj.value(QStringLiteral("name")).toString().toStdString();
            p.domain_category = obj.value(QStringLiteral("domain_category")).toString().toStdString();
            p.version = obj.value(QStringLiteral("version")).toString(QStringLiteral("1.0.0")).toStdString();
            p.description = obj.value(QStringLiteral("description")).toString().toStdString();
            p.active_bundle_id = obj.value(QStringLiteral("active_bundle_id")).toString().toStdString();
            p.target_audience = obj.value(QStringLiteral("target_audience")).toString(QStringLiteral("general")).toStdString();
            p.default_theme = obj.value(QStringLiteral("default_theme")).toString(QStringLiteral("default")).toStdString();
            p.is_active = obj.value(QStringLiteral("is_active")).toBool(false);
            p.telemetry_enabled = obj.value(QStringLiteral("telemetry_enabled")).toBool(true);
            p.registered_at = static_cast<std::uint64_t>(obj.value(QStringLiteral("registered_at")).toInteger(0));

            for (const auto& t : obj.value(QStringLiteral("tags")).toArray()) {
                p.tags.push_back(t.toString().toStdString());
            }
            auto metaObj = obj.value(QStringLiteral("metadata_schema")).toObject();
            for (auto it = metaObj.begin(); it != metaObj.end(); ++it) {
                p.metadata_schema[it.key().toStdString()] = it.value().toString().toStdString();
            }
            applications_[p.app_id] = std::move(p);
        }
        return true;
#else
        return false;
#endif
    }

    bool save_to_file(const std::string& file_path) const {
#ifdef ELO_HAS_QT
        QJsonArray arr;
        for (const auto& [_, app] : applications_) {
            QJsonObject obj;
            obj[QStringLiteral("app_id")] = QString::fromStdString(app.app_id);
            obj[QStringLiteral("name")] = QString::fromStdString(app.name);
            obj[QStringLiteral("domain_category")] = QString::fromStdString(app.domain_category);
            obj[QStringLiteral("version")] = QString::fromStdString(app.version);
            obj[QStringLiteral("description")] = QString::fromStdString(app.description);
            obj[QStringLiteral("active_bundle_id")] = QString::fromStdString(app.active_bundle_id);
            obj[QStringLiteral("target_audience")] = QString::fromStdString(app.target_audience);
            obj[QStringLiteral("default_theme")] = QString::fromStdString(app.default_theme);
            obj[QStringLiteral("is_active")] = app.is_active;
            obj[QStringLiteral("telemetry_enabled")] = app.telemetry_enabled;
            obj[QStringLiteral("registered_at")] = static_cast<qint64>(app.registered_at);

            QJsonArray tagsArr;
            for (const auto& t : app.tags) tagsArr.append(QString::fromStdString(t));
            obj[QStringLiteral("tags")] = tagsArr;

            QJsonObject metaObj;
            for (const auto& [k, v] : app.metadata_schema) {
                metaObj[QString::fromStdString(k)] = QString::fromStdString(v);
            }
            obj[QStringLiteral("metadata_schema")] = metaObj;
            arr.append(obj);
        }

        QFile file(QString::fromStdString(file_path));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        return true;
#else
        return false;
#endif
    }

private:
    std::unordered_map<std::string, ApplicationProfile> applications_;
};

} // namespace elo::application
