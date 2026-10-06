#pragma once

#include "elo/content/content_types.hpp"
#include "elo/core/result.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace elo::content {

/// @brief ELO-CONTENT-001 Section 51 & 52: Local Offline Content Catalog
/// Pure C++26 domain model without Qt coupling in public interfaces.
class ContentCatalog {
public:
    ContentCatalog() = default;

    /// @brief Loads a decoupled content tree (e.g. content/catalog/)
    [[nodiscard]] core::Result<void> load_from_directory(const std::filesystem::path& catalog_dir);

    // Direct insertion for programmatically configured or tested content
    void add_atom(ContentAtom atom);
    void add_relation(ContentRelation relation);
    void add_variant(ContentVariant variant);
    void add_recipe(ExperienceRecipe recipe);

    // Queries
    [[nodiscard]] const ContentAtom* find_atom(const std::string& content_id) const noexcept;
    [[nodiscard]] const ContentAtom* find_atom_by_name(std::string_view name) const noexcept;
    [[nodiscard]] std::vector<const ContentAtom*> all_atoms() const;
    [[nodiscard]] std::vector<const ContentAtom*> find_by_type(ContentType type) const;
    [[nodiscard]] std::vector<const ContentAtom*> find_by_theme(std::string_view theme) const;
    [[nodiscard]] std::vector<const ContentAtom*> find_by_role(ContentRole role) const;

    [[nodiscard]] std::vector<const ContentRelation*> find_relations_from(const std::string& content_id) const;
    [[nodiscard]] std::vector<const ContentRelation*> find_relations_to(const std::string& content_id) const;

    [[nodiscard]] std::vector<const ContentVariant*> find_variants_for(const std::string& content_id) const;
    [[nodiscard]] const ContentVariant* find_variant(const std::string& variant_id) const noexcept;

    [[nodiscard]] const ExperienceRecipe* find_recipe(const std::string& recipe_id) const noexcept;
    [[nodiscard]] std::vector<const ExperienceRecipe*> all_recipes() const;

    [[nodiscard]] const BundleManifest& manifest() const noexcept { return manifest_; }
    [[nodiscard]] std::string_view default_theme() const noexcept { return manifest_.default_theme; }
    void set_manifest(BundleManifest manifest) noexcept { manifest_ = std::move(manifest); }

    [[nodiscard]] std::size_t atom_count() const noexcept { return atoms_.size(); }
    [[nodiscard]] std::size_t relation_count() const noexcept { return relations_.size(); }
    [[nodiscard]] std::size_t variant_count() const noexcept { return variants_.size(); }
    [[nodiscard]] std::size_t recipe_count() const noexcept { return recipes_.size(); }

    void clear() noexcept;

private:
    BundleManifest manifest_{};
    std::unordered_map<std::string, ContentAtom> atoms_{};
    std::vector<ContentRelation> relations_{};
    std::unordered_map<std::string, ContentVariant> variants_{};
    std::unordered_map<std::string, ExperienceRecipe> recipes_{};

    // Fast indices
    std::unordered_map<std::string, std::vector<std::size_t>> relations_by_from_{};
    std::unordered_map<std::string, std::vector<std::size_t>> relations_by_to_{};
    std::unordered_map<std::string, std::vector<std::string>> variants_by_content_id_{};
};

} // namespace elo::content
