#pragma once

#include "kernel.hpp"
#include "elo/identity/identity_spaces.hpp"
#include "elo/biometric/biometric_evidence.hpp"
#include "elo/biometric/biometric_matcher.hpp"
#include "elo/storage/storage_interfaces.hpp"
#include "elo/judgment/jev_adapter.hpp"
#include "elo/judgment/jev_event.hpp"
#include "elo/content/content_catalog.hpp"
#include "elo/content/content_selector.hpp"
#include "elo/content/recipe_executor.hpp"
#include "elo/core/result.hpp"

#include <memory>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>

namespace elo::experience {

/// @brief Explicit visual and operational states defined in Section 41.
enum class SessionState {
    IDLE,
    PRESENCE_DETECTED,
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
    BIOMETRIC_CONTINUITY_STARTED,
    IDENTITY_EVIDENCE_EVALUATED,
    PERSON_ENROLLED,
    CONTENT_SELECTED,
    RECIPE_STEP_ADVANCED,
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

/// @brief Central coordinator for personal and contextual experience sessions.
/// Conforms to Invariants E1, E2, E8, E17, and ELO-EXPERIENCE-001 / ELO-CONTENT-001.
class ExperienceEngine {
public:
    ExperienceEngine(
        identity::EloId elo_id,
        std::uint64_t kernel_seed_id,
        std::shared_ptr<storage::IBiometricStore> biometric_store,
        std::shared_ptr<storage::IExperienceStore> experience_store,
        std::shared_ptr<storage::ISurveyStore> survey_store,
        std::unique_ptr<judgment::JevAdapter> jev_adapter = nullptr,
        std::shared_ptr<storage::IJevEventStore> jev_event_store = nullptr,
        std::shared_ptr<content::ContentCatalog> content_catalog = nullptr);

    [[nodiscard]] const identity::EloId& elo_id() const noexcept { return elo_id_; }
    [[nodiscard]] SessionState current_state() const noexcept { return state_; }
    [[nodiscard]] ente::kernel::view kernel_view() const noexcept { return kernel_.observe(); }
    [[nodiscard]] const std::optional<identity::PersonLocalId>& active_person() const noexcept { return active_person_; }
    [[nodiscard]] std::uint64_t session_revision() const noexcept { return session_revision_; }
    [[nodiscard]] const std::string& current_session_id() const noexcept { return current_session_id_; }
    [[nodiscard]] const std::optional<Event>& last_event() const noexcept { return last_event_; }
    [[nodiscard]] const std::optional<SessionTransition>& last_session_transition() const noexcept {
        return last_session_transition_;
    }
    [[nodiscard]] const std::optional<ConstitutiveTransformation>&
    last_constitutive_transformation() const noexcept {
        return last_constitutive_transformation_;
    }

    core::Result<ente::kernel::view> apply_constitutive_transformation(
        const ConstitutiveTransformation& transformation);

    // Session flow triggers (ELO-EXPERIENCE-001 Section 24)
    void on_presence_detected();
    void begin_automatic_continuity();

    // Biometric continuity hypothesis ingestion
    void evaluate_biometric_evidence(const biometric::IdentityHypothesis& hypothesis);

    core::Result<identity::PersonLocalId> enroll_local_person(
        const std::vector<float>& embedding,
        double quality);

    core::Result<biometric::IdentityHypothesis> identify_or_enroll_local_face(
        const std::vector<float>& embedding,
        double quality);

    // Content sequencing (ELO-CONTENT-001 & ELO-EXPERIENCE-001)
    void set_content_catalog(std::shared_ptr<content::ContentCatalog> catalog);
    [[nodiscard]] std::shared_ptr<content::ContentCatalog> content_catalog() const noexcept { return content_catalog_; }
    [[nodiscard]] std::shared_ptr<storage::IJevEventStore> jev_event_store() const noexcept { return jev_event_store_; }

    [[nodiscard]] std::string select_next_content();
    core::Result<void> select_contextual_content(
        content::ContentRole role = content::ContentRole::Attract,
        std::string_view theme = "");
    core::Result<void> select_atom(std::string_view content_id);

    [[nodiscard]] const content::ContentAtom* active_content_atom() const noexcept { return active_atom_; }
    [[nodiscard]] const content::ContentVariant* active_content_variant() const noexcept { return active_variant_; }
    [[nodiscard]] const content::SelectionReason& active_selection_reason() const noexcept { return active_reason_; }

    core::Result<void> start_recipe(const std::string& recipe_id);
    [[nodiscard]] std::vector<content::PresentationAction> active_presentation_actions() const;
    void advance_recipe(std::string_view user_action);
    [[nodiscard]] bool is_recipe_active() const noexcept { return recipe_active_; }
    [[nodiscard]] const content::RecipeSessionState& recipe_state() const noexcept { return recipe_state_; }

    // JEV Event recording (Section 15, 16)
    void record_jev_event(std::string_view event_name, std::string_view payload = "");

    // Local randomization for survey eligibility
    [[nodiscard]] bool evaluate_survey_selection(double probability = 0.5);

    // Submit survey response
    core::Result<void> submit_survey_response(
        const std::string& question_id,
        const std::string& selected_option,
        survey::LinkagePolicy policy);

    // Right to be forgotten (Section 28, Invariant E14)
    core::Result<bool> request_forget(const identity::PersonLocalId& person_id);

    // Conclude and reset session
    void finish_session();

private:
    identity::EloId elo_id_;
    ente::kernel kernel_;
    std::shared_ptr<storage::IBiometricStore> biometric_store_;
    std::shared_ptr<storage::IExperienceStore> experience_store_;
    std::shared_ptr<storage::ISurveyStore> survey_store_;
    std::unique_ptr<judgment::JevAdapter> jev_adapter_;
    std::shared_ptr<storage::IJevEventStore> jev_event_store_;
    std::shared_ptr<content::ContentCatalog> content_catalog_;
    biometric::BiometricMatcher biometric_matcher_;
    content::ContentSelector content_selector_{42};
    content::RecipeExecutor recipe_executor_{};

    SessionState state_{SessionState::IDLE};
    std::string current_session_id_{"sess-0"};
    std::uint64_t session_count_{0};
    std::optional<identity::PersonLocalId> active_person_{std::nullopt};
    bool biometric_continuity_active_{false};
    std::uint64_t next_event_sequence_{1};
    std::uint64_t session_revision_{0};
    std::optional<Event> last_event_{std::nullopt};
    std::optional<SessionTransition> last_session_transition_{std::nullopt};
    std::optional<ConstitutiveTransformation> last_constitutive_transformation_{std::nullopt};

    // Content & Recipe session state
    const content::ContentAtom* active_atom_{nullptr};
    const content::ContentVariant* active_variant_{nullptr};
    content::SelectionReason active_reason_{};
    std::unordered_set<std::string> session_seen_content_{};
    bool recipe_active_{false};
    content::RecipeSessionState recipe_state_{};

    std::mt19937_64 rng_{std::random_device{}()};
    std::uint64_t next_person_seq_{1};

    void transition_to(SessionState next_state, EventType cause);
};

} // namespace elo::experience
