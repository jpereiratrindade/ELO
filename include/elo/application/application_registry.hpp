#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <cstdint>

namespace elo::application {

/// @brief Represents a registered decoupled application or domain experience in the ELO ecosystem.
struct ApplicationProfile {
    std::string app_id;               // e.g. "app.elo.bioma-pampa", "app.elo.arte-contemporanea", "app.elo.patrimonio-historico"
    std::string name;                 // e.g. "Biodiversidade do Pampa Gaúcho"
    std::string domain_category;      // e.g. "environmental_sciences", "cultural_heritage", "fine_arts", "civic_engagement"
    std::string version;              // e.g. "1.2.0"
    std::string description;          // Curatorial purpose of this application
    std::string active_bundle_id;     // Linked Content Bundle ID
    std::vector<std::string> tags;    // Categorical tags
    std::unordered_map<std::string, std::string> metadata_schema; // Extensible key-value metadata
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
        applications_[profile.app_id] = std::move(profile);
    }

    [[nodiscard]] std::optional<ApplicationProfile> find(const std::string& app_id) const {
        auto it = applications_.find(app_id);
        if (it != applications_.end()) return it->second;
        return std::nullopt;
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

private:
    std::unordered_map<std::string, ApplicationProfile> applications_;
};

} // namespace elo::application
