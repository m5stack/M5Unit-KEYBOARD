/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file cardkb2_modifier_state.hpp
  @brief CardKB2 Sym/Caps/Fn modifier state machine (firmware-parity), event-driven and native-testable.
*/
#ifndef M5_UNIT_KEYBOARD_UTILITY_CARDKB2_MODIFIER_STATE_HPP
#define M5_UNIT_KEYBOARD_UTILITY_CARDKB2_MODIFIER_STATE_HPP

#include <cstdint>

#include "button_event_detector.hpp"

namespace m5 {
namespace unit {
namespace cardkb2 {

/*!
  @class CardKB2ModifierState
  @brief Tracks Sym (toggle + momentary), Caps (one-shot/lock/hold) and Fn (momentary) from key edges,
  mirroring the CardKB2 firmware's iot_button handlers. Pure logic; no M5 / key-constant dependency.
  @internal Internal helper; not for direct end-user use.
 */
/// @cond INTERNAL
class CardKB2ModifierState {
public:
    using ButtonEvent = m5::unit::keyboard_bitwise::ButtonEvent;

    //! @brief Feed a Sym key edge (pressed=true PRESS, false RELEASE).
    void onSymEdge(const bool pressed, const uint32_t now_ms)
    {
        _sym_held            = pressed;
        const ButtonEvent ev = _sym_detector.edge(pressed, now_ms);
        if (ev == ButtonEvent::SingleClick) _sym_mode = !_sym_mode;
    }

    //! @brief Feed an Aa (Caps) key edge.
    void onAaEdge(const bool pressed, const uint32_t now_ms)
    {
        const ButtonEvent ev = _caps_detector.edge(pressed, now_ms);
        if (pressed) {
            if (ev == ButtonEvent::PressDown && !_caps_lock) _caps_hold = true;
        } else {
            if ((ev == ButtonEvent::PressUp || ev == ButtonEvent::LongPressUp) && !_caps_lock) _caps_hold = false;
        }
    }

    //! @brief Feed a Fn key edge (momentary).
    void onFnEdge(const bool pressed)
    {
        _fn = pressed;
    }

    //! @brief Advance time; call once per tick after draining frames. Resolves delayed clicks.
    void poll(const uint32_t now_ms)
    {
        const ButtonEvent sev = _sym_detector.poll(now_ms);
        if (sev == ButtonEvent::SingleClick) _sym_mode = !_sym_mode;

        const ButtonEvent cev = _caps_detector.poll(now_ms);
        if (cev == ButtonEvent::SingleClick) {
            if (_caps_lock) {
                _caps_lock     = false;
                _caps_one_time = false;
            } else {
                _caps_one_time = true;
            }
        } else if (cev == ButtonEvent::DoubleClick) {
            _caps_lock     = !_caps_lock;
            _caps_one_time = false;
        }
    }

    inline bool fnActive() const
    {
        return _fn;
    }
    inline bool symActive() const
    {
        return _sym_mode || _sym_held;
    }
    inline bool capsActive() const
    {
        return _caps_one_time || _caps_lock || _caps_hold;
    }

    //! @brief Consume one-shot uppercase (call after emitting an alphabetic character).
    inline void consumeCapsOneShot()
    {
        if (_caps_one_time && !_caps_lock && !_caps_hold) _caps_one_time = false;
    }

    void reset()
    {
        _sym_detector.reset();
        _caps_detector.reset();
        _sym_mode = _sym_held = _fn = false;
        _caps_one_time = _caps_lock = _caps_hold = false;
    }

private:
    m5::unit::keyboard_bitwise::ButtonEventDetector _sym_detector{};
    m5::unit::keyboard_bitwise::ButtonEventDetector _caps_detector{};
    bool _sym_mode{false};
    bool _sym_held{false};
    bool _fn{false};
    bool _caps_one_time{false};
    bool _caps_lock{false};
    bool _caps_hold{false};
};
/// @endcond

}  // namespace cardkb2
}  // namespace unit
}  // namespace m5

#endif
