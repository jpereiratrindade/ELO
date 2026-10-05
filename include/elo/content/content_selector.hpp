#pragma once

#include "elo/content/content_catalog.hpp"
#include <unordered_set>
#include <random>

namespace elo::content {

/// @brief Runtime context supplied by the ExperienceEngine
struct SelectionContext {
    ContentRole target_role{ContentRole::Ambient};
    std::string theme_filter{};
    std::unordered_set<std::string> seen_content_ids{};
    std::string current_focus_id{};
    bool audio_supported{true};
    std::uint32_t session_depth{1};
};

/// @brief ELO-CONTENT-001 Section 51 & 56: Explainable Contextual Content Selector
class ContentSelector {
public:
    explicit ContentSelector(std::uint64_t seed = 42) : rng_{seed} {}

    struct SelectionResult {
        const ContentAtom* atom{nullptr};
        const ContentVariant* variant{nullptr};
        SelectionReason reason{};
    };

    /// @brief Selects the next best content atom & variant according to context
    [[nodiscard]] std::optional<SelectionResult> select(
        const ContentCatalog& catalog,
        const SelectionContext& context);

private:
    std::mt19937_64 rng_;
};

} // namespace elo::content
