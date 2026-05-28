/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file button_event_detector.hpp
  @brief Host-side port of the iot_button v4.1.4 click/long-press FSM.
  @details Converts raw PRESS/RELEASE frame edges plus a millisecond clock into
  semantic button events. Drives off edge() (frame arrival) and poll() (each tick).
*/
#ifndef M5_UNIT_KEYBOARD_UTILITY_BUTTON_EVENT_DETECTOR_HPP
#define M5_UNIT_KEYBOARD_UTILITY_BUTTON_EVENT_DETECTOR_HPP

#include <cstdint>

namespace m5 {
namespace unit {
namespace keyboard_bitwise {

//! @brief Semantic button events (subset of iot_button needed for CardKB2 modifiers)
enum class ButtonEvent : uint8_t {
    None,
    PressDown,
    PressUp,
    SingleClick,
    DoubleClick,
    LongPressStart,
    LongPressUp,
};

/*!
  @class ButtonEventDetector
  @brief Per-key click/long-press recognizer mirroring iot_button v4.1.4 timing.
  @internal Internal helper; not for direct end-user use.
 */
/// @cond INTERNAL
class ButtonEventDetector {
public:
    struct config_t {
        uint32_t short_press_ms{200};  //!< iot_button SHORT_TICKS equivalent
        uint32_t long_press_ms{400};   //!< iot_button LONG_TICKS equivalent
    };

    ButtonEventDetector() : _cfg()
    {
    }

    explicit ButtonEventDetector(const config_t& cfg) : _cfg(cfg)
    {
    }

    inline void reset()
    {
        _state  = State::Idle;
        _repeat = 0;
        _t0     = 0U;
    }

    //! @brief Feed a frame edge. pressed=true for PRESS, false for RELEASE.
    //! @return Immediate event (PressDown/PressUp/LongPressUp) or None.
    //! @note A PRESS for an already-down key (hardware repeat) returns None.
    ButtonEvent edge(const bool pressed, const uint32_t now_ms)
    {
        switch (_state) {
            case State::Idle:
                if (pressed) {
                    _repeat = 1;
                    _t0     = now_ms;
                    _state  = State::Down;
                    return ButtonEvent::PressDown;
                }
                return ButtonEvent::None;

            case State::Down:
                if (!pressed) {
                    _t0    = now_ms;
                    _state = State::WaitRepeat;
                    return ButtonEvent::PressUp;
                }
                return ButtonEvent::None;  // hardware repeat — ignore

            case State::WaitRepeat:
                if (pressed) {
                    ++_repeat;
                    _t0    = now_ms;
                    _state = State::RepeatDown;
                    return ButtonEvent::PressDown;
                }
                return ButtonEvent::None;

            case State::RepeatDown:
                if (!pressed) {
                    _t0    = now_ms;
                    _state = State::WaitRepeat;
                    return ButtonEvent::PressUp;
                }
                return ButtonEvent::None;

            case State::LongHold:
                if (!pressed) {
                    _state = State::Idle;
                    return ButtonEvent::LongPressUp;
                }
                return ButtonEvent::None;
        }
        return ButtonEvent::None;
    }

    //! @brief Advance time. Call every tick.
    //! @return Time-resolved event (SingleClick/DoubleClick/LongPressStart) or None.
    ButtonEvent poll(const uint32_t now_ms)
    {
        switch (_state) {
            case State::Down:
                if (now_ms - _t0 >= _cfg.long_press_ms) {
                    _state = State::LongHold;
                    return ButtonEvent::LongPressStart;
                }
                return ButtonEvent::None;

            case State::WaitRepeat:
                if (now_ms - _t0 > _cfg.short_press_ms) {
                    const uint8_t repeat = _repeat;
                    _repeat              = 0;
                    _state               = State::Idle;
                    if (repeat == 1) return ButtonEvent::SingleClick;
                    if (repeat == 2) return ButtonEvent::DoubleClick;
                    return ButtonEvent::None;
                }
                return ButtonEvent::None;

            default:
                return ButtonEvent::None;
        }
    }

private:
    enum class State : uint8_t { Idle, Down, WaitRepeat, RepeatDown, LongHold };

    config_t _cfg{};
    State _state{State::Idle};
    uint8_t _repeat{0};
    uint32_t _t0{0};
};
/// @endcond

}  // namespace keyboard_bitwise
}  // namespace unit
}  // namespace m5

#endif
