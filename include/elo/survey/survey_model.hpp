#pragma once

#include "elo/identity/identity_spaces.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace elo::survey {

/// @brief Linkage policy between participant identity and responses (Constitution Section 36).
enum class LinkagePolicy {
    LinkedToIdentity,   // Section 36.1: Pxx -> response (longitudinal tracking authorized)
    UnlinkedAnonymous   // Section 36.2: Pxx -> participation validated -> link stripped -> response
};

struct SurveyQuestion {
    std::string id;
    std::string text;
    std::vector<std::string> options;
};

struct SurveyResponse {
    std::string question_id;
    std::string selected_option;
    std::uint64_t timestamp{0};
    LinkagePolicy policy{LinkagePolicy::UnlinkedAnonymous};
    std::optional<identity::PersonLocalId> linked_person{std::nullopt};
};

} // namespace elo::survey
