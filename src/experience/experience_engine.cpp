#include "elo/experience/experience_engine.hpp"
#include <algorithm>
#include <chrono>
#include <charconv>
#include <string_view>

namespace elo::experience {

ExperienceEngine::ExperienceEngine(
    identity::EloId elo_id,
    std::uint64_t kernel_seed_id,
    std::shared_ptr<storage::IBiometricStore> biometric_store,
    std::shared_ptr<storage::IExperienceStore> experience_store,
    std::shared_ptr<storage::ISurveyStore> survey_store,
    std::unique_ptr<judgment::JevAdapter> jev_adapter,
    std::shared_ptr<storage::IJevEventStore> jev_event_store,
    std::shared_ptr<content::ContentCatalog> content_catalog)
    : elo_id_{std::move(elo_id)},
      kernel_{kernel_seed_id},
      biometric_store_{std::move(biometric_store)},
      experience_store_{std::move(experience_store)},
      survey_store_{std::move(survey_store)},
      jev_adapter_{std::move(jev_adapter)},
      jev_event_store_{std::move(jev_event_store)},
      content_catalog_{std::move(content_catalog)} {
    const auto templates = biometric_store_->get_all_templates();
    if (!templates) {
        return;
    }

    constexpr std::string_view prefix = "person-local://P";
    for (const auto& value : *templates) {
        const auto id = value.person_local_id.str();
        if (!id.starts_with(prefix)) {
            continue;
        }
        std::uint64_t index = 0;
        const auto suffix = std::string_view(id).substr(prefix.size());
        const auto [position, error] = std::from_chars(
            suffix.data(), suffix.data() + suffix.size(), index);
        if (error == std::errc{} && position == suffix.data() + suffix.size()) {
            next_person_seq_ = std::max(next_person_seq_, index + 1);
        }
    }
}

void ExperienceEngine::transition_to(SessionState next_state, EventType cause) {
    const Event event{.type = cause, .sequence = next_event_sequence_++};
    last_event_ = event;

    if (state_ == next_state) {
        return;
    }

    const auto previous_state = state_;
    state_ = next_state;
    ++session_revision_;
    last_session_transition_ = SessionTransition{
        .from = previous_state,
        .to = next_state,
        .cause = event
    };
}

core::Result<ente::kernel::view> ExperienceEngine::apply_constitutive_transformation(
    const ConstitutiveTransformation& transformation) {
    if (transformation.change_id.empty()) {
        return std::unexpected(core::make_error(
            core::ErrorCode::InvalidOperation,
            "A constitutive transformation requires a non-empty change identifier"));
    }

    if (!kernel_.transform()) {
        return std::unexpected(core::make_error(
            core::ErrorCode::InvalidConstitutionalState,
            "The ente-kernel rejected the constitutive transformation",
            transformation.change_id));
    }

    last_constitutive_transformation_ = transformation;
    return kernel_.observe();
}

void ExperienceEngine::record_jev_event(std::string_view event_name, std::string_view payload) {
    if (!jev_event_store_) {
        return;
    }
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    judgment::JevEvent ev{
        .event_name = std::string(event_name),
        .timestamp_ms = static_cast<std::uint64_t>(now),
        .session_id = current_session_id_,
        .payload = std::string(payload),
        .sequence = next_event_sequence_++
    };
    (void)jev_event_store_->record_event(ev);
}

void ExperienceEngine::on_presence_detected() {
    if (state_ == SessionState::IDLE) {
        current_session_id_ = "sess-" + std::to_string(++session_count_);
        session_seen_content_.clear();
        recipe_active_ = false;
        record_jev_event("presence.enter");
        transition_to(SessionState::PRESENCE_DETECTED, EventType::PRESENCE_DETECTED);
    }
}

void ExperienceEngine::begin_automatic_continuity() {
    if (state_ != SessionState::PRESENCE_DETECTED) {
        return;
    }
    biometric_continuity_active_ = true;
    record_jev_event("session.start");
    transition_to(SessionState::BIOMETRIC_SESSION, EventType::BIOMETRIC_CONTINUITY_STARTED);
}

void ExperienceEngine::evaluate_biometric_evidence(const biometric::IdentityHypothesis& hypothesis) {
    if (!biometric_continuity_active_) {
        return;
    }

    switch (hypothesis.state) {
        case biometric::IdentityState::SUPPORTED:
            active_person_ = hypothesis.resolved_person_id;
            if (active_person_ && experience_store_) {
                auto hist = experience_store_->get_history(*active_person_);
                if (hist) {
                    for (const auto& item : *hist) {
                        session_seen_content_.insert(item);
                    }
                }
            }
            transition_to(SessionState::IDENTITY_SUPPORTED, EventType::IDENTITY_EVIDENCE_EVALUATED);
            break;
        case biometric::IdentityState::CANDIDATE:
            active_person_ = hypothesis.resolved_person_id;
            transition_to(SessionState::IDENTITY_CANDIDATE, EventType::IDENTITY_EVIDENCE_EVALUATED);
            break;
        case biometric::IdentityState::UNCERTAIN:
            active_person_ = std::nullopt;
            transition_to(SessionState::IDENTITY_UNCERTAIN, EventType::IDENTITY_EVIDENCE_EVALUATED);
            break;
        case biometric::IdentityState::UNKNOWN:
        default:
            active_person_ = std::nullopt;
            transition_to(SessionState::IDENTITY_UNKNOWN, EventType::IDENTITY_EVIDENCE_EVALUATED);
            break;
    }
}

core::Result<identity::PersonLocalId> ExperienceEngine::enroll_local_person(
    const std::vector<float>& embedding, double quality) {
    if (!biometric_continuity_active_) {
        return std::unexpected(core::make_error(
            core::ErrorCode::BiometricContinuityInactive,
            "Biometric continuity is not active for this session"));
    }

    auto new_person = identity::PersonLocalId::from_index(next_person_seq_++);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    biometric::FaceTemplate tmpl{
        .template_id = "tmpl-" + new_person.str(),
        .person_local_id = new_person,
        .model_id = "opencv_sface",
        .model_version = "2021dec-int8",
        .representation = embedding,
        .quality = quality,
        .created_at = static_cast<std::uint64_t>(now),
        .integrity_digest = "sha256:2b0e941e6f16cc048c20aee0c8e31f569118f65d702914540f7bfdc14048d78a"
    };

    auto res = biometric_store_->save_template(tmpl);
    if (!res) {
        return std::unexpected(res.error());
    }

    active_person_ = new_person;
    transition_to(SessionState::IDENTITY_SUPPORTED, EventType::PERSON_ENROLLED);
    return new_person;
}

core::Result<biometric::IdentityHypothesis> ExperienceEngine::identify_or_enroll_local_face(
    const std::vector<float>& embedding, double quality) {
    if (!biometric_continuity_active_) {
        return std::unexpected(core::make_error(
            core::ErrorCode::BiometricContinuityInactive,
            "Biometric continuity is not active for this session"));
    }

    auto templates_result = biometric_store_->get_all_templates();
    if (!templates_result) {
        return std::unexpected(templates_result.error());
    }

    auto hypothesis = biometric_matcher_.match(embedding, *templates_result);
    if (hypothesis.state != biometric::IdentityState::UNKNOWN) {
        if (hypothesis.state == biometric::IdentityState::SUPPORTED &&
            hypothesis.resolved_person_id) {
            const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            std::vector<float> refined_embedding = embedding;
            const auto existing_templates = biometric_store_->get_templates_for(
                *hypothesis.resolved_person_id);
            if (!existing_templates) {
                return std::unexpected(existing_templates.error());
            }
            if (!existing_templates->empty()) {
                const auto latest = std::max_element(
                    existing_templates->begin(), existing_templates->end(),
                    [](const auto& left, const auto& right) {
                        return left.created_at < right.created_at;
                    });
                if (latest->representation.size() == refined_embedding.size()) {
                    for (std::size_t index = 0; index < refined_embedding.size(); ++index) {
                        refined_embedding[index] =
                            0.7F * latest->representation[index] + 0.3F * refined_embedding[index];
                    }
                }
            }
            auto refresh_result = biometric_store_->save_template(biometric::FaceTemplate{
                .template_id = "tmpl-" + hypothesis.resolved_person_id->str(),
                .person_local_id = *hypothesis.resolved_person_id,
                .model_id = "opencv_sface",
                .model_version = "2021dec-int8",
                .representation = std::move(refined_embedding),
                .quality = quality,
                .created_at = static_cast<std::uint64_t>(now),
                .integrity_digest = "sha256:2b0e941e6f16cc048c20aee0c8e31f569118f65d702914540f7bfdc14048d78a"
            });
            if (!refresh_result) {
                return std::unexpected(refresh_result.error());
            }
        }
        evaluate_biometric_evidence(hypothesis);
        return hypothesis;
    }

    auto enrollment = enroll_local_person(embedding, quality);
    if (!enrollment) {
        return std::unexpected(enrollment.error());
    }

    hypothesis.state = biometric::IdentityState::SUPPORTED;
    hypothesis.resolved_person_id = *enrollment;
    hypothesis.match_score = 1.0;
    hypothesis.liveness = biometric::LivenessState::NOT_VERIFIED;
    hypothesis.newly_enrolled = true;
    hypothesis.rationale = templates_result->empty()
        ? "First local identity enrolled automatically"
        : "Unknown participant enrolled as a new local identity";
    return hypothesis;
}

void ExperienceEngine::set_content_catalog(std::shared_ptr<content::ContentCatalog> catalog) {
    content_catalog_ = std::move(catalog);
}

core::Result<void> ExperienceEngine::select_contextual_content(
    content::ContentRole role, std::string_view theme) {
    if (!content_catalog_) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError, "ContentCatalog is not configured"));
    }
    std::string effective_theme(theme);
    if (effective_theme.empty()) {
        effective_theme = std::string(content_catalog_->default_theme());
    }
    content::SelectionContext ctx{
        .target_role = role,
        .theme_filter = effective_theme,
        .seen_content_ids = session_seen_content_,
        .current_focus_id = active_atom_ ? active_atom_->content_id : "",
        .audio_supported = true,
        .session_depth = 1
    };
    auto res = content_selector_.select(*content_catalog_, ctx);
    if (!res) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError, "No content atom matched contextual criteria"));
    }
    active_atom_ = res->atom;
    active_variant_ = res->variant;
    active_reason_ = res->reason;
    recipe_active_ = false;
    if (active_atom_) {
        session_seen_content_.insert(active_atom_->content_id);
        if (active_person_ && experience_store_) {
            (void)experience_store_->record_content_served(*active_person_, active_atom_->content_id);
        }
        record_jev_event("content.selected", active_atom_->content_id);
    }
    transition_to(SessionState::CONTENT_ACTIVE, EventType::CONTENT_SELECTED);
    return {};
}

