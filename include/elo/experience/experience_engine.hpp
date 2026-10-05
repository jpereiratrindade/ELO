#pragma once

#include "kernel.hpp"
#include "elo/identity/identity_spaces.hpp"
#include "elo/biometric/biometric_evidence.hpp"
#include "elo/storage/storage_interfaces.hpp"
#include "elo/judgment/jev_adapter.hpp"
#include "elo/core/result.hpp"

#include <memory>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace elo::experience {

/// @brief Explicit visual and operational states defined in Section 41.
enum class SessionState {
    IDLE,
    PRESENCE_DETECTED,
    INFORMATION_PRESENTED,
    CONSENT_PENDING,
    NON_BIOMETRIC_SESSION,
    BIOMETRIC_SESSION,
    IDENTITY_UNKNOWN,
    IDENTITY_CANDIDATE,
    IDENTITY_SUPPORTED,
    IDENTITY_UNCERTAIN,
    CONTENT_ACTIVE,
    SURVEY_ELIGIBLE,
    SURVEY_SELECTED,
    SURVEY_ACTIVE,
    SESSION_COMPLETE
};

constexpr std::string_view to_string(SessionState s) noexcept {
    switch (s) {
        case SessionState::IDLE: return "IDLE";
        case SessionState::PRESENCE_DETECTED: return "PRESENCE_DETECTED";
        case SessionState::INFORMATION_PRESENTED: return "INFORMATION_PRESENTED";
        case SessionState::CONSENT_PENDING: return "CONSENT_PENDING";
        case SessionState::NON_BIOMETRIC_SESSION: return "NON_BIOMETRIC_SESSION";
        case SessionState::BIOMETRIC_SESSION: return "BIOMETRIC_SESSION";
        case SessionState::IDENTITY_UNKNOWN: return "IDENTITY_UNKNOWN";
        case SessionState::IDENTITY_CANDIDATE: return "IDENTITY_CANDIDATE";
        case SessionState::IDENTITY_SUPPORTED: return "IDENTITY_SUPPORTED";
        case SessionState::IDENTITY_UNCERTAIN: return "IDENTITY_UNCERTAIN";
        case SessionState::CONTENT_ACTIVE: return "CONTENT_ACTIVE";
        case SessionState::SURVEY_ELIGIBLE: return "SURVEY_ELIGIBLE";
        case SessionState::SURVEY_SELECTED: return "SURVEY_SELECTED";
        case SessionState::SURVEY_ACTIVE: return "SURVEY_ACTIVE";
        case SessionState::SESSION_COMPLETE: return "SESSION_COMPLETE";
    }
    return "UNKNOWN";
}

/// @brief Operational facts emitted by the ELO domain.
/// These events belong to the life of the system and never imply an
/// ente::kernel constitutive transformation.
enum class EventType {
    PRESENCE_DETECTED,
    INFORMATION_ACKNOWLEDGED,
    PARTICIPATION_ACCEPTED,
    PARTICIPATION_DECLINED,
    BIOMETRIC_CONSENT_GRANTED,
    BIOMETRIC_CONSENT_DECLINED,
    IDENTITY_EVIDENCE_EVALUATED,
    PERSON_ENROLLED,
    CONTENT_SELECTED,
    SURVEY_SELECTION_EVALUATED,
    SURVEY_RESPONSE_SUBMITTED,
    PERSON_FORGOTTEN,
    SESSION_FINISHED
};

struct Event {
    EventType type;
    std::uint64_t sequence;
};

/// @brief A domain state change caused by an operational event.
template <typename State>
struct DomainStateTransition {
    State from;
    State to;
    Event cause;
};

using SessionTransition = DomainStateTransition<SessionState>;

/// @brief An identified change to the realization of ELO itself.
/// Examples include a biometric-model replacement or a storage migration.
struct ConstitutiveTransformation {
    std::string change_id;
    std::string description;
};

