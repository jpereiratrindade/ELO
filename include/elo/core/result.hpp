#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <system_error>

namespace elo::core {

enum class ErrorCode {
    Success = 0,
    InvalidConstitutionalState,
    IdentitySpaceMismatch,
    ConsentRequired,
    EpistemicUncertainty,
    EntityRetired,
    BiometricStoreError,
    ExperienceStoreError,
    ForgottenIdentity,
    SubstrateFailure,
    InvalidOperation
};

struct Error {
    ErrorCode code{ErrorCode::Success};
    std::string message{};
    std::string detail{};

    [[nodiscard]] std::string to_string() const {
        if (detail.empty()) return message;
        return message + " [" + detail + "]";
    }
};

template <typename T>
using Result = std::expected<T, Error>;

inline Error make_error(ErrorCode code, std::string_view msg, std::string_view detail = "") {
    return Error{code, std::string(msg), std::string(detail)};
}

} // namespace elo::core
