#pragma once

#include <atomic>
#include <cstdint>

namespace ente {

class kernel final {
public:
    using id_type = std::uint64_t;

    struct view {
        id_type id;
        std::uint32_t generation;
        bool alive;
    };

    constexpr explicit kernel(id_type id) noexcept
        : id_{id}, word_{0} {}

    [[nodiscard]] view observe() const noexcept;
    [[nodiscard]] bool transform() noexcept;
    [[nodiscard]] bool retire() noexcept;

private:
    static constexpr std::uint64_t retired_bit =
        std::uint64_t{1} << 63;

    const id_type id_;
    std::atomic<std::uint64_t> word_;
};

static_assert(sizeof(kernel) == 16);

} // namespace ente
