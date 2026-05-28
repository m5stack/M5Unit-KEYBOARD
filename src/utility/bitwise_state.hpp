/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file bitwise_state.hpp
  @brief Generic bit-map state tracking for keyboard-like devices.
*/
#ifndef M5_UNIT_KEYBOARD_UTILITY_BITWISE_STATE_HPP
#define M5_UNIT_KEYBOARD_UTILITY_BITWISE_STATE_HPP

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>

namespace m5 {
namespace unit {
namespace keyboard_bitwise {

/*!
  @struct BitwiseState
  @brief Per-key state tracking (now / prev / edge / hold / repeat) with timestamps.
  @tparam N Number of trackable keys.
  @internal Internal helper used by each Unit class; not intended for direct end-user use.
 */
/// @cond INTERNAL
template <size_t N>
struct BitwiseState {
    using bits_t = std::bitset<N>;

    bits_t now{};
    bits_t prev{};
    bits_t holding{};

    bits_t pressed{};
    bits_t released{};
    bits_t was_hold{};
    bits_t repeating{};

    std::array<uint32_t, N> press_at{};
    std::array<uint32_t, N> last_repeat_at{};

    uint32_t holding_threshold_ms{800};
    uint32_t repeat_initial_ms{400};
    uint32_t repeat_rate_ms{400};

    inline void resetOneShot()
    {
        pressed.reset();
        released.reset();
        was_hold.reset();
        repeating.reset();
    }

    inline void resetAll()
    {
        now.reset();
        prev.reset();
        holding.reset();
        resetOneShot();
        press_at.fill(0U);
        last_repeat_at.fill(0U);
    }

    inline void setKey(const size_t kidx, const bool is_pressed, const uint32_t now_ms)
    {
        if (kidx >= N) return;
        if (is_pressed) {
            if (!now.test(kidx)) {
                press_at[kidx]       = now_ms;
                last_repeat_at[kidx] = 0U;
            }
            now.set(kidx);
        } else {
            now.reset(kidx);
        }
    }

    inline void computeEdges()
    {
        const bits_t changed = now ^ prev;
        pressed              = changed & now;
        released             = changed & ~now;
    }

    inline void tickHoldRepeat(const uint32_t now_ms)
    {
        for (size_t i = 0; i < N; ++i) {
            if (now.test(i)) {
                const uint32_t elapsed = now_ms - press_at[i];
                if (elapsed >= holding_threshold_ms) {
                    if (!holding.test(i)) was_hold.set(i);
                    holding.set(i);
                }
                if (elapsed >= repeat_initial_ms) {
                    if (last_repeat_at[i] == 0U || (now_ms - last_repeat_at[i]) >= repeat_rate_ms) {
                        repeating.set(i);
                        last_repeat_at[i] = now_ms;
                    }
                }
            } else {
                holding.reset(i);
                last_repeat_at[i] = 0U;
            }
        }
    }

    inline void commitPrev()
    {
        prev = now;
    }
};
/// @endcond

}  // namespace keyboard_bitwise
}  // namespace unit
}  // namespace m5

#endif