core::Result<void> ExperienceEngine::select_atom(std::string_view content_id) {
    if (!content_catalog_) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError, "ContentCatalog not configured"));
    }
    const auto* atom = content_catalog_->find_atom(std::string(content_id));
    if (!atom) {
        atom = content_catalog_->find_atom_by_name(content_id);
    }
    if (!atom) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError, "Atom not found: " + std::string(content_id)));
    }

    active_atom_ = atom;
    auto variants = content_catalog_->find_variants_for(atom->content_id);
    active_variant_ = variants.empty() ? nullptr : variants.front();
    active_reason_ = content::SelectionReason{
        .selected_content_id = atom->content_id,
        .selected_variant_id = active_variant_ ? active_variant_->variant_id : "",
        .recipe_id = "",
        .requested_role = content::ContentRole::Attract,
        .theme_matched = "",
        .not_seen_in_session = false,
        .media_requirements_met = true,
        .explanation = "Explicitly selected atom: " + atom->title
    };
    recipe_active_ = false;

    session_seen_content_.insert(active_atom_->content_id);
    if (active_person_ && experience_store_) {
        (void)experience_store_->record_content_served(*active_person_, active_atom_->content_id);
    }
    record_jev_event("content.selected", active_atom_->content_id);
    transition_to(SessionState::CONTENT_ACTIVE, EventType::CONTENT_SELECTED);
    return {};
}

