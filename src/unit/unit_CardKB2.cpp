/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB2.cpp
  @brief CardKB2 Unit for M5UnitUnified
*/
#include "unit_CardKB2.hpp"
#include <M5Utility.hpp>
#include <algorithm>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::keyboard;
using namespace m5::unit::keyboard::command;
using m5::unit::UnitCardKB2;
using Packet = m5::unit::UnitCardKB2::Packet;

namespace {
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
static_assert(m5::stl::size(key_map) == UnitCardKB2::NUMBER_OF_KEYS, "Invalid size");

// modifier bit to key_map mode index
constexpr uint8_t mod_table[] = {1, 0, 3, 2};  // 0x01:Shift, 0x80:Symbol 0x40:Fucntion

// ASCII to mode bit and key_index_t
// 1:normal 2:shift 4:symbol 8:function
// 0x0: no key, 0xFF: invalid char
constexpr std::pair<uint8_t, key_index_t> character_map[] = {
    {0x00, 0xFF},                         // NULL (0)
    {0x00, 0xFF},                         // SOH (1)
    {0x00, 0xFF},                         // STX (2)
    {0x00, 0xFF},                         // ETX (3)
    {0x00, 0xFF},                         // EOT (4)
    {0x00, 0xFF},                         // ENG (5)
    {0x00, 0xFF},                         // ACK (6)
    {0x00, 0xFF},                         // BEL (7)
    {1 + 4, UnitCardKB2::KEY_DELETE},     // BS (8)
    {0x00, 0xFF},                         // HT (9)
    {1 + 2 + 4, UnitCardKB2::KEY_ENTER},  // LF (10)
    {0x00, 0xFF},                         // VT (11)
    {0x00, 0xFF},                         // FF (12)
    {1 + 2 + 4, UnitCardKB2::KEY_ENTER},  // CR (13)
    {0x00, 0xFF},                         // SO (14)
    {0x00, 0xFF},                         // SI (15)
    {0x00, 0xFF},                         // DLE (16)
    {0x00, 0xFF},                         // DC1 (17)
    {0x00, 0xFF},                         // DC2 (18)
    {0x00, 0xFF},                         // DC3 (19)
    {0x00, 0xFF},                         // DC4 (20)
    {0x00, 0xFF},                         // NAK (21)
    {0x00, 0xFF},                         // SYN (22)
    {0x00, 0xFF},                         // ETB (23)
    {0x00, 0xFF},                         // CAN (24)
    {0x00, 0xFF},                         // EM (25)
    {0x00, 0xFF},                         // SUB (26)
    {0x00, 0xFF},                         // ESC (27)
    {0x00, 0xFF},                         // FS (28)
    {0x00, 0xFF},                         // GS (29)
    {0x00, 0xFF},                         // RS (30)
    {0x00, 0xFF},                         // US (31)
    {1 + 2 + 4, UnitCardKB2::KEY_SPACE},  // SP (32)
    {4, UnitCardKB2::KEY_1},              // ! (33)
    {4, UnitCardKB2::KEY_P},              // " (34)
    {4, UnitCardKB2::KEY_3},              // # (35)
    {4, UnitCardKB2::KEY_4},              // $ (36)
    {4, UnitCardKB2::KEY_5},              // % (37)
    {4, UnitCardKB2::KEY_7},              // & (38)
    {4, UnitCardKB2::KEY_J},              // ' (39)
    {4, UnitCardKB2::KEY_9},              // ( (40)
    {4, UnitCardKB2::KEY_0},              // ) (41)
    {4, UnitCardKB2::KEY_8},              // * (42)
    {4, UnitCardKB2::KEY_O},              // + (43)
    {1 + 2, UnitCardKB2::KEY_N},          // , (44)
    {4, UnitCardKB2::KEY_H},              // - (45)
    {1 + 2, UnitCardKB2::KEY_M},          // . (46)
    {4, UnitCardKB2::KEY_Y},              // / (47)
    {1 + 2, UnitCardKB2::KEY_0},          // 0 (48)
    {1 + 2, UnitCardKB2::KEY_1},          // 1 (49)
    {1 + 2, UnitCardKB2::KEY_2},          // 2 (50)
    {1 + 2, UnitCardKB2::KEY_3},          // 3 (51)
    {1 + 2, UnitCardKB2::KEY_4},          // 4 (52)
    {1 + 2, UnitCardKB2::KEY_5},          // 5 (53)
    {1 + 2, UnitCardKB2::KEY_6},          // 6 (54)
    {1 + 2, UnitCardKB2::KEY_7},          // 7 (55)
    {1 + 2, UnitCardKB2::KEY_8},          // 8 (56)
    {1 + 2, UnitCardKB2::KEY_9},          // 9 (57)
    {4, UnitCardKB2::KEY_K},              // : (58)
    {4, UnitCardKB2::KEY_S},              // ; (59)
    {4, UnitCardKB2::KEY_V},              // < (60)
    {4, UnitCardKB2::KEY_P},              // = (61)
    {4, UnitCardKB2::KEY_B},              // > (62)
    {4, UnitCardKB2::KEY_E},              // ? (63)
    {4, UnitCardKB2::KEY_2},              // @ (64)
    {2, UnitCardKB2::KEY_A},              // A (65)
    {2, UnitCardKB2::KEY_B},              // B (66)
    {2, UnitCardKB2::KEY_C},              // C (67)
    {2, UnitCardKB2::KEY_D},              // D (68)
    {2, UnitCardKB2::KEY_E},              // E (69)
    {2, UnitCardKB2::KEY_F},              // F (70)
    {2, UnitCardKB2::KEY_G},              // G (71)
    {2, UnitCardKB2::KEY_H},              // H (72)
    {2, UnitCardKB2::KEY_I},              // I (73)
    {2, UnitCardKB2::KEY_J},              // J (74)
    {2, UnitCardKB2::KEY_K},              // K (75)
    {2, UnitCardKB2::KEY_L},              // L (76)
    {2, UnitCardKB2::KEY_M},              // M (77)
    {2, UnitCardKB2::KEY_N},              // N (78)
    {2, UnitCardKB2::KEY_O},              // O (79)
    {2, UnitCardKB2::KEY_P},              // P (80)
    {2, UnitCardKB2::KEY_Q},              // Q (81)
    {2, UnitCardKB2::KEY_R},              // R (82)
    {2, UnitCardKB2::KEY_S},              // S (83)
    {2, UnitCardKB2::KEY_T},              // T (84)
    {2, UnitCardKB2::KEY_U},              // U (85)
    {2, UnitCardKB2::KEY_V},              // V (86)
    {2, UnitCardKB2::KEY_W},              // W (87)
    {2, UnitCardKB2::KEY_X},              // X (88)
    {2, UnitCardKB2::KEY_Y},              // Y (89)
    {2, UnitCardKB2::KEY_Z},              // Z (90)
    {4, UnitCardKB2::KEY_F},              // [ (91)
    {4, UnitCardKB2::KEY_R},              // \ (92)
    {4, UnitCardKB2::KEY_G},              // ] (93)
    {4, UnitCardKB2::KEY_D},              // ^ (94)
    {4, UnitCardKB2::KEY_I},              // _ (95)
    {4, UnitCardKB2::KEY_W},              // ` (96)
    {1, UnitCardKB2::KEY_A},              // a (97)
    {1, UnitCardKB2::KEY_B},              // b (98)
    {1, UnitCardKB2::KEY_C},              // c (99)
    {1, UnitCardKB2::KEY_D},              // d (100)
    {1, UnitCardKB2::KEY_E},              // e (101)
    {1, UnitCardKB2::KEY_F},              // f (102)
    {1, UnitCardKB2::KEY_G},              // g (103)
    {1, UnitCardKB2::KEY_H},              // h (104)
    {1, UnitCardKB2::KEY_I},              // i (105)
    {1, UnitCardKB2::KEY_J},              // j (106)
    {1, UnitCardKB2::KEY_K},              // k (107)
    {1, UnitCardKB2::KEY_L},              // l (108)
    {1, UnitCardKB2::KEY_M},              // m (109)
    {1, UnitCardKB2::KEY_N},              // n (110)
    {1, UnitCardKB2::KEY_O},              // o (111)
    {1, UnitCardKB2::KEY_P},              // p (112)
    {1, UnitCardKB2::KEY_Q},              // q (113)
    {1, UnitCardKB2::KEY_R},              // r (114)
    {1, UnitCardKB2::KEY_S},              // s (115)
    {1, UnitCardKB2::KEY_T},              // t (116)
    {1, UnitCardKB2::KEY_U},              // u (117)
    {1, UnitCardKB2::KEY_V},              // v (118)
    {1, UnitCardKB2::KEY_W},              // w (119)
    {1, UnitCardKB2::KEY_X},              // x (120)
    {1, UnitCardKB2::KEY_Y},              // y (121)
    {1, UnitCardKB2::KEY_Z},              // z (122)
    {4, UnitCardKB2::KEY_A},              // { (123)
    {4, UnitCardKB2::KEY_U},              // | (124)
    {4, UnitCardKB2::KEY_S},              // } (125)
    {4, UnitCardKB2::KEY_Q},              // ~ (126)
    {2, UnitCardKB2::KEY_DELETE},         // DEL (127)
};
static_assert(m5::stl::size(character_map) == 128, "Invalid size");

constexpr std::pair<uint8_t, key_index_t> special_character_map[] = {
    {1 + 2 + 4, UnitCardKB2::KEY_Z},  // Left cursor (Fn+Z)
    {1 + 2 + 4, UnitCardKB2::KEY_D},  // Up cursor (Fn+D)
    {1 + 2 + 4, UnitCardKB2::KEY_X},  // Down cursor (Fn+X)
    {1 + 2 + 4, UnitCardKB2::KEY_C},  // Right cursor (Fn+C)
};

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
    if (p.size() != 5 || p[0] != PACKET_HEADER || p[1] != PID_DATA_LEN) {
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

// class UnitCardKB
const char UnitCardKB2::name[] = "UnitCardKB2";
const types::uid_t UnitCardKB2::uid{"UnitCardKB2"_mmh3};
const types::attr_t UnitCardKB2::attr{attribute::AccessI2C | attribute::AccessUART};

key_index_t UnitCardKB2::character_to_key_index(const char ch)
{
    unsigned char uc = ch;
    // function (>= 0x80)
    if (uc & 0x80) {
        key_index_t kidx = (key_index_t)(uc - 0x80);
        // Special key?
        if (uc >= SCHAR_LEFT && uc <= SCHAR_RIGHT) {
            return special_character_map[uc - SCHAR_LEFT].second;
        }
        return static_cast<key_index_t>((kidx < m5::stl::size(key_map)) ? kidx : 0xFF);
    }
    // normal,shift or symbol
    return static_cast<key_index_t>((uc < m5::stl::size(character_map)) ? (character_map[uc].second) : 0xFF);
}

uint8_t UnitCardKB2::character_to_mode_bits(const char ch)
{
    unsigned char uc = ch;
    // function? (>= 0x80)
    if (uc & 0x80) {
        key_index_t kidx = (key_index_t)(uc - 0x80);
        // Special key?
        if (uc >= SCHAR_LEFT && uc <= SCHAR_RIGHT) {
            // M5_LIB_LOGI("%c => %02X", ch, special_character_map[uc - SCHAR_LEFT].first);
            return special_character_map[uc - SCHAR_LEFT].first;
        }

        // M5_LIB_LOGI("%c => %02X", ch, (kidx < m5::stl::size(key_map)) ? 0x08 : 0x00);
        return (kidx < m5::stl::size(key_map)) ? 0x08 : 0x00;
    }
    // normal,shift or symbol
    // M5_LIB_LOGI("%c => %02X", ch, (uc < m5::stl::size(character_map)) ? (character_map[uc].first) : 0x00);
    return (uc < m5::stl::size(character_map)) ? (character_map[uc].first) : 0x00;
}

bool UnitCardKB2::begin()
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
    if (uart) {
        uart->setTimeout(10);
        uart->flushRX();
        _mode             = Mode::M5UnitUnified;
        _firmware_version = 0;
        return _cfg.start_periodic ? startPeriodicMeasurement(_cfg.interval) : true;
    }

    auto i2c = asAdapter<AdapterI2C>(Adapter::Type::I2C);
    if (!i2c) {
        M5_LIB_LOGE("Illegal adapter");
        return false;
    }

    uint8_t discard{};
    // Read and reject values to avoid false evaluations due to transmission of values for released keys
    readWithTransaction(&discard, 1);
    auto reg                 = registerMap();
    reg.firmware_version_reg = 0xF1;
    registerMap(reg);
    readFirmwareVersion(_firmware_version);
    M5_LIB_LOGI("Type:%S Firmware:%02X", "CardKB2", _firmware_version);

    if (!firmwareVersion()) {
        return false;
    }
    return UnitKeyboardBitwise::begin() && (_cfg.start_periodic ? startPeriodicMeasurement(_cfg.interval) : true);
}

uint8_t UnitCardKB2::read_data(Packet& rbuf)
{
    rbuf.clear();
    rbuf.resize(1);

    // Sync to frame header first to avoid permanent desynchronization.
    if (readWithTransaction(rbuf.data(), 1) != m5::hal::error::error_t::OK || rbuf[0] != PACKET_HEADER) {
        return 0;
    }

    rbuf.resize(5);
    if (readWithTransaction(rbuf.data() + 1, 4) == m5::hal::error::error_t::OK && is_valid_ack(rbuf)) {
        return rbuf[3];  // key state
    }
    return 0;
}

void UnitCardKB2::update(const bool force)
{
    if (!inPeriodic()) {
        return;
    }

    if (asAdapter<AdapterUART>(Adapter::Type::UART)) {
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

        _updated    = false;
        _prev       = _now;
        _wasPressed = _wasReleased = _wasHold = 0;
        _holding = _repeating = 0;

        Packet rbuf{};
        const uint8_t state = read_data(rbuf);
        if (!state || rbuf.size() != 5) {
            return;
        }

        const uint8_t kidx = rbuf[2];
        M5_LIB_LOGI("kidx:%d", kidx);
        if (kidx >= NUMBER_OF_KEYS) {
            return;
        }

        const uint64_t bit = (1ULL << kidx);
        uint8_t ch         = 0;
        if (state == KEY_STATE_PRESSED) {
            _now |= bit;

            if (kidx == 34) {  // sym pressed
                _sym_was_pressed = !_sym_was_pressed;
            }

            if (kidx == 22) {  // caps key
                _caps_pressing   = true;
                _caps_pressed_at = at;
            } else {
                const bool caps_active = (_caps_lock || _caps_hold_active || _caps_shift_once);
                if (!_sym_was_pressed) {
                    ch = key_map[kidx][caps_active ? 1 : 0];
                } else {
                    ch = key_map[kidx][2];
                }

                if (ch) {
                    _data->push_back(ch);
                }

                // One-shot uppercase is consumed by the next non-caps key.
                if (_caps_shift_once && !_caps_lock && !_caps_hold_active) {
                    _caps_shift_once = false;
                }
            }
        } else if (state == KEY_STATE_RELEASED) {
            _now &= ~bit;

            if (kidx == 22 && _caps_pressing) {
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
        } else {
            return;
        }

        _wasPressed  = (_now ^ _prev) & _now;
        _wasReleased = (_now ^ _prev) & ~_now;
        _updated     = (_wasPressed | _wasReleased);
        if (_updated) {
            _latest = at;
        }
        return;
    }

    UnitKeyboardBitwise::update(force);
}

}  // namespace unit
}  // namespace m5
