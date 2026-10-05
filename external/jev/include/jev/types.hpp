#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <cstdint>
#include <concepts>
#include <string_view>

namespace jev {

/// @brief Represents a probabilistic score with an associated confidence/epistemic weight.
struct Score {
    double value{0.0};       // Normalized value [0.0, 1.0]
    double confidence{1.0};  // Epistemic confidence in this score [0.0, 1.0]

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return value >= 0.0 && value <= 1.0 && confidence >= 0.0 && confidence <= 1.0;
    }
};

/// @brief Represents a probabilistic choice among discrete options.
struct Choice {
    std::string selected_id;
    double probability{0.0};
    std::vector<std::pair<std::string, double>> distribution{};
};

/// @brief Represents structured epistemic absence of judgment (undecidability, lack of evidence).
struct Noul {
    enum class Reason : std::uint8_t {
        InsufficientEvidence,
        ContradictoryEvidence,
        DomainOutOfScope,
        Indeterminate
    };

    Reason reason{Reason::Indeterminate};
    std::string rationale{};
};

/// @brief Tagged judgment output produced strictly according to JEV contract.
/// Invariant: FACT != JUDGMENT != DECISION != ACTION.
struct Judgment {
    std::variant<Score, Choice, Noul> payload{Noul{Noul::Reason::Indeterminate, "Uninitialized judgment"}};
    uint64_t timestamp_ns{0};
    std::string judge_model_id{};

    [[nodiscard]] bool is_score() const noexcept {
        return std::holds_alternative<Score>(payload);
    }
    [[nodiscard]] bool is_choice() const noexcept {
        return std::holds_alternative<Choice>(payload);
    }
    [[nodiscard]] bool is_noul() const noexcept {
        return std::holds_alternative<Noul>(payload);
    }

    [[nodiscard]] const Score* as_score() const noexcept {
        return std::get_if<Score>(&payload);
    }
    [[nodiscard]] const Choice* as_choice() const noexcept {
        return std::get_if<Choice>(&payload);
    }
    [[nodiscard]] const Noul* as_noul() const noexcept {
        return std::get_if<Noul>(&payload);
    }
};

/// @brief Context submitted to JEV for judgment.
struct Context {
    std::string context_id;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::vector<double> numerical_features;
};

/// @brief Query defining the question posed to JEV.
struct Query {
    std::string query_type;
    std::string subject;
    std::vector<std::string> candidates;
};

/// @brief Pure interface for a JEV Judgment Provider.
/// JEV observes context and query, then judges. It NEVER acts.
class IJudge {
public:
    virtual ~IJudge() = default;
    [[nodiscard]] virtual Judgment judge(const Context& context, const Query& query) = 0;
};

} // namespace jev
