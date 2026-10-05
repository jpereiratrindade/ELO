#pragma once

#include "jev/types.hpp"
#include "elo/biometric/biometric_evidence.hpp"
#include <memory>
#include <vector>

namespace elo::judgment {

/// @brief Bridge between ELO and JEV contract.
/// Invariants E3 & E4: JEV evaluates state; ELO retains operational decision responsibility.
class JevAdapter final {
public:
    explicit JevAdapter(std::shared_ptr<jev::IJudge> judge)
        : judge_{std::move(judge)} {}

    /// @brief Submits biometric candidate matches to JEV to obtain structured judgment on ambiguity.
    [[nodiscard]] jev::Judgment judge_biometric_continuity(
        const std::vector<biometric::CandidateMatch>& candidates,
        double quality_score) const {
        if (!judge_) {
            return jev::Judgment{
                .payload = jev::Noul{jev::Noul::Reason::Indeterminate, "No JEV judge configured"},
                .timestamp_ns = 0,
                .judge_model_id = "null_judge"
            };
        }

        jev::Context ctx;
        ctx.context_id = "biometric_evaluation";
        ctx.numerical_features.push_back(quality_score);

        jev::Query q;
        q.query_type = "continuity_probabilistic_ranking";
        q.subject = "current_observation";

        for (const auto& c : candidates) {
            ctx.attributes.emplace_back(c.person_id.str(), std::to_string(c.similarity));
            q.candidates.push_back(c.person_id.str());
        }

        return judge_->judge(ctx, q);
    }

private:
    std::shared_ptr<jev::IJudge> judge_;
};

} // namespace elo::judgment
