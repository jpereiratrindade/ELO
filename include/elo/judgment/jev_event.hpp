#pragma once

#include <cstdint>
#include <string>

namespace elo::judgment {

/// @brief ELO-EXPERIENCE-001 Section 5.4, 15 & ELO-CONTENT-001 Section 57:
/// JEV events represent lived experiential facts with timestamp and payload.
struct JevEvent {
    std::string event_name;
    std::uint64_t timestamp_ms{0};
    std::string session_id{};
    std::string payload{};
    std::uint64_t sequence{0};
};

} // namespace elo::judgment