core::Result<void> ExperienceEngine::start_recipe(const std::string& recipe_id) {
    if (!content_catalog_) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError, "ContentCatalog not configured"));
    }
    const auto* rec = content_catalog_->find_recipe(recipe_id);
    if (!rec) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ContentError, "Recipe not found: " + recipe_id));
    }
    if (!active_atom_) {
        auto sel_res = select_contextual_content(content::ContentRole::Attract);
        if (!sel_res) return sel_res;
    }
    recipe_state_ = recipe_executor_.start(*rec, *active_atom_);
    recipe_active_ = true;
    record_jev_event("recipe.started", recipe_id);
    transition_to(SessionState::CONTENT_ACTIVE, EventType::CONTENT_SELECTED);
    return {};
}

std::vector<content::PresentationAction> ExperienceEngine::active_presentation_actions() const {
    if (!recipe_active_ || !active_atom_ || !content_catalog_) {
        if (active_atom_) {
            std::string title = active_atom_->title.empty() ? active_atom_->subject.canonical_name : active_atom_->title;
            std::string text;
            if (!active_atom_->canonical_facts.empty()) {
                text = active_atom_->canonical_facts.front().statement;
            } else if (active_variant_ && !active_variant_->presentation.text.empty()) {
                text = active_variant_->presentation.text;
            }

            std::string asset_path;
            if (!active_atom_->assets.images.empty()) {
                asset_path = active_atom_->assets.images.front();
            } else if (!active_atom_->assets.audios.empty()) {
                asset_path = active_atom_->assets.audios.front();
            } else if (active_variant_ && !active_variant_->presentation.media_refs.empty()) {
                asset_path = active_variant_->presentation.media_refs.front();
            }

            std::vector<std::string> options;
            if (active_variant_ && !active_variant_->presentation.options.empty()) {
                options = active_variant_->presentation.options;
            }

            return {content::PresentationAction{
                .type = content::PresentationActionType::ShowText,
                .title = std::move(title),
                .text = std::move(text),
                .asset_path = std::move(asset_path),
                .options = std::move(options),
                .target_content_id = active_atom_->content_id
            }};
        }
        if (active_variant_) {
            return {content::PresentationAction{
                .type = content::PresentationActionType::ShowText,
                .title = active_variant_->presentation.title,
                .text = active_variant_->presentation.text,
                .asset_path = active_variant_->presentation.media_refs.empty() ? "" : active_variant_->presentation.media_refs.front(),
                .options = active_variant_->presentation.options,
                .target_content_id = active_variant_->content_id
            }};
        }
        return {};
    }
    const auto* rec = content_catalog_->find_recipe(recipe_state_.recipe_id);
    if (!rec) return {};
    return recipe_executor_.evaluate_step(*rec, *active_atom_, recipe_state_);
}

