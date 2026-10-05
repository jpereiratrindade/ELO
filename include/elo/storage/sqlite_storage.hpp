#pragma once

#include "elo/core/result.hpp"
#include "elo/storage/storage_interfaces.hpp"

#include <filesystem>
#include <memory>

namespace elo::storage {

struct LocalStores {
    std::shared_ptr<IBiometricStore> biometric;
    std::shared_ptr<IExperienceStore> experience;
    std::shared_ptr<ISurveyStore> survey;
    std::shared_ptr<IJevEventStore> jev_events;
};

/// @brief Opens the production offline store. All three repositories share a
/// transactional SQLite database while preserving their domain interfaces.
[[nodiscard]] core::Result<LocalStores> open_sqlite_stores(
    const std::filesystem::path& database_path);

} // namespace elo::storage
