#pragma once

#include "elo/content/content_catalog.hpp"
#include "elo/content/content_validator.hpp"
#include "elo/core/result.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace elo::content {

/// @brief ELO-ADMIN-001 Section 4: Editorial States
enum class EditorialState {
    Draft,
    Valid,
    Previewed,
    Published,
    Active,
    Superseded
};

[[nodiscard]] constexpr std::string_view to_string(EditorialState state) noexcept {
    switch (state) {
        case EditorialState::Draft: return "draft";
        case EditorialState::Valid: return "valid";
        case EditorialState::Previewed: return "previewed";
        case EditorialState::Published: return "published";
        case EditorialState::Active: return "active";
        case EditorialState::Superseded: return "superseded";
    }
    return "draft";
}

struct BundlePublishResult {
    bool success{false};
    std::string bundle_id{};
    std::string version{};
    std::string content_hash{};
    std::filesystem::path bundle_path{};
    std::filesystem::path current_link{};
    std::string error_message{};
};

/// @brief ELO-PUBLISHING-001 Section 1: Immutable Content Bundle
class ContentBundle {
public:
    explicit ContentBundle(std::filesystem::path root_dir);

    [[nodiscard]] const std::filesystem::path& root_path() const noexcept { return root_dir_; }
    [[nodiscard]] std::filesystem::path manifest_path() const { return root_dir_ / "manifest.json"; }
    [[nodiscard]] std::filesystem::path catalog_path() const { return root_dir_ / "catalog"; }
    [[nodiscard]] std::filesystem::path assets_path() const { return root_dir_ / "assets"; }

    [[nodiscard]] core::Result<BundleManifest> load_manifest() const;
    [[nodiscard]] core::Result<ContentCatalog> load_catalog() const;

    /// @brief Computes SHA-256 hash deterministically over catalog and assets
    [[nodiscard]] core::Result<std::string> compute_content_hash() const;

    /// @brief Validates bundle using native ContentValidator and asset integrity
    [[nodiscard]] ValidationReport validate() const;

private:
    std::filesystem::path root_dir_;
};

/// @brief ELO-PUBLISHING-001 Section 4 & 5: Bundle Publisher and Atomic Activation
class BundlePublisher {
public:
    explicit BundlePublisher(std::filesystem::path content_root_dir);

    [[nodiscard]] const std::filesystem::path& content_root() const noexcept { return root_dir_; }
    [[nodiscard]] std::filesystem::path bundles_dir() const { return root_dir_ / "bundles"; }
    [[nodiscard]] std::filesystem::path staging_dir() const { return root_dir_ / "staging"; }
    [[nodiscard]] std::filesystem::path current_symlink() const { return root_dir_ / "current"; }

    /// @brief Lists all published bundles in bundles/
    [[nodiscard]] std::vector<BundleManifest> list_bundles() const;

    /// @brief Returns manifest of current active bundle (following 'current' symlink)
    [[nodiscard]] std::optional<BundleManifest> active_bundle() const;

    /// @brief Packages, validates, hashes, stores and atomically activates a bundle
    [[nodiscard]] BundlePublishResult publish_and_activate(
        const std::filesystem::path& candidate_dir,
        std::string_view creator = "admin");

    /// @brief Atomically rolls back 'current' to an existing bundle in bundles/
    [[nodiscard]] core::Result<void> rollback_to(const std::string& bundle_dir_or_version);

    /// @brief Performs atomic symlink swap: current -> target_bundle_path
    static core::Result<void> atomic_activate(
        const std::filesystem::path& current_link,
        const std::filesystem::path& target_bundle_path);

private:
    std::filesystem::path root_dir_;
};

/// @brief Resolves sovereign Linux system content directory according to FHS / XDG.
/// Precedence:
/// 1. ELO_CONTENT_DIR or ELO_CONTENT_ROOT if set in environment
/// 2. /var/lib/elo/content if writable (production system kiosk)
/// 3. $XDG_DATA_HOME/elo/content (default: ~/.local/share/elo/content)
/// If bootstrap_from_seed is true and the resolved directory is empty, seeds it from the factory template.
[[nodiscard]] std::filesystem::path resolve_system_content_dir(bool bootstrap_from_seed = true);

} // namespace elo::content
