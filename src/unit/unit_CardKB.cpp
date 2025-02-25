/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB.cpp
  @brief CardKB Unit for M5UnitUnified
*/
#include "unit_CardKB.hpp"
#include <M5Utility.hpp>
#include <algorithm>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::keyboard;
using namespace m5::unit::keyboard::command;
using namespace m5::unit::cardkb;
using namespace m5::unit::cardkb::command;
using m5::unit::UnitCardKB;

namespace {
constexpr uint8_t key_map[][4 /* mode: normal, shift, sym, fn */] = {
    {27, 27, 27, 128},      // esc
    {'1', '1', '!', 129},   // 1
    {'2', '2', '@', 130},   // 2
    {'3', '3', '#', 131},   // 3
    {'4', '4', '$', 132},   // 4
    {'5', '5', '%', 133},   // 5
    {'6', '6', '^', 134},   // 6
    {'7', '7', '&', 135},   // 7
    {'8', '8', '*', 136},   // 8
    {'9', '9', '(', 137},   // 9
    {'0', '0', ')', 138},   // 0
    {8, 127, 8, 139},       // bs/del
    {9, 9, 9, 140},         // tab
    {'q', 'Q', '{', 141},   // q
    {'w', 'W', '}', 142},   // w
    {'e', 'E', '[', 143},   // e
    {'r', 'R', ']', 144},   // r
    {'t', 'T', '/', 145},   // t
    {'y', 'Y', '\\', 146},  // y
    {'u', 'U', '|', 147},   // u
    {'i', 'I', '~', 148},   // i
    {'o', 'O', '\'', 149},  // o
    {'p', 'P', '"', 150},   // p
    {0, 0, 0, 0},           // no key
    {180, 180, 180, 152},   // LEFT
    {181, 181, 181, 153},   // UP
    {'a', 'A', ';', 154},   // a
    {'s', 'S', ':', 155},   // s
    {'d', 'D', '`', 156},   // d
    {'f', 'F', '+', 157},   // f
    {'g', 'G', '-', 158},   // g
    {'h', 'H', '_', 159},   // h
    {'j', 'J', '=', 160},   // j
    {'k', 'K', '?', 161},   // k
    {'l', 'L', 0, 162},     // l
    {13, 13, 13, 163},      // enter
    {182, 182, 182, 164},   // DOWN
    {183, 183, 183, 165},   // RIGHT
    {'z', 'Z', 0, 166},     // z
    {'x', 'X', 0, 167},     // x
    {'c', 'C', 0, 168},     // c
    {'v', 'V', 0, 169},     // v
    {'b', 'B', 0, 170},     // b
    {'n', 'N', 0, 171},     // n
    {'m', 'M', 0, 172},     // m
    {',', ',', '<', 173},   //,
    {'.', '.', '>', 174},   //.
    {' ', ' ', ' ', 175}    // space
};
static_assert(m5::stl::size(key_map) == UnitCardKB::NUMBER_OF_KEYS, "Invalid size");

// modifier bit to key_map mode index
constexpr uint8_t mod_table[] = {1, 0, 3, 2};  // 0x01:Shift, 0x80:Symbol 0x40:Fucntion

// ASCII to mode bit and key_index_t
// 1:normal 2:shift 4:symbol 8:fuction
constexpr std::pair<uint8_t, key_index_t> character_map[] = {
    {0x00, 0xFF},                        // NULL
    {0x00, 0xFF},                        // SOH
    {0x00, 0xFF},                        // STX
    {0x00, 0xFF},                        // ETX
    {0x00, 0xFF},                        // EOT
    {0x00, 0xFF},                        // ENG
    {0x00, 0xFF},                        // ACK
    {0x00, 0xFF},                        // BEL
    {1 + 4, UnitCardKB::KEY_BS},         // BS
    {1 + 2 + 4, UnitCardKB::KEY_TAB},    // HT
    {1 + 2 + 4, UnitCardKB::KEY_ENTER},  // LF
    {0x00, 0xFF},                        // VT
    {0x00, 0xFF},                        // FF
    {1 + 2 + 4, UnitCardKB::KEY_ENTER},  // CR
    {0x00, 0xFF},                        // SO
    {0x00, 0xFF},                        // SI
    {0x00, 0xFF},                        // DLE
    {0x00, 0xFF},                        // DC1
    {0x00, 0xFF},                        // DC2
    {0x00, 0xFF},                        // DC3
    {0x00, 0xFF},                        // DC4
    {0x00, 0xFF},                        // NAK
    {0x00, 0xFF},                        // SYN
    {0x00, 0xFF},                        // ETB
    {0x00, 0xFF},                        // CAN
    {0x00, 0xFF},                        // EM
    {0x00, 0xFF},                        // SUB
    {1 + 2 + 4, UnitCardKB::KEY_ESC},    // ESC
    {0x00, 0xFF},                        // FS
    {0x00, 0xFF},                        // GS
    {0x00, 0xFF},                        // RS
    {0x00, 0xFF},                        // US
    {1 + 2 + 4, UnitCardKB::KEY_SPACE},  // SP
    {4, UnitCardKB::KEY_1},              // !
    {4, UnitCardKB::KEY_P},              // "
    {4, UnitCardKB::KEY_3},              // #
    {4, UnitCardKB::KEY_4},              // $
    {4, UnitCardKB::KEY_5},              // %
    {4, UnitCardKB::KEY_7},              // &
    {4, UnitCardKB::KEY_O},              // ' (apostrophe)
    {4, UnitCardKB::KEY_9},              // (
    {4, UnitCardKB::KEY_0},              // )
    {4, UnitCardKB::KEY_8},              // *
    {4, UnitCardKB::KEY_F},              // +
    {1 + 2, UnitCardKB::KEY_COMMA},      // ,
    {4, UnitCardKB::KEY_G},              // -
    {1 + 2, UnitCardKB::KEY_PERIOD},     // .
    {4, UnitCardKB::KEY_T},              // /
    {1 + 2, UnitCardKB::KEY_0},          // 0
    {1 + 2, UnitCardKB::KEY_1},          // 1
    {1 + 2, UnitCardKB::KEY_2},          // 2
    {1 + 2, UnitCardKB::KEY_3},          // 3
    {1 + 2, UnitCardKB::KEY_4},          // 4
    {1 + 2, UnitCardKB::KEY_5},          // 5
    {1 + 2, UnitCardKB::KEY_6},          // 6
    {1 + 2, UnitCardKB::KEY_7},          // 7
    {1 + 2, UnitCardKB::KEY_8},          // 8
    {1 + 2, UnitCardKB::KEY_9},          // 9
    {4, UnitCardKB::KEY_S},              // :
    {4, UnitCardKB::KEY_A},              // ;
    {4, UnitCardKB::KEY_COMMA},          // <
    {4, UnitCardKB::KEY_J},              // =
    {4, UnitCardKB::KEY_PERIOD},         // >
    {4, UnitCardKB::KEY_K},              // ?
    {4, UnitCardKB::KEY_2},              // @
    {2, UnitCardKB::KEY_A},              // A
    {2, UnitCardKB::KEY_B},              // B
    {2, UnitCardKB::KEY_C},              // C
    {2, UnitCardKB::KEY_D},              // D
    {2, UnitCardKB::KEY_E},              // E
    {2, UnitCardKB::KEY_F},              // F
    {2, UnitCardKB::KEY_G},              // G
    {2, UnitCardKB::KEY_H},              // H
    {2, UnitCardKB::KEY_I},              // I
    {2, UnitCardKB::KEY_J},              // J
    {2, UnitCardKB::KEY_K},              // K
    {2, UnitCardKB::KEY_L},              // L
    {2, UnitCardKB::KEY_M},              // M
    {2, UnitCardKB::KEY_N},              // N
    {2, UnitCardKB::KEY_O},              // O
    {2, UnitCardKB::KEY_P},              // P
    {2, UnitCardKB::KEY_Q},              // Q
    {2, UnitCardKB::KEY_R},              // R
    {2, UnitCardKB::KEY_S},              // S
    {2, UnitCardKB::KEY_T},              // T
    {2, UnitCardKB::KEY_U},              // U
    {2, UnitCardKB::KEY_V},              // V
    {2, UnitCardKB::KEY_W},              // W
    {2, UnitCardKB::KEY_X},              // X
    {2, UnitCardKB::KEY_Y},              // Y
    {2, UnitCardKB::KEY_Z},              // Z
    {4, UnitCardKB::KEY_E},              // [
    {4, UnitCardKB::KEY_Y},              // '\'
    {4, UnitCardKB::KEY_R},              // ]
    {4, UnitCardKB::KEY_6},              // ^
    {4, UnitCardKB::KEY_H},              // _
    {4, UnitCardKB::KEY_D},              // ` (grave accent)
    {1, UnitCardKB::KEY_A},              // a
    {1, UnitCardKB::KEY_B},              // b
    {1, UnitCardKB::KEY_C},              // c
    {1, UnitCardKB::KEY_D},              // d
    {1, UnitCardKB::KEY_E},              // e
    {1, UnitCardKB::KEY_F},              // f
    {1, UnitCardKB::KEY_G},              // g
    {1, UnitCardKB::KEY_H},              // h
    {1, UnitCardKB::KEY_I},              // i
    {1, UnitCardKB::KEY_J},              // j
    {1, UnitCardKB::KEY_K},              // k
    {1, UnitCardKB::KEY_L},              // l
    {1, UnitCardKB::KEY_M},              // m
    {1, UnitCardKB::KEY_N},              // n
    {1, UnitCardKB::KEY_O},              // o
    {1, UnitCardKB::KEY_P},              // p
    {1, UnitCardKB::KEY_Q},              // q
    {1, UnitCardKB::KEY_R},              // r
    {1, UnitCardKB::KEY_S},              // s
    {1, UnitCardKB::KEY_T},              // t
    {1, UnitCardKB::KEY_U},              // u
    {1, UnitCardKB::KEY_V},              // v
    {1, UnitCardKB::KEY_W},              // w
    {1, UnitCardKB::KEY_X},              // x
    {1, UnitCardKB::KEY_Y},              // y
    {1, UnitCardKB::KEY_Z},              // z
    {4, UnitCardKB::KEY_Q},              // {
    {4, UnitCardKB::KEY_U},              // |
    {4, UnitCardKB::KEY_W},              // }
    {4, UnitCardKB::KEY_I},              // ~
    {2, UnitCardKB::KEY_BS},             // DEL
};
static_assert(m5::stl::size(character_map) == 128, "Invalid size");

constexpr std::pair<uint8_t, key_index_t> special_character_map[] = {
    {1 + 2 + 4, UnitCardKB::KEY_LEFT},   // Left cursor
    {1 + 2 + 4, UnitCardKB::KEY_UP},     // Up cursor
    {1 + 2 + 4, UnitCardKB::KEY_DOWN},   // Down cursor
    {1 + 2 + 4, UnitCardKB::KEY_RIGHT},  // Right cursor

};

}  // namespace

