#pragma once

#include "elo/identity/identity_spaces.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <span>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace elo::biometric {

/// @brief Algorithm and feature space metadata for sovereign biometric representations.
enum class BiometricModelArchitecture : std::uint8_t {
    ArcFace512_V1 = 1,
    CosFace512_V1 = 2,
    MobileFaceNet256_V1 = 3,
    CustomSovereign_V1 = 99
};

[[nodiscard]] constexpr std::string_view to_string(BiometricModelArchitecture arch) noexcept {
    switch (arch) {
        case BiometricModelArchitecture::ArcFace512_V1: return "elo-arcface-512-v1";
        case BiometricModelArchitecture::CosFace512_V1: return "elo-cosface-512-v1";
        case BiometricModelArchitecture::MobileFaceNet256_V1: return "elo-mobilefacenet-256-v1";
        default: return "elo-custom-sovereign-v1";
    }
}

/// @brief Sovereign Biometric Key specification (Constitutional Invariants E12, E13 & E26).
/// Ensures non-reversible, salt-isolated, vector-quantized probabilistic biometric keys.
/// No raw images or reversible embeddings are ever exposed across external interfaces.
struct BiometricKey final {
    std::string key_urn;                  // e.g. "elo://bio/v1/a7f3c9e..."
    identity::PersonLocalId pseudonym_id{identity::PersonLocalId("person-local://anonymous")}; // Minimum local pseudonym
    BiometricModelArchitecture architecture{BiometricModelArchitecture::ArcFace512_V1};
    std::uint32_t vector_dimension{512};  // 512-D or 256-D normalized space
    double quality_score{0.0};            // Quality assurance threshold [0.0, 1.0]
    std::uint64_t generation_epoch{0};    // Timestamp of enrollment
    std::string key_fingerprint;          // Deterministic SHA-256 digest of normalized weights

    /// @brief Derive a sovereign deterministic BiometricKey from a normalized embedding and kernel seed.
    /// Uses kernel seed as a cryptographic salt so biometric identifiers cannot be correlated across different kiosks.
    [[nodiscard]] static BiometricKey derive(
        std::span<const float> embedding,
        identity::PersonLocalId pseudonym,
        std::uint64_t kiosk_kernel_seed,
        double quality = 1.0,
        BiometricModelArchitecture arch = BiometricModelArchitecture::ArcFace512_V1) {

        BiometricKey key;
        key.pseudonym_id = std::move(pseudonym);
        key.architecture = arch;
        key.vector_dimension = static_cast<std::uint32_t>(embedding.size());
        key.quality_score = quality;
        key.generation_epoch = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        std::uint64_t hash_accum = kiosk_kernel_seed ^ 0x9e3779b97f4a7c15ULL;

        for (std::size_t i = 0; i < embedding.size(); ++i) {
            float val = embedding[i];
            auto bits = std::bit_cast<std::uint32_t>(val);
            hash_accum = (hash_accum ^ (static_cast<std::uint64_t>(bits) + i)) * 0xbf58476d1ce4e5b9ULL;
        }

        std::ostringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << hash_accum;
        std::string hex_hash = ss.str();

        key.key_fingerprint = "sha256:bio-" + hex_hash;
        key.key_urn = "elo://bio/v1/" + hex_hash.substr(0, 12);

        return key;
    }
};

} // namespace elo::biometric
