#pragma once

#include <string>
#include <string_view>
#include <format>
#include <cstdint>
#include <compare>
#include <type_traits>

namespace elo::identity {

/// @brief Represents the identity of the ELO system instance (e.g. "elo://S01").
/// Section 19.1
class EloId final {
public:
    explicit EloId(std::string id) : value_{std::move(id)} {}

    [[nodiscard]] const std::string& str() const noexcept { return value_; }
    [[nodiscard]] std::string_view view() const noexcept { return value_; }

    auto operator<=>(const EloId&) const = default;

private:
    std::string value_;
};

/// @brief Represents the constitutive identity maintained by ente::kernel.
/// Section 19.3
class KernelId final {
public:
    explicit constexpr KernelId(std::uint64_t val) noexcept : value_{val} {}

    [[nodiscard]] constexpr std::uint64_t value() const noexcept { return value_; }

    auto operator<=>(const KernelId&) const = default;

private:
    std::uint64_t value_{0};
};

/// @brief Represents the physical hardware device identity (e.g. "device://D04").
/// Section 19.2
class DeviceId final {
public:
    explicit DeviceId(std::string id) : value_{std::move(id)} {}

    [[nodiscard]] const std::string& str() const noexcept { return value_; }
    [[nodiscard]] std::string_view view() const noexcept { return value_; }

    auto operator<=>(const DeviceId&) const = default;

private:
    std::string value_;
};

/// @brief Represents a locally distinguished human participant (e.g. "person-local://P17").
/// Section 19.4 & 20: Minimum human identity, no civil identity required.
class PersonLocalId final {
public:
    explicit PersonLocalId(std::string id) : value_{std::move(id)} {}

    [[nodiscard]] static PersonLocalId from_index(std::uint64_t idx) {
        return PersonLocalId(std::format("person-local://P{:02d}", idx));
    }

    [[nodiscard]] const std::string& str() const noexcept { return value_; }
    [[nodiscard]] std::string_view view() const noexcept { return value_; }

    auto operator<=>(const PersonLocalId&) const = default;

private:
    std::string value_;
};

// Compile-time checks for Section 19: ELO_ID != KERNEL_ID != DEVICE_ID != PERSON_LOCAL_ID
static_assert(!std::is_convertible_v<EloId, KernelId>);
static_assert(!std::is_convertible_v<EloId, DeviceId>);
static_assert(!std::is_convertible_v<EloId, PersonLocalId>);
static_assert(!std::is_convertible_v<KernelId, EloId>);
static_assert(!std::is_convertible_v<KernelId, DeviceId>);
static_assert(!std::is_convertible_v<KernelId, PersonLocalId>);
static_assert(!std::is_convertible_v<DeviceId, EloId>);
static_assert(!std::is_convertible_v<DeviceId, KernelId>);
static_assert(!std::is_convertible_v<DeviceId, PersonLocalId>);
static_assert(!std::is_convertible_v<PersonLocalId, EloId>);
static_assert(!std::is_convertible_v<PersonLocalId, KernelId>);
static_assert(!std::is_convertible_v<PersonLocalId, DeviceId>);

} // namespace elo::identity
