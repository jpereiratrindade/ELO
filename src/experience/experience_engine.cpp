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
    std::unique_ptr<judgment::JevAdapter> jev_adapter)
    : elo_id_{std::move(elo_id)},
      kernel_{kernel_seed_id},
      biometric_store_{std::move(biometric_store)},
      experience_store_{std::move(experience_store)},
      survey_store_{std::move(survey_store)},
      jev_adapter_{std::move(jev_adapter)} {
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

void ExperienceEngine::on_presence_detected() {
    if (state_ == SessionState::IDLE) {
        transition_to(SessionState::PRESENCE_DETECTED, EventType::PRESENCE_DETECTED);
    }
}

void ExperienceEngine::on_information_acknowledged() {
    transition_to(SessionState::CONSENT_PENDING, EventType::INFORMATION_ACKNOWLEDGED);
}

void ExperienceEngine::decide_participation(bool participate) {
    if (!participate) {
        active_person_ = std::nullopt;
        biometric_consented_ = false;
        transition_to(SessionState::IDLE, EventType::PARTICIPATION_DECLINED);
    } else {
        transition_to(SessionState::CONSENT_PENDING, EventType::PARTICIPATION_ACCEPTED);
    }
}

void ExperienceEngine::decide_biometric_consent(bool consent) {
    biometric_consented_ = consent;
    if (consent) {
        transition_to(SessionState::BIOMETRIC_SESSION, EventType::BIOMETRIC_CONSENT_GRANTED);
    } else {
        transition_to(SessionState::NON_BIOMETRIC_SESSION, EventType::BIOMETRIC_CONSENT_DECLINED);
    }
}

void ExperienceEngine::evaluate_biometric_evidence(const biometric::IdentityHypothesis& hypothesis) {
    if (!biometric_consented_) {
        return;
    }

    switch (hypothesis.state) {
        case biometric::IdentityState::SUPPORTED:
            active_person_ = hypothesis.resolved_person_id;
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

core::Result<identity::PersonLocalId> ExperienceEngine::enroll_consented_person(
    const std::vector<float>& embedding, double quality) {
    if (!biometric_consented_) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ConsentRequired,
            "Cannot enroll biometric identity without explicit consent (Invariant E9)"));
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

core::Result<biometric::IdentityHypothesis> ExperienceEngine::identify_or_enroll_consented_face(
    const std::vector<float>& embedding, double quality) {
    if (!biometric_consented_) {
        return std::unexpected(core::make_error(
            core::ErrorCode::ConsentRequired,
            "Cannot resolve or enroll a biometric identity without explicit consent"));
    }

    auto templates_result = biometric_store_->get_all_templates();
    if (!templates_result) {
        return std::unexpected(templates_result.error());
    }

    auto hypothesis = biometric_matcher_.match(embedding, *templates_result);
    if (hypothesis.state != biometric::IdentityState::UNKNOWN) {
        evaluate_biometric_evidence(hypothesis);
        return hypothesis;
    }

    auto enrollment = enroll_consented_person(embedding, quality);
    if (!enrollment) {
        return std::unexpected(enrollment.error());
    }

    hypothesis.state = biometric::IdentityState::SUPPORTED;
    hypothesis.resolved_person_id = *enrollment;
    hypothesis.match_score = 1.0;
    hypothesis.liveness = biometric::LivenessState::NOT_VERIFIED;
    hypothesis.rationale = templates_result->empty()
        ? "First consented local identity enrolled automatically"
        : "Unknown consented participant enrolled as a new local identity";
    return hypothesis;
}

std::string ExperienceEngine::select_next_content() {
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
    active_person_ = std::nullopt;
    biometric_consented_ = false;
    transition_to(SessionState::IDLE, EventType::SESSION_FINISHED);
}

} // namespace elo::experience
