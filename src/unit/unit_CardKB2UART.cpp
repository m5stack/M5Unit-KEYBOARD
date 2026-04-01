/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB2UART.cpp
  @brief CardKB2 Unit for M5UnitUnified (UART mode)
*/
#include "unit_CardKB2UART.hpp"
#include <M5Utility.hpp>
#include <algorithm>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::keyboard;
using namespace m5::unit::keyboard::command;
using m5::unit::UnitCardKB2UART;
using Packet = m5::unit::UnitCardKB2UART::Packet;

namespace {

// key_map used by UART update for character lookup
constexpr uint8_t key_map[][4 /* mode: normal, shift, sym, fn */] = {
    // key_id = line *11+column, line range(0~3), column range(0~10)
    // line1
    {'1', '1', '!', 27},   // 0: 1 (Fn+1=ESC)
    {'2', '2', '@', 128},  // 1: 2
    {'3', '3', '#', 129},  // 2: 3
    {'4', '4', '$', 130},  // 3: 4
    {'5', '5', '%', 131},  // 4: 5
    {'6', '6', '^', 132},  // 5: 6
    {'7', '7', '&', 133},  // 6: 7
    {'8', '8', '*', 134},  // 7: 8
    {'9', '9', '(', 135},  // 8: 9
    {'0', '0', ')', 136},  // 9: 0
    // line2
    {0, 0, 0, 0},              // 10: no key
    {'q', 'Q', '~', 137},      // 11: q
    {'w', 'W', '`', 138},      // 12: w
    {'e', 'E', '?', 139},      // 13: e
    {'r', 'R', '\\', 140},     // 14: r
    {'t', 'T', '/', 141},      // 15: t
    {'y', 'Y', '|', 142},      // 16: y
    {'u', 'U', '_', 143},      // 17: u
    {'i', 'I', '-', 144},      // 18: i
    {'o', 'O', '+', 145},      // 19: o
    {'p', 'P', '=', 146},      // 20: p
    {0x08, 0x08, 0x08, 0x08},  // 21: delete
    // line3
    {0, 0, 0, 0},              // 22: Aa (cap lock toggle, no char)
    {'a', 'A', '{', 148},      // 23: a
    {'s', 'S', '}', 149},      // 24: s
    {'d', 'D', '^', 181},      // 25: d (Fn+D=UP)
    {'f', 'F', '[', 150},      // 26: f
    {'g', 'G', ']', 151},      // 27: g
    {'h', 'H', '"', 152},      // 28: h
    {'j', 'J', '\'', 153},     // 29: j
    {'k', 'K', ';', 154},      // 30: k
    {'l', 'L', ':', 155},      // 31: l
    {0x0A, 0x0A, 0x0A, 0x0A},  // 32: enter
    // line4
    {0, 0, 0, 0},          // 33: fn (function key, no char)
    {0, 0, 0, 0},          // 34: sym (symbol key, no char)
    {'z', 'Z', 'z', 180},  // 35: z (Fn+Z=LEFT)
    {'x', 'X', 'x', 182},  // 36: x (Fn+X=DOWN)
    {'c', 'C', 'c', 183},  // 37: c (Fn+C=RIGHT)
    {'v', 'V', '<', 157},  // 38: v
    {'b', 'B', '>', 158},  // 39: b
    {'n', 'N', ',', 159},  // 40: n
    {'m', 'M', '.', 160},  // 41: m
    {' ', ' ', ' ', 161}   // 42: space
};
static_assert(m5::stl::size(key_map) == m5::unit::cardkb2::NUMBER_OF_KEYS, "Invalid size");

constexpr uint16_t PACKET_HEADER{0xAA};
enum PID : uint8_t {
    PID_DATA_LEN = 0x03,
};

enum KeyState : uint8_t {
    KEY_STATE_PRESSED  = 0x01,
    KEY_STATE_RELEASED = 0x02,
};

constexpr uint32_t CAPS_DOUBLE_CLICK_WINDOW_MS = 280;
constexpr uint32_t CAPS_HOLD_THRESHOLD_MS      = 350;

bool is_valid_ack(const Packet& p)
{
    if (p[0] != PACKET_HEADER || p[1] != PID_DATA_LEN) {
        return false;
    }
    if (p[3] != KEY_STATE_PRESSED && p[3] != KEY_STATE_RELEASED) {
        return false;
    }
    const uint8_t checksum = static_cast<uint8_t>((p[1] + p[2] + p[3]) & 0xFF);
    return p[4] == checksum;
}

}  // namespace

