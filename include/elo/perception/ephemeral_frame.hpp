#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <span>
#include <cstring>

namespace elo::perception {

/// @brief Enforces Invariant E10 (RAW_IMAGE_EPHEMERALITY) and Section 25.
/// EphemeralFrame cannot be copied, only moved. Memory is securely erased upon destruction.
class EphemeralFrame final {
public:
    EphemeralFrame() = default;

    EphemeralFrame(std::uint32_t width, std::uint32_t height, std::uint32_t channels, std::vector<std::uint8_t> data)
        : width_{width}, height_{height}, channels_{channels}, buffer_{std::move(data)} {}

    ~EphemeralFrame() {
        wipe();
    }

    // Explicitly non-copyable to prevent accidental proliferation or persisting
    EphemeralFrame(const EphemeralFrame&) = delete;
    EphemeralFrame& operator=(const EphemeralFrame&) = delete;

    // Move-constructible and move-assignable
    EphemeralFrame(EphemeralFrame&& other) noexcept
        : width_{other.width_}, height_{other.height_}, channels_{other.channels_}, buffer_{std::move(other.buffer_)} {
        other.width_ = 0;
        other.height_ = 0;
        other.channels_ = 0;
    }

    EphemeralFrame& operator=(EphemeralFrame&& other) noexcept {
        if (this != &other) {
            wipe();
            width_ = other.width_;
            height_ = other.height_;
            channels_ = other.channels_;
            buffer_ = std::move(other.buffer_);
            other.width_ = 0;
            other.height_ = 0;
            other.channels_ = 0;
        }
        return *this;
    }

    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::uint32_t channels() const noexcept { return channels_; }
    [[nodiscard]] std::span<const std::uint8_t> data() const noexcept { return buffer_; }
    [[nodiscard]] std::span<std::uint8_t> data() noexcept { return buffer_; }
    [[nodiscard]] bool empty() const noexcept { return buffer_.empty(); }

    void wipe() noexcept {
        if (!buffer_.empty()) {
            std::memset(buffer_.data(), 0, buffer_.size());
            buffer_.clear();
            buffer_.shrink_to_fit();
        }
        width_ = 0;
        height_ = 0;
        channels_ = 0;
    }

private:
    std::uint32_t width_{0};
    std::uint32_t height_{0};
    std::uint32_t channels_{0};
    std::vector<std::uint8_t> buffer_{};
};

} // namespace elo::perception
