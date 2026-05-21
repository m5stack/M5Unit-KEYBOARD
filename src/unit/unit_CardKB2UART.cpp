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
using m5::unit::keyboard_bitwise::ButtonEvent;

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

    _updated = false;
    // Snapshot prior `now` into `prev` BEFORE this tick mutates `now`, so external
    // consumers can observe `nowBits() != previousBits()` across the transition
    // (SimpleDisplay's release-edge redraw relies on this).
    _state.commitPrev();
    _state.resetOneShot();

    // Work with a uint64_t view of the live bitmap so we can apply UART event deltas incrementally,
    // then sync back to _state.now / _state.holding once all events are consumed.
    const uint64_t prev_bits    = _state.now.to_ullong();
    uint64_t now_bits           = prev_bits;
    uint64_t holding_bits       = _state.holding.to_ullong();
    const uint64_t prev_holding = holding_bits;

    // Consume all available UART packets to update now_bits
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
            if (!(now_bits & bit)) {
                // Initial press only (not hardware repeat)
                now_bits |= bit;

                if (kidx == cardkb2::KEY_SYM) {
                    const ButtonEvent ev = _sym_detector.edge(true, static_cast<uint32_t>(at));
                    if (ev == ButtonEvent::SingleClick) _sym_mode = !_sym_mode;
                }

                if (kidx == cardkb2::KEY_AA) {
                    const ButtonEvent ev = _caps_detector.edge(true, static_cast<uint32_t>(at));
                    if (ev == ButtonEvent::PressDown && !_caps_lock) _caps_hold_active = true;
                }

                _state.press_at[kidx]       = at;
                _state.last_repeat_at[kidx] = 0U;
            }
            // CardKB2 firmware (300ms/50ms hard-coded) re-sends KEY_STATE=0x01 every 50ms after 300ms hold.
            // PRESS and REPEAT share the same state code (0x01) so they cannot be distinguished on the wire.
            // The rising-edge guard above (`if (!(now_bits & bit))`) discards these repeats, letting the
            // library's software repeat (driven by config_t::repeating_threshold) own the timing —
            // this also keeps I2C/UART/ESP-NOW UX consistent and user-tunable.
        } else if (state == KEY_STATE_RELEASED) {
            now_bits &= ~bit;
            holding_bits &= ~bit;

            if (kidx == cardkb2::KEY_SYM) {
                const ButtonEvent ev = _sym_detector.edge(false, static_cast<uint32_t>(at));
                if (ev == ButtonEvent::SingleClick) _sym_mode = !_sym_mode;
            }

            if (kidx == cardkb2::KEY_AA) {
                const ButtonEvent ev = _caps_detector.edge(false, static_cast<uint32_t>(at));
                if ((ev == ButtonEvent::PressUp || ev == ButtonEvent::LongPressUp) && !_caps_lock) {
                    _caps_hold_active = false;
                }
            }
        }
    }

    {
        const ButtonEvent ev = _sym_detector.poll(static_cast<uint32_t>(at));
        if (ev == ButtonEvent::SingleClick) _sym_mode = !_sym_mode;
    }
    {
        const ButtonEvent ev = _caps_detector.poll(static_cast<uint32_t>(at));
        if (ev == ButtonEvent::SingleClick) {
            if (_caps_lock) {
                _caps_lock       = false;
                _caps_shift_once = false;
            } else {
                _caps_shift_once = true;
            }
        } else if (ev == ButtonEvent::DoubleClick) {
            _caps_lock       = !_caps_lock;
            _caps_shift_once = false;
        }
    }

    // Compute press/release transitions against the snapshot taken at the start of this tick.
    const uint64_t was_pressed_bits  = (now_bits ^ prev_bits) & now_bits;
    const uint64_t was_released_bits = (now_bits ^ prev_bits) & ~now_bits;

    // Push characters for newly pressed keys
    uint64_t wp = was_pressed_bits;
    while (wp) {
        uint_fast8_t kidx = __builtin_ctzll(wp);
        wp &= wp - 1;  // clear lowest set bit

        // Skip modifier keys
        if (kidx == cardkb2::KEY_AA || kidx == cardkb2::KEY_FN || kidx == cardkb2::KEY_SYM) {
            continue;
        }

        const bool fn_active   = (now_bits & (1ULL << cardkb2::KEY_FN)) != 0U;
        const bool caps_active = (_caps_lock || _caps_hold_active || _caps_shift_once);
        uint8_t ch;
        if (fn_active) {
            ch = key_map[kidx][3];
        } else if (_sym_mode || (now_bits & (1ULL << cardkb2::KEY_SYM)) != 0U) {
            ch = key_map[kidx][2];
        } else {
            ch = key_map[kidx][caps_active ? 1 : 0];
        }

        if (ch) {
            _data->push_back(ch);
        }

        // One-shot uppercase is consumed by the next alphabetic key (firmware parity).
        if (_caps_shift_once && !_caps_lock && !_caps_hold_active &&
            ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))) {
            _caps_shift_once = false;
        }
    }

    // Software repeat and hold (mirrors CardKB legacy behaviour: rate == initial threshold).
    uint64_t repeating_bits = 0;
    uint64_t bit            = 1;
    for (uint_fast8_t i = 0; i < cardkb2::NUMBER_OF_KEYS; ++i, bit <<= 1) {
        if (!(now_bits & bit) || (was_pressed_bits & bit)) {
            continue;  // not held or just pressed
        }
        // Skip modifier keys — they don't repeat
        if (i == cardkb2::KEY_AA || i == cardkb2::KEY_FN || i == cardkb2::KEY_SYM) {
            continue;
        }
        // Repeat?
        if (at - _state.press_at[i] >= _state.repeat_initial_ms) {
            _state.press_at[i] = at;
            repeating_bits |= bit;

            // Push repeat character
            const bool fn_active   = (now_bits & (1ULL << cardkb2::KEY_FN)) != 0U;
            const bool caps_active = (_caps_lock || _caps_hold_active || _caps_shift_once);
            uint8_t ch;
            if (fn_active) {
                ch = key_map[i][3];
            } else if (_sym_mode || (now_bits & (1ULL << cardkb2::KEY_SYM)) != 0U) {
                ch = key_map[i][2];
            } else {
                ch = key_map[i][caps_active ? 1 : 0];
            }
            if (ch) {
                _data->push_back(ch);
            }
        }
        // Hold?
        if (at - _state.press_at[i] >= _state.holding_threshold_ms) {
            holding_bits |= bit;
        } else {
            holding_bits &= ~bit;
        }
    }
    const uint64_t was_hold_bits = (prev_holding ^ holding_bits) & holding_bits;

    // Synthesize modifier bits in upper byte for isModifier()/isShift()/isSymbol()/isFunction()
    now_bits &= 0x00FFFFFFFFFFFFFFULL;  // Clear modifier byte
    if (_caps_lock || _caps_hold_active || _caps_shift_once || (now_bits & (1ULL << cardkb2::KEY_AA))) {
        now_bits |= keyboard::MODIFIER_SHIFT_BIT;
    }
    if (_sym_mode || (now_bits & (1ULL << cardkb2::KEY_SYM)) != 0U) {
        now_bits |= keyboard::MODIFIER_SYMBOL_BIT;
    }
    if (now_bits & (1ULL << cardkb2::KEY_FN)) {
        now_bits |= keyboard::MODIFIER_FUNCTION_BIT;
    }

    // Sync the local uint64_t views back into the shared bitwise state.
    _state.now       = std::bitset<64>(now_bits);
    _state.holding   = std::bitset<64>(holding_bits);
    _state.pressed   = std::bitset<64>(was_pressed_bits);
    _state.released  = std::bitset<64>(was_released_bits);
    _state.repeating = std::bitset<64>(repeating_bits);
    _state.was_hold  = std::bitset<64>(was_hold_bits);

    _updated = (was_pressed_bits | was_released_bits | repeating_bits) != 0U;
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
