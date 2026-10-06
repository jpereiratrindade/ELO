#include "elo/content/recipe_executor.hpp"

namespace elo::content {

RecipeSessionState RecipeExecutor::start(
    const ExperienceRecipe& recipe,
    const ContentAtom& atom) {
    return RecipeSessionState{
        .recipe_id = recipe.recipe_id,
        .content_id = atom.content_id,
        .current_step_index = 0,
        .completed = false,
        .user_response = std::nullopt,
        .user_was_correct = false
    };
}

std::vector<PresentationAction> RecipeExecutor::evaluate_step(
    const ExperienceRecipe& recipe,
    const ContentAtom& atom,
    const RecipeSessionState& state) const {

    std::vector<PresentationAction> actions;
    if (state.current_step_index >= recipe.steps.size()) {
        actions.push_back(PresentationAction{
            .type = PresentationActionType::Clear,
            .title = "Concluído",
            .text = "Experiência concluída."
        });
        return actions;
    }

    const auto& step = recipe.steps[state.current_step_index];

    if (step == "play_audio") {
        actions.push_back(PresentationAction{
            .type = PresentationActionType::PlayAudio,
            .title = atom.title.empty() ? atom.subject.canonical_name : atom.title,
            .text = atom.canonical_facts.empty() ? "Paisagem do Pampa e presença viva." : atom.canonical_facts.front().statement,
            .asset_path = atom.assets.audios.empty() ? "" : atom.assets.audios.front()
        });
    } else if (step == "ask_identification" || step == "ask_choice") {
        actions.push_back(PresentationAction{
            .type = PresentationActionType::AskChoice,
            .title = atom.title.empty() ? atom.subject.canonical_name : atom.title,
            .text = atom.canonical_facts.empty() ? "Conheça mais sobre esta forma de vida e seu papel ecológico:" : atom.canonical_facts.front().statement,
            .asset_path = atom.assets.images.empty() ? "" : atom.assets.images.front(),
            .options = {atom.subject.canonical_name.empty() ? "Conhecer" : atom.subject.canonical_name, "Habitats e Teia Ecológica", "Próxima Descoberta"}
        });
    } else if (step == "reveal_image" && !atom.assets.images.empty()) {
        actions.push_back(PresentationAction{
            .type = PresentationActionType::ShowImage,
            .title = atom.title.empty() ? atom.subject.canonical_name : atom.title,
            .text = atom.canonical_facts.empty() ? "" : atom.canonical_facts.front().statement,
            .asset_path = atom.assets.images.front()
        });
    } else if (step == "reveal_name" || step == "reveal") {
        actions.push_back(PresentationAction{
            .type = PresentationActionType::Reveal,
            .title = atom.subject.canonical_name,
            .text = atom.canonical_facts.empty() ? (atom.subject.scientific_name.empty() ? "" : "(" + atom.subject.scientific_name + ")") : atom.canonical_facts.front().statement,
            .asset_path = atom.assets.images.empty() ? "" : atom.assets.images.front()
        });
    } else if (step == "show_micro_fact" && !atom.canonical_facts.empty()) {
        actions.push_back(PresentationAction{
            .type = PresentationActionType::ShowText,
            .title = "Você sabia?",
            .text = atom.canonical_facts.front().statement
        });
    } else if (step == "offer_deepen") {
        actions.push_back(PresentationAction{
            .type = PresentationActionType::OfferDeepen,
            .title = "Explorar mais",
            .text = "Gostaria de ver o habitat e as relações ecológicas desta espécie?",
            .target_content_id = atom.content_id
        });
    } else {
        actions.push_back(PresentationAction{
            .type = PresentationActionType::ShowText,
            .title = atom.title.empty() ? atom.subject.canonical_name : atom.title,
            .text = atom.canonical_facts.empty() ? "" : atom.canonical_facts.front().statement
        });
    }

    return actions;
}

RecipeSessionState RecipeExecutor::advance(
    const ExperienceRecipe& recipe,
    const ContentAtom& atom,
    RecipeSessionState current_state,
    std::string_view user_action) {

    current_state.user_response = std::string(user_action);

    if (current_state.current_step_index < recipe.steps.size()) {
        const auto& step = recipe.steps[current_state.current_step_index];
        if (step == "ask_identification" || step == "ask_choice") {
            current_state.user_was_correct = (user_action == atom.subject.canonical_name);
        }
    }

    current_state.current_step_index++;
    if (current_state.current_step_index >= recipe.steps.size()) {
        current_state.completed = true;
    }

    return current_state;
}

} // namespace elo::content