namespace m5 {
namespace unit {

// class UnitCardKB
const char UnitCardKB::name[] = "UnitCardKB";
const types::uid_t UnitCardKB::uid{"UnitCardKB"_mmh3};
const types::uid_t UnitCardKB::attr{0};

key_index_t UnitCardKB::character_to_key_index(const char ch)
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

uint8_t UnitCardKB::character_to_mode_bits(const char ch)
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

bool UnitCardKB::begin()
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

    uint8_t discard{};
    // Read and reject values to avoid false evaluations due to transmission of values for released keys
    readWithTransaction(&discard, 1);

    readFirmwareVersion(_firmware_version);
    readHardwareType(_type);
    M5_LIB_LOGI("Type:%02X Firmware:%02X", _type, _firmware_version);

    if (firmwareVersion()) {
        if (_type != TYPE_CARDKB && _type != TYPE_CARDKB_V11) {
            M5_LIB_LOGE("Invalid hardware type %02X", _type);
            return false;
        }
        if (!writeMode(_cfg.mode)) {
            M5_LIB_LOGE("Failed to write mode");
            return false;
        }
    }
    return UnitKeyboardBitwise::begin() && (_cfg.start_periodic ? startPeriodicMeasurement(_cfg.interval) : true);
}

void UnitCardKB::update(const bool force)
{
    if (!inPeriodic()) {
        return;
    }

    switch (_mode) {
        case Mode::M5UnitUnified: {
            auto at = m5::utility::millis();
            if (force || !_latest || at >= _latest + _interval) {
                _updated = update_new_firmware(at);
                if (_updated) {
                    _latest = at;
                }
            }
        } break;
        default:
            UnitKeyboardBitwise::update(force);
            break;
    }
}

bool UnitCardKB::update_new_firmware(const types::elapsed_time_t at)
{
    _wasHold = _wasPressed = _wasReleased = 0;
    _prev                                 = _now;
    auto prev_holding                     = _holding;

    uint8_t rbuf[(NUMBER_OF_KEYS + 7) / 8 + 1]{};
    if (!readRegister(CMD_SCAN_REG, rbuf, m5::stl::size(rbuf), 0)) {
        M5_LIB_LOGE("Failed to read");
        return false;
    }

    _now = (((uint64_t)rbuf[6]) << 56) | (((uint64_t)rbuf[5]) << 40) | (((uint64_t)rbuf[4]) << 32) |
           (((uint64_t)rbuf[3]) << 24) | (((uint64_t)rbuf[2]) << 16) | (((uint64_t)rbuf[1]) << 8) |
           (((uint64_t)rbuf[0]) << 0);

    uint8_t mod8 = rbuf[6];
    uint64_t bit{1};

    _wasPressed  = (_now ^ _prev) & _now;
    _wasReleased = (_now ^ _prev) & ~_now;

    for (uint_fast8_t i = 0; i < NUMBER_OF_KEYS; ++i, bit <<= 1) {
        // Was pressed
        if (_wasPressed & bit) {
            push_back(_data.get(), i, mod8);
            _repeat_start_at[i] = _hold_start_at[i] = at;
            _repeating |= bit;
            continue;
        }
        // Repeat?
        if ((_now & bit) && at - _repeat_start_at[i] >= _cfg.repeating_threshold) {
            _repeat_start_at[i] = at;
            _repeating |= bit;
            push_back(_data.get(), i, mod8);
        } else {
            _repeating &= ~bit;
        }
        // Hold?
        if ((_now & bit) && at - _hold_start_at[i] >= _cfg.holding_threshold) {
            _holding |= bit;
        } else {
            _holding &= ~bit;
        }
    }
    _wasHold = (prev_holding ^ _holding) & _holding;

    return true;  // Always true
}

void UnitCardKB::push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx, const uint8_t mod8)
{
    auto k = key_map[kidx][mod8 ? __builtin_ctz(mod8) + 1 : 0];
    if (k) {
        container->push_back(k);
    }
}

bool UnitCardKB::readHardwareType(uint8_t& htype)
{
    htype = 0;
    return readRegister8(CMD_HARDWARE_TYPE_REG, htype, 0);
}

}  // namespace unit
}  // namespace m5
