#include "elo/experience/experience_engine.hpp"
#include <chrono>

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
      jev_adapter_{std::move(jev_adapter)} {}

void ExperienceEngine::on_presence_detected() {
    if (state_ == SessionState::IDLE) {
        state_ = SessionState::PRESENCE_DETECTED;
        (void)kernel_.transform();
    }
}

void ExperienceEngine::on_information_acknowledged() {
    state_ = SessionState::CONSENT_PENDING;
    (void)kernel_.transform();
}

void ExperienceEngine::decide_participation(bool participate) {
    if (!participate) {
        state_ = SessionState::IDLE;
        active_person_ = std::nullopt;
        biometric_consented_ = false;
    } else {
        state_ = SessionState::CONSENT_PENDING;
    }
    (void)kernel_.transform();
}

void ExperienceEngine::decide_biometric_consent(bool consent) {
    biometric_consented_ = consent;
    if (consent) {
        state_ = SessionState::BIOMETRIC_SESSION;
    } else {
        state_ = SessionState::NON_BIOMETRIC_SESSION;
    }
    (void)kernel_.transform();
}

void ExperienceEngine::evaluate_biometric_evidence(const biometric::IdentityHypothesis& hypothesis) {
    if (!biometric_consented_) {
        return;
    }

    switch (hypothesis.state) {
        case biometric::IdentityState::SUPPORTED:
            state_ = SessionState::IDENTITY_SUPPORTED;
            active_person_ = hypothesis.resolved_person_id;
            break;
        case biometric::IdentityState::CANDIDATE:
            state_ = SessionState::IDENTITY_CANDIDATE;
            active_person_ = hypothesis.resolved_person_id;
            break;
        case biometric::IdentityState::UNCERTAIN:
            state_ = SessionState::IDENTITY_UNCERTAIN;
            active_person_ = std::nullopt;
            break;
        case biometric::IdentityState::UNKNOWN:
        default:
            state_ = SessionState::IDENTITY_UNKNOWN;
            active_person_ = std::nullopt;
            break;
    }
    (void)kernel_.transform();
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
        .model_id = "sface_local",
        .model_version = "0.2.0",
        .representation = embedding,
        .quality = quality,
        .created_at = static_cast<std::uint64_t>(now),
        .integrity_digest = "sha256:local_verified"
    };

    auto res = biometric_store_->save_template(tmpl);
    if (!res) {
        return std::unexpected(res.error());
    }

    active_person_ = new_person;
    state_ = SessionState::IDENTITY_SUPPORTED;
    (void)kernel_.transform();
    return new_person;
}

std::string ExperienceEngine::select_next_content() {
    state_ = SessionState::CONTENT_ACTIVE;
    (void)kernel_.transform();

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
        state_ = SessionState::SURVEY_SELECTED;
    } else {
        state_ = SessionState::SURVEY_ELIGIBLE;
    }
    (void)kernel_.transform();
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

    state_ = SessionState::SESSION_COMPLETE;
    (void)kernel_.transform();
    return {};
}

core::Result<bool> ExperienceEngine::request_forget(const identity::PersonLocalId& person_id) {
    (void)biometric_store_->forget_person(person_id);
    (void)experience_store_->forget_person(person_id);
    (void)survey_store_->forget_person(person_id);

    if (active_person_ && *active_person_ == person_id) {
        active_person_ = std::nullopt;
        state_ = SessionState::IDENTITY_UNKNOWN;
    }
    (void)kernel_.transform();
    return true;
}

void ExperienceEngine::finish_session() {
    active_person_ = std::nullopt;
    biometric_consented_ = false;
    state_ = SessionState::IDLE;
    (void)kernel_.transform();
}

} // namespace elo::experience
