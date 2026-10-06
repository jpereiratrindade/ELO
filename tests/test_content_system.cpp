#include "elo/content/content_catalog.hpp"
#include "elo/content/content_validator.hpp"
#include "elo/content/content_selector.hpp"
#include "elo/content/recipe_executor.hpp"

#include <cassert>
#include <iostream>
#include <filesystem>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg << " (" << #cond << ")\n"; \
            std::abort(); \
        } \
    } while (0)

static std::filesystem::path resolve_content_dir() {
    for (const auto& candidate : {"content/catalog", "../content/catalog", "../../content/catalog"}) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return "content/catalog";
}

void test_content_catalog_and_validation() {
    std::cout << "[TEST] ContentCatalog and ContentValidator...\n";
    elo::content::ContentCatalog catalog;

    std::filesystem::path content_dir = resolve_content_dir();
    auto load_res = catalog.load_from_directory(content_dir);
    if (!load_res.has_value()) {
        std::cerr << "Load error: " << load_res.error().to_string() << '\n';
    }
    TEST_ASSERT(load_res.has_value(), "ContentCatalog must load successfully from decoupled content dir");

    std::cout << "  Atoms loaded: " << catalog.atom_count() << '\n';
    std::cout << "  Relations loaded: " << catalog.relation_count() << '\n';
    std::cout << "  Variants loaded: " << catalog.variant_count() << '\n';
    std::cout << "  Recipes loaded: " << catalog.recipe_count() << '\n';

    TEST_ASSERT(catalog.atom_count() >= 4, "Must have at least 4 Pampa atoms");
    TEST_ASSERT(catalog.relation_count() >= 2, "Must have relations");
    TEST_ASSERT(catalog.recipe_count() >= 2, "Must have recipes");
    TEST_ASSERT(!catalog.manifest().bundle_id.empty(), "Manifest must be loaded");
    TEST_ASSERT(catalog.default_theme() == "pampa", "Default theme is pampa");

    auto cardeal = catalog.find_atom("species_cardeal_001");
    TEST_ASSERT(cardeal != nullptr, "Cardeal-amarelo atom must exist");
    TEST_ASSERT(cardeal->type == elo::content::ContentType::Entity, "Cardeal type is Entity");
    TEST_ASSERT(cardeal->subject.canonical_name == "Cardeal-amarelo", "Canonical name matches");
    TEST_ASSERT(!cardeal->canonical_facts.empty(), "Cardeal has facts");
    TEST_ASSERT(!cardeal->canonical_facts.front().source_ids.empty(), "Fact has required source provenance");

    auto report = elo::content::ContentValidator::validate(catalog);
    TEST_ASSERT(report.valid, "Catalog must pass constitutional validation");
    TEST_ASSERT(report.error_count() == 0, "No validation errors");

    std::cout << "  -> PASSED: Catalog loaded and constitutional validation succeeded.\n";
}

void test_content_selector_and_recipes() {
    std::cout << "[TEST] ContentSelector and RecipeExecutor...\n";
    elo::content::ContentCatalog catalog;
    std::filesystem::path content_dir = resolve_content_dir();
    (void)catalog.load_from_directory(content_dir);

    elo::content::ContentSelector selector(12345);
    elo::content::SelectionContext ctx;
    ctx.target_role = elo::content::ContentRole::Attract;
    ctx.theme_filter = std::string(catalog.default_theme());

    auto selection = selector.select(catalog, ctx);
    TEST_ASSERT(selection.has_value(), "Must select content for Attract role");
    TEST_ASSERT(selection->atom != nullptr, "Selected atom is not null");
    TEST_ASSERT(selection->atom->content_id == "species_cardeal_001", "Selected Cardeal-amarelo for attract");
    TEST_ASSERT(!selection->reason.explanation.empty(), "Selection has explainable rationale");

    // Test Recipe execution
    auto recipe = catalog.find_recipe("discover_by_sound");
    TEST_ASSERT(recipe != nullptr, "discover_by_sound recipe found");

    elo::content::RecipeExecutor executor;
    auto state = executor.start(*recipe, *selection->atom);
    TEST_ASSERT(!state.completed, "Recipe starts incomplete");

    // Step 0: play_audio
    auto actions = executor.evaluate_step(*recipe, *selection->atom, state);
    TEST_ASSERT(!actions.empty(), "Step 0 yields presentation action");
    TEST_ASSERT(actions.front().type == elo::content::PresentationActionType::PlayAudio, "Action is PlayAudio");

    // Advance to Step 1: ask_identification
    state = executor.advance(*recipe, *selection->atom, state, "");
    actions = executor.evaluate_step(*recipe, *selection->atom, state);
    TEST_ASSERT(actions.front().type == elo::content::PresentationActionType::AskChoice, "Action is AskChoice");

    // User chooses correctly
    state = executor.advance(*recipe, *selection->atom, state, "Cardeal-amarelo");
    TEST_ASSERT(state.user_was_correct, "User answered correctly");

    // Step 2: reveal_image
    actions = executor.evaluate_step(*recipe, *selection->atom, state);
    TEST_ASSERT(actions.front().type == elo::content::PresentationActionType::ShowImage, "Action is ShowImage");

    std::cout << "  -> PASSED: Contextual selection and recipe stepping verified.\n";
}

int main() {
    std::cout << "=== Running ELO Content System Tests ===\n";
    test_content_catalog_and_validation();
    test_content_selector_and_recipes();
    std::cout << "=== All ELO Content System Tests Passed! ===\n";
    return 0;
}
