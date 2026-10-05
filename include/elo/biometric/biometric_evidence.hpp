#pragma once

#include "elo/identity/identity_spaces.hpp"
#include <string>
#include <optional>
#include <vector>
#include <string_view>

namespace elo::biometric {

/// @brief Probabilistic identity states defined in Constitution Section 21 & 42.
enum class IdentityState {
    UNKNOWN,
    CANDIDATE,
    SUPPORTED,
    UNCERTAIN,
    CONTRADICTORY,
    UNAVAILABLE
};

constexpr std::string_view to_string(IdentityState s) noexcept {
    switch (s) {
        case IdentityState::UNKNOWN: return "UNKNOWN";
        case IdentityState::CANDIDATE: return "CANDIDATE";
        case IdentityState::SUPPORTED: return "SUPPORTED";
        case IdentityState::UNCERTAIN: return "UNCERTAIN";
        case IdentityState::CONTRADICTORY: return "CONTRADICTORY";
        case IdentityState::UNAVAILABLE: return "UNAVAILABLE";
    }
    return "UNKNOWN";
}

/// @brief Liveness state representation (Constitution Section 44).
enum class LivenessState {
    NOT_VERIFIED,
    CHALLENGE_PENDING,
    PASSIVE_EVIDENCED,
    FAILED
};

/// @brief Probabilistic candidate match.
struct CandidateMatch {
    identity::PersonLocalId person_id;
    double similarity{0.0}; // [0.0, 1.0]
    double confidence{0.0}; // [0.0, 1.0]
};

/// @brief Result of biometric identity resolution.
/// Invariant E12: Biometric matching constitutes probabilistic evidence, never absolute certainty.
struct IdentityHypothesis {
    IdentityState state{IdentityState::UNKNOWN};
    std::optional<identity::PersonLocalId> resolved_person_id{std::nullopt};
    double match_score{0.0};
    LivenessState liveness{LivenessState::NOT_VERIFIED};
    bool newly_enrolled{false};
    std::vector<CandidateMatch> ranked_candidates{};
    std::string rationale{};
};

} // namespace elo::biometric
