#pragma once

#include "elo/content/content_types.hpp"
#include <string>
#include <vector>
#include <optional>

namespace elo::content {

/// @brief State of an active recipe execution
struct RecipeSessionState {
    std::string recipe_id;
    std::string content_id;
    std::size_t current_step_index{0};
    bool completed{false};
    std::optional<std::string> user_response{std::nullopt};
    bool user_was_correct{false};
};

/// @brief ELO-CONTENT-001 Section 44 & 53: Recipe Executor
/// Steps through experiential recipes and generates presentation actions for the UI
class RecipeExecutor {
public:
    RecipeExecutor() = default;

    /// @brief Starts a recipe with the selected content atom and recipe definition
    [[nodiscard]] RecipeSessionState start(
        const ExperienceRecipe& recipe,
        const ContentAtom& atom);

    /// @brief Generates the next presentation action based on current recipe step
    [[nodiscard]] std::vector<PresentationAction> evaluate_step(
        const ExperienceRecipe& recipe,
        const ContentAtom& atom,
        const RecipeSessionState& state) const;

    /// @brief Ingests an interaction (e.g. user choice) and advances recipe state
    [[nodiscard]] RecipeSessionState advance(
        const ExperienceRecipe& recipe,
        const ContentAtom& atom,
        RecipeSessionState current_state,
        std::string_view user_action);
};

} // namespace elo::content
