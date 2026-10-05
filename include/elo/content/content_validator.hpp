#pragma once

#include "elo/content/content_catalog.hpp"
#include <string>
#include <vector>

namespace elo::content {

/// @brief Result of catalog validation
struct ValidationError {
    std::string item_id;
    std::string message;
    bool is_critical{true};
};

struct ValidationReport {
    bool valid{true};
    std::vector<ValidationError> errors{};
    std::vector<std::string> warnings{};

    [[nodiscard]] std::size_t error_count() const noexcept { return errors.size(); }
};

/// @brief ELO-CONTENT-001 Section 55: Content Validator
class ContentValidator {
public:
    ContentValidator() = default;

    /// @brief Validates integrity, relations, provenance and recipe requirements
    [[nodiscard]] static ValidationReport validate(const ContentCatalog& catalog);
};

} // namespace elo::content
