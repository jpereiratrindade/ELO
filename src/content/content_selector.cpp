#include "elo/content/content_selector.hpp"
#include <algorithm>

namespace elo::content {

std::optional<ContentSelector::SelectionResult> ContentSelector::select(
    const ContentCatalog& catalog,
    const SelectionContext& context) {

    auto all = catalog.all_atoms();
    if (all.empty()) {
        return std::nullopt;
    }

    // Candidate scoring
    struct ScoredCandidate {
        const ContentAtom* atom{nullptr};
        int score{0};
        std::string explanation{};
    };

    std::vector<ScoredCandidate> candidates;

    // 1. If deepening from an existing focus, check relations first
    if (!context.current_focus_id.empty() && context.target_role == ContentRole::Deepen) {
        auto rels = catalog.find_relations_from(context.current_focus_id);
        for (const auto* r : rels) {
            const auto* target = catalog.find_atom(r->to_content_id);
            if (target && !context.seen_content_ids.contains(target->content_id)) {
                candidates.push_back({
                    target,
                    100,
                    "Followed relation '" + std::string(to_string(r->type)) + "' from " + context.current_focus_id
                });
            }
        }
        if (candidates.empty()) {
            for (const auto* r : rels) {
                const auto* target = catalog.find_atom(r->to_content_id);
                if (target && target->content_id != context.current_focus_id) {
                    candidates.push_back({
                        target,
                        90,
                        "Followed seen relation '" + std::string(to_string(r->type)) + "' from " + context.current_focus_id
                    });
                }
            }
        }
    }

    // 2. Score remaining atoms if needed
    if (candidates.empty()) {
        size_t curr_idx = 0;
        bool found_curr = false;
        for (size_t i = 0; i < all.size(); ++i) {
            if (all[i]->content_id == context.current_focus_id) {
                curr_idx = i;
                found_curr = true;
                break;
            }
        }

        for (size_t i = 0; i < all.size(); ++i) {
            const auto* atom = all[i];
            int score = 10;
            std::string reason_parts;

            // Current focus penalty: prevent sticking to the same atom
            if (atom->content_id == context.current_focus_id && all.size() > 1) {
                score -= 100;
                reason_parts += "current focus penalty; ";
            } else if (found_curr && all.size() > 1) {
                size_t dist = (i + all.size() - curr_idx) % all.size();
                score += static_cast<int>(all.size() - dist);
                reason_parts += "round-robin rotation bonus; ";
            }

            // Unseen penalty/bonus
            bool unseen = !context.seen_content_ids.contains(atom->content_id);
            if (unseen) {
                score += 50;
                reason_parts += "unseen in current session; ";
            } else {
                score -= 30;
                reason_parts += "already seen in session; ";
            }

            // Role support
            bool supports_role = false;
            for (auto r : atom->supported_roles) {
                if (r == context.target_role) {
                    supports_role = true;
                    break;
                }
            }
            if (supports_role) {
                score += 40;
                reason_parts += "supports role '" + std::string(to_string(context.target_role)) + "'; ";
            }

            // Theme filter
            if (!context.theme_filter.empty()) {
                bool has_theme = false;
                for (const auto& t : atom->themes) {
                    if (t == context.theme_filter) {
                        has_theme = true;
                        break;
                    }
                }
                if (has_theme) {
                    score += 30;
                    reason_parts += "matches theme '" + context.theme_filter + "'; ";
                }
            }

            // Media requirement check
            if (context.audio_supported && !atom->assets.audios.empty()) {
                score += 15;
                reason_parts += "has audio asset; ";
            }

            candidates.push_back({atom, score, reason_parts});
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    // Sort by score descending
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });

    const auto* best_atom = candidates.front().atom;
    const auto& best_expl = candidates.front().explanation;

    // Pick best matching variant
    auto variants = catalog.find_variants_for(best_atom->content_id);
    const ContentVariant* best_variant = nullptr;
    for (const auto* v : variants) {
        if (v->role == context.target_role) {
            best_variant = v;
            break;
        }
    }
    if (!best_variant && !variants.empty()) {
        best_variant = variants.front();
    }

    SelectionReason reason{
        .selected_content_id = best_atom->content_id,
        .selected_variant_id = best_variant ? best_variant->variant_id : "",
        .recipe_id = "",
        .requested_role = context.target_role,
        .theme_matched = context.theme_filter,
        .not_seen_in_session = !context.seen_content_ids.contains(best_atom->content_id),
        .media_requirements_met = true,
        .explanation = best_expl
    };

    return SelectionResult{
        .atom = best_atom,
        .variant = best_variant,
        .reason = std::move(reason)
    };
}

} // namespace elo::content
