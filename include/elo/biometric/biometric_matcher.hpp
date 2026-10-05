#pragma once

#include "elo/biometric/biometric_evidence.hpp"
#include "elo/biometric/face_template.hpp"
#include <vector>
#include <span>

namespace elo::biometric {

class BiometricMatcher {
public:
    struct Config {
        double supported_threshold{0.85};
        double candidate_threshold{0.70};
        double ambiguity_margin{0.05};
    };

    BiometricMatcher() = default;
    explicit BiometricMatcher(Config config) : config_{config} {}

    [[nodiscard]] IdentityHypothesis match(
        std::span<const float> query_embedding,
        const std::vector<FaceTemplate>& enrolled_templates) const;

    [[nodiscard]] static double cosine_similarity(
        std::span<const float> a,
        std::span<const float> b) noexcept;

private:
    Config config_;
};

} // namespace elo::biometric
