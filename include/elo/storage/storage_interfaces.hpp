#pragma once

#include "elo/biometric/face_template.hpp"
#include "elo/identity/identity_spaces.hpp"
#include "elo/survey/survey_model.hpp"
#include "elo/judgment/jev_event.hpp"
#include "elo/core/result.hpp"
#include <vector>
#include <optional>
#include <string>

namespace elo::storage {

/// @brief Isolated store for derived biometric templates (Constitution Section 27).
class IBiometricStore {
public:
    virtual ~IBiometricStore() = default;

    virtual core::Result<void> save_template(const biometric::FaceTemplate& tmpl) = 0;
    virtual core::Result<std::vector<biometric::FaceTemplate>> get_all_templates() const = 0;
    virtual core::Result<std::vector<biometric::FaceTemplate>> get_templates_for(
        const identity::PersonLocalId& person_id) const = 0;

    /// @brief Invariant E14 (LOCAL_FORGETTING) & Section 28 (Right to be forgotten).
    virtual core::Result<bool> forget_person(const identity::PersonLocalId& person_id) = 0;

    virtual core::Result<void> clear() = 0;
};

/// @brief Isolated store for participant experience state (Constitution Section 27).
class IExperienceStore {
public:
    virtual ~IExperienceStore() = default;

    virtual core::Result<void> record_content_served(
        const identity::PersonLocalId& person_id,
        const std::string& content_id) = 0;

    virtual core::Result<std::vector<std::string>> get_history(
        const identity::PersonLocalId& person_id) const = 0;

    virtual core::Result<bool> forget_person(const identity::PersonLocalId& person_id) = 0;

    virtual core::Result<void> clear() = 0;
};

/// @brief Isolated store for survey responses with strict linkage policy enforcement (Section 36).
class ISurveyStore {
public:
    virtual ~ISurveyStore() = default;

    virtual core::Result<void> record_response(const survey::SurveyResponse& response) = 0;
    virtual core::Result<std::vector<survey::SurveyResponse>> get_all_responses() const = 0;
    virtual core::Result<bool> forget_person(const identity::PersonLocalId& person_id) = 0;
    virtual core::Result<void> clear() = 0;
};

/// @brief ELO-EXPERIENCE-001 Section 15 & 16: Isolated store for JEV experience event history
class IJevEventStore {
public:
    virtual ~IJevEventStore() = default;

    virtual core::Result<void> record_event(const judgment::JevEvent& event) = 0;
    virtual core::Result<std::vector<judgment::JevEvent>> get_events_for_session(const std::string& session_id) const = 0;
    virtual core::Result<std::vector<judgment::JevEvent>> get_all_events() const = 0;
    virtual core::Result<void> clear() = 0;
};

} // namespace elo::storage