/// @brief Central coordinator for personal experience sessions.
/// Conforms to Invariants E1, E2 (ente::kernel consumed), E8 (person autonomy), E17 (randomization independence).
class ExperienceEngine {
public:
    ExperienceEngine(
        identity::EloId elo_id,
        std::uint64_t kernel_seed_id,
        std::shared_ptr<storage::IBiometricStore> biometric_store,
        std::shared_ptr<storage::IExperienceStore> experience_store,
        std::shared_ptr<storage::ISurveyStore> survey_store,
        std::unique_ptr<judgment::JevAdapter> jev_adapter = nullptr);

    [[nodiscard]] const identity::EloId& elo_id() const noexcept { return elo_id_; }
    [[nodiscard]] SessionState current_state() const noexcept { return state_; }
    [[nodiscard]] ente::kernel::view kernel_view() const noexcept { return kernel_.observe(); }
    [[nodiscard]] const std::optional<identity::PersonLocalId>& active_person() const noexcept { return active_person_; }
    [[nodiscard]] std::uint64_t session_revision() const noexcept { return session_revision_; }
    [[nodiscard]] const std::optional<Event>& last_event() const noexcept { return last_event_; }
    [[nodiscard]] const std::optional<SessionTransition>& last_session_transition() const noexcept {
        return last_session_transition_;
    }
    [[nodiscard]] const std::optional<ConstitutiveTransformation>&
    last_constitutive_transformation() const noexcept {
        return last_constitutive_transformation_;
    }

    // Reserved for changes to the realization of ELO itself. Operational
    // events and session state changes must not call this API.
    core::Result<ente::kernel::view> apply_constitutive_transformation(
        const ConstitutiveTransformation& transformation);

    // Session flow triggers (Section 24)
    void on_presence_detected();
    void on_information_acknowledged();
    void decide_participation(bool participate);
    void decide_biometric_consent(bool consent);

    // Biometric continuity hypothesis ingestion (Section 21, 37)
    void evaluate_biometric_evidence(const biometric::IdentityHypothesis& hypothesis);

    // Enroll newly consented participant (Section 23, Invariant E9)
    core::Result<identity::PersonLocalId> enroll_consented_person(const std::vector<float>& embedding, double quality);

    // Content sequencing (Section 31, 37)
    [[nodiscard]] std::string select_next_content();

    // Local randomization for survey eligibility (Section 33, 48, Invariant E17)
    [[nodiscard]] bool evaluate_survey_selection(double probability = 0.5);

    // Submit survey response
    core::Result<void> submit_survey_response(
        const std::string& question_id,
        const std::string& selected_option,
        survey::LinkagePolicy policy);

    // Right to be forgotten (Section 28, Invariant E14)
    core::Result<bool> request_forget(const identity::PersonLocalId& person_id);

    // Conclude and reset session (Invariant E10, Section 47)
    void finish_session();

private:
    identity::EloId elo_id_;
    ente::kernel kernel_; // Constitution Section 14: consumed by contract, not modified
    std::shared_ptr<storage::IBiometricStore> biometric_store_;
    std::shared_ptr<storage::IExperienceStore> experience_store_;
    std::shared_ptr<storage::ISurveyStore> survey_store_;
    std::unique_ptr<judgment::JevAdapter> jev_adapter_;

    SessionState state_{SessionState::IDLE};
    std::optional<identity::PersonLocalId> active_person_{std::nullopt};
    bool biometric_consented_{false};
    std::uint64_t next_event_sequence_{1};
    std::uint64_t session_revision_{0};
    std::optional<Event> last_event_{std::nullopt};
    std::optional<SessionTransition> last_session_transition_{std::nullopt};
    std::optional<ConstitutiveTransformation> last_constitutive_transformation_{std::nullopt};

    std::mt19937_64 rng_{std::random_device{}()};
    std::uint64_t next_person_seq_{1};

    void transition_to(SessionState next_state, EventType cause);
};

} // namespace elo::experience
