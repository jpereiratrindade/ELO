#include "kernel.hpp"

namespace ente {

kernel::view kernel::observe() const noexcept {
    const auto w = word_.load(std::memory_order_relaxed);

    return {
        id_,
        static_cast<std::uint32_t>(w),
        !(w & retired_bit)
    };
}

bool kernel::transform() noexcept {
    auto old = word_.load(std::memory_order_relaxed);

    for (;;) {
        if (old & retired_bit ||
            static_cast<std::uint32_t>(old) == 0xffff'ffffu)
            return false;

        if (word_.compare_exchange_weak(
                old,
                old + 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed))
            return true;
    }
}

bool kernel::retire() noexcept {
    return !(word_.fetch_or(
                 retired_bit,
                 std::memory_order_relaxed)
             & retired_bit);
}

} // namespace ente