void ExperienceEngine::advance_recipe(std::string_view user_action) {
    if (!recipe_active_ || !active_atom_ || !content_catalog_) return;
    const auto* rec = content_catalog_->find_recipe(recipe_state_.recipe_id);
    if (!rec) return;

    record_jev_event("user.touch", user_action);
    recipe_state_ = recipe_executor_.advance(*rec, *active_atom_, recipe_state_, user_action);
    transition_to(SessionState::CONTENT_ACTIVE, EventType::RECIPE_STEP_ADVANCED);

    if (recipe_state_.completed) {
        if (active_person_ && experience_store_) {
            (void)experience_store_->record_content_served(*active_person_, rec->recipe_id);
        }
        record_jev_event("recipe.completed", rec->recipe_id);
    }
}

std::string ExperienceEngine::select_next_content() {
    if (content_catalog_ && content_catalog_->atom_count() > 0) {
        auto res = select_contextual_content(content::ContentRole::Attract);
        if (res && active_atom_) {
            return active_atom_->content_id;
        }
    }

    transition_to(SessionState::CONTENT_ACTIVE, EventType::CONTENT_SELECTED);

    if (!active_person_) {
        return "content_welcome_anonymous";
    }

    auto history_res = experience_store_->get_history(*active_person_);
    auto history = history_res.value_or(std::vector<std::string>{});

    std::string next_content = "content_stage_1";
    if (!history.empty()) {
        if (history.back() == "content_stage_1") {
            next_content = "content_stage_2";
        } else if (history.back() == "content_stage_2") {
            next_content = "content_stage_3";
        } else {
            next_content = "content_revisit_summary";
        }
    }

    (void)experience_store_->record_content_served(*active_person_, next_content);
    return next_content;
}

bool ExperienceEngine::evaluate_survey_selection(double probability) {
    std::bernoulli_distribution dist(probability);
    bool selected = dist(rng_);
    if (selected) {
        transition_to(SessionState::SURVEY_SELECTED, EventType::SURVEY_SELECTION_EVALUATED);
    } else {
        transition_to(SessionState::SURVEY_ELIGIBLE, EventType::SURVEY_SELECTION_EVALUATED);
    }
    return selected;
}

core::Result<void> ExperienceEngine::submit_survey_response(
    const std::string& question_id,
    const std::string& selected_option,
    survey::LinkagePolicy policy) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    survey::SurveyResponse resp{
        .question_id = question_id,
        .selected_option = selected_option,
        .timestamp = static_cast<std::uint64_t>(now),
        .policy = policy,
        .linked_person = (policy == survey::LinkagePolicy::LinkedToIdentity) ? active_person_ : std::nullopt
    };

    auto res = survey_store_->record_response(resp);
    if (!res) {
        return res;
    }

    record_jev_event("answer.received", selected_option);
    transition_to(SessionState::SESSION_COMPLETE, EventType::SURVEY_RESPONSE_SUBMITTED);
    return {};
}

core::Result<bool> ExperienceEngine::request_forget(const identity::PersonLocalId& person_id) {
    (void)biometric_store_->forget_person(person_id);
    (void)experience_store_->forget_person(person_id);
    (void)survey_store_->forget_person(person_id);

    if (active_person_ && *active_person_ == person_id) {
        active_person_ = std::nullopt;
        transition_to(SessionState::IDENTITY_UNKNOWN, EventType::PERSON_FORGOTTEN);
    } else {
        transition_to(state_, EventType::PERSON_FORGOTTEN);
    }
    return true;
}

void ExperienceEngine::finish_session() {
    record_jev_event("session.end");
    active_person_ = std::nullopt;
    biometric_continuity_active_ = false;
    active_atom_ = nullptr;
    active_variant_ = nullptr;
    recipe_active_ = false;
    session_seen_content_.clear();
    transition_to(SessionState::IDLE, EventType::SESSION_FINISHED);
}

} // namespace elo::experience