namespace m5 {
namespace unit {

const char UnitCardKB2UART::name[] = "UnitCardKB2UART";
const types::uid_t UnitCardKB2UART::uid{"UnitCardKB2UART"_mmh3};
const types::attr_t UnitCardKB2UART::attr{attribute::AccessUART};

bool UnitCardKB2UART::begin()
{
    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<uint8_t>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    auto uart = asAdapter<AdapterUART>(Adapter::Type::UART);
    if (!uart) {
        M5_LIB_LOGE("Illegal adapter: UnitCardKB2UART requires UART adapter");
        return false;
    }

    uart->setTimeout(10);
    uart->flushRX();
    _mode             = Mode::M5UnitUnified;
    _firmware_version = 0;  // UART protocol has no version query command

    return UnitKeyboardBitwise::begin() && (_cfg.start_periodic ? startPeriodicMeasurement(_cfg.interval) : true);
}

bool UnitCardKB2UART::readFirmwareVersion(uint8_t& ver)
{
    M5_LIB_LOGW("readFirmwareVersion is not available in UART mode");
    ver = 0;
    return false;
}

uint8_t UnitCardKB2UART::read_data(Packet& rbuf)
{
    rbuf.fill(0);

    // Sync to frame header first to avoid permanent desynchronization.
    if (readWithTransaction(rbuf.data(), 1) != m5::hal::error::error_t::OK || rbuf[0] != PACKET_HEADER) {
        return 0;
    }

    if (readWithTransaction(rbuf.data() + 1, 4) == m5::hal::error::error_t::OK && is_valid_ack(rbuf)) {
        return rbuf[3];  // key state
    }
    return 0;
}

void UnitCardKB2UART::update_uart(const bool force)
{
    auto at = m5::utility::millis();
    if (!(force || !_latest || at >= _latest + _interval)) {
        return;
    }

    // Expire click sequence window.
    if (!_caps_pressing && _caps_click_count && (at - _caps_last_release_at) > CAPS_DOUBLE_CLICK_WINDOW_MS) {
        _caps_click_count = 0;
    }

    // While keeping Caps key pressed, enable temporary uppercase.
    if (_caps_pressing && !_caps_hold_active && (at - _caps_pressed_at) >= CAPS_HOLD_THRESHOLD_MS) {
        _caps_hold_active = true;
        _caps_shift_once  = false;
        _caps_lock        = false;
        _caps_click_count = 0;
    }

    _updated          = false;
    _prev             = _now;
    auto prev_holding = _holding;
    _wasPressed = _wasReleased = _wasHold = 0;

    // Consume all available UART packets to update _now bits
    while (true) {
        Packet rbuf{};
        const uint8_t state = read_data(rbuf);
        if (!state) {
            break;
        }

        const uint8_t kidx = rbuf[2];
        M5_LIB_LOGV("UART kidx:%d state:%s", kidx,
                    state == KEY_STATE_PRESSED    ? "PRESS"
                    : state == KEY_STATE_RELEASED ? "RELEASE"
                                                  : "UNKNOWN");
        if (kidx >= cardkb2::NUMBER_OF_KEYS) {
            continue;
        }

        const uint64_t bit = (1ULL << kidx);

        if (state == KEY_STATE_PRESSED) {
            if (!(_now & bit)) {
                // Initial press only (not hardware repeat)
                _now |= bit;

                if (kidx == cardkb2::KEY_SYM) {
                    _sym_was_pressed = !_sym_was_pressed;
                }

                if (kidx == cardkb2::KEY_AA) {
                    _caps_pressing   = true;
                    _caps_pressed_at = at;
                }

                _repeat_start_at[kidx] = _hold_start_at[kidx] = at;
            }
            // Hardware repeat packets are consumed but ignored — software repeat handles timing
        } else if (state == KEY_STATE_RELEASED) {
            _now &= ~bit;
            _holding &= ~bit;

            if (kidx == cardkb2::KEY_AA && _caps_pressing) {
                _caps_pressing = false;

                if (_caps_hold_active) {
                    _caps_hold_active = false;
                    _caps_shift_once  = false;
                    _caps_lock        = false;
                    _caps_click_count = 0;
                } else {
                    if (_caps_click_count == 1 && (at - _caps_last_release_at) <= CAPS_DOUBLE_CLICK_WINDOW_MS) {
                        _caps_lock        = !_caps_lock;
                        _caps_shift_once  = false;
                        _caps_click_count = 0;
                    } else {
                        if (_caps_lock) {
                            _caps_lock       = false;
                            _caps_shift_once = false;
                        } else {
                            _caps_shift_once = true;
                        }
                        _caps_click_count     = 1;
                        _caps_last_release_at = at;
                    }
                }
            }
        }
    }

    // Compute press/release transitions
    _wasPressed  = (_now ^ _prev) & _now;
    _wasReleased = (_now ^ _prev) & ~_now;

    // Push characters for newly pressed keys
    uint64_t wp = _wasPressed;
    while (wp) {
        uint_fast8_t kidx = __builtin_ctzll(wp);
        wp &= wp - 1;  // clear lowest set bit

        // Skip modifier keys
        if (kidx == cardkb2::KEY_AA || kidx == cardkb2::KEY_FN || kidx == cardkb2::KEY_SYM) {
            continue;
        }

        const bool fn_active   = _now & (1ULL << cardkb2::KEY_FN);
        const bool caps_active = (_caps_lock || _caps_hold_active || _caps_shift_once);
        uint8_t ch;
        if (fn_active) {
            ch = key_map[kidx][3];
        } else if (_sym_was_pressed) {
            ch = key_map[kidx][2];
        } else {
            ch = key_map[kidx][caps_active ? 1 : 0];
        }

        if (ch) {
            _data->push_back(ch);
        }

        // One-shot uppercase is consumed by the next non-caps key.
        if (_caps_shift_once && !_caps_lock && !_caps_hold_active) {
            _caps_shift_once = false;
        }
    }

    // Software repeat and hold (same logic as CardKB)
    _repeating   = 0;
    uint64_t bit = 1;
    for (uint_fast8_t i = 0; i < cardkb2::NUMBER_OF_KEYS; ++i, bit <<= 1) {
        if (!(_now & bit) || (_wasPressed & bit)) {
            continue;  // not held or just pressed
        }
        // Skip modifier keys — they don't repeat
        if (i == cardkb2::KEY_AA || i == cardkb2::KEY_FN || i == cardkb2::KEY_SYM) {
            continue;
        }
        // Repeat?
        if (at - _repeat_start_at[i] >= _repeating_threshold) {
            _repeat_start_at[i] = at;
            _repeating |= bit;

            // Push repeat character
            if (i != cardkb2::KEY_AA && i != cardkb2::KEY_FN && i != cardkb2::KEY_SYM) {
                const bool fn_active   = _now & (1ULL << cardkb2::KEY_FN);
                const bool caps_active = (_caps_lock || _caps_hold_active || _caps_shift_once);
                uint8_t ch;
                if (fn_active) {
                    ch = key_map[i][3];
                } else if (_sym_was_pressed) {
                    ch = key_map[i][2];
                } else {
                    ch = key_map[i][caps_active ? 1 : 0];
                }
                if (ch) {
                    _data->push_back(ch);
                }
            }
        }
        // Hold?
        if (at - _hold_start_at[i] >= _holding_threshold) {
            _holding |= bit;
        } else {
            _holding &= ~bit;
        }
    }
    _wasHold = (prev_holding ^ _holding) & _holding;

    // Synthesize modifier bits in upper byte for isModifier()/isShift()/isSymbol()/isFunction()
    _now &= 0x00FFFFFFFFFFFFFFULL;  // Clear modifier byte
    if (_caps_lock || _caps_hold_active || _caps_shift_once || (_now & (1ULL << cardkb2::KEY_AA))) {
        _now |= keyboard::MODIFIER_SHIFT_BIT;
    }
    if (_sym_was_pressed) {
        _now |= keyboard::MODIFIER_SYMBOL_BIT;
    }
    if (_now & (1ULL << cardkb2::KEY_FN)) {
        _now |= keyboard::MODIFIER_FUNCTION_BIT;
    }

    _updated = (_wasPressed | _wasReleased | _repeating);
    if (_updated) {
        _latest = at;
    }
}

void UnitCardKB2UART::update(const bool force)
{
    if (!inPeriodic()) {
        return;
    }
    update_uart(force);
}

}  // namespace unit
}  // namespace m5
