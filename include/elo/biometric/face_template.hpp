#pragma once

#include "elo/identity/identity_spaces.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace elo::biometric {

/// @brief Derived local biometric representation conforming to Constitution Section 26.
struct FaceTemplate {
    std::string template_id;
    identity::PersonLocalId person_local_id;
    std::string model_id;
    std::string model_version;
    std::vector<float> representation; // Derived feature embedding vector
    double quality{0.0};               // Quality score [0.0, 1.0]
    std::uint64_t created_at{0};       // Epoch timestamp
    std::string integrity_digest;      // SHA-256 or similar integrity check
};

} // namespace elo::biometric
