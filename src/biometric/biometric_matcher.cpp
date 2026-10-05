#include "elo/biometric/biometric_matcher.hpp"
#include <cmath>
#include <algorithm>

namespace elo::biometric {

double BiometricMatcher::cosine_similarity(
    std::span<const float> a,
    std::span<const float> b) noexcept {
    if (a.empty() || a.size() != b.size()) {
        return 0.0;
    }

    double dot = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;

    for (std::size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
        norm_a += static_cast<double>(a[i]) * static_cast<double>(a[i]);
        norm_b += static_cast<double>(b[i]) * static_cast<double>(b[i]);
    }

    if (norm_a <= 0.0 || norm_b <= 0.0) {
        return 0.0;
    }

    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

IdentityHypothesis BiometricMatcher::match(
    std::span<const float> query_embedding,
    const std::vector<FaceTemplate>& enrolled_templates) const {

    IdentityHypothesis hypothesis;
    hypothesis.liveness = LivenessState::NOT_VERIFIED;

    if (enrolled_templates.empty() || query_embedding.empty()) {
        hypothesis.state = IdentityState::UNKNOWN;
        hypothesis.rationale = "No enrolled templates or empty query";
        return hypothesis;
    }

    std::vector<CandidateMatch> candidates;
    candidates.reserve(enrolled_templates.size());

    for (const auto& tmpl : enrolled_templates) {
        double sim = cosine_similarity(query_embedding, tmpl.representation);
        candidates.push_back(CandidateMatch{
            .person_id = tmpl.person_local_id,
            .similarity = sim,
            .confidence = tmpl.quality * sim
        });
    }

    std::sort(candidates.begin(), candidates.end(), [](const CandidateMatch& a, const CandidateMatch& b) {
        return a.similarity > b.similarity;
    });

    hypothesis.ranked_candidates = candidates;

    const auto& best = candidates.front();
    hypothesis.match_score = best.similarity;

    // Check ambiguity if there are at least two candidates
    if (candidates.size() >= 2) {
        const auto& second = candidates[1];
        if (best.similarity >= config_.candidate_threshold &&
            (best.similarity - second.similarity) < config_.ambiguity_margin) {
            hypothesis.state = IdentityState::UNCERTAIN;
            hypothesis.rationale = "Ambiguous match between top candidates";
            hypothesis.resolved_person_id = std::nullopt;
            return hypothesis;
        }
    }

    if (best.similarity >= config_.supported_threshold) {
        hypothesis.state = IdentityState::SUPPORTED;
        hypothesis.resolved_person_id = best.person_id;
        hypothesis.rationale = "Probabilistic continuity supported";
    } else if (best.similarity >= config_.candidate_threshold) {
        hypothesis.state = IdentityState::CANDIDATE;
        hypothesis.resolved_person_id = best.person_id;
        hypothesis.rationale = "Candidate match under verification";
    } else {
        hypothesis.state = IdentityState::UNKNOWN;
        hypothesis.resolved_person_id = std::nullopt;
        hypothesis.rationale = "Similarity below candidate threshold";
    }

    return hypothesis;
}

} // namespace elo::biometric
