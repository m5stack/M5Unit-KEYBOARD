/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_FacesQWERTY.cpp
  @brief Faces QWERTY Unit for M5UnitUnified
*/
#include "unit_FacesQWERTY.hpp"
#if defined(ARDUINO)
#include <Arduino.h> // For digitalPinToInterrupt
#endif

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::keyboard;
using namespace m5::unit::keyboard::command;
using namespace m5::unit::faces;
using namespace m5::unit::faces::command;
using m5::unit::UnitFacesQWERTY;

namespace {
constexpr uint8_t key_map[][5 /* mode: normal, shift, sym, fn, alt */] = {
    {'q', 'Q', '#', '~', 144},   // q
    {'w', 'W', '1', '^', 145},   // w
    {'e', 'E', '2', '&', 146},   // e
    {'r', 'R', '3', '`', 147},   // r
    {'t', 'T', '(', '<', 148},   // t
    {'y', 'Y', ')', '>', 149},   // y
    {'u', 'U', '_', '{', 150},   // u
    {'i', 'I', '-', '}', 151},   // i
    {'o', 'O', '+', '[', 152},   // o
    {'p', 'P', '@', ']', 153},   // p
    {'a', 'A', '*', '|', 154},   // a
    {'s', 'S', '4', '=', 155},   // s
    {'d', 'D', '5', '\\', 156},  // d
    {'f', 'F', '6', '%', 157},   // f
    {'g', 'G', '/', 180, 158},   // g
    {'h', 'H', ':', 181, 159},   // h
    {'j', 'J', ';', 182, 160},   // j
    {'k', 'K', '\'', 183, 161},  // k
    {'l', 'L', '"', 184, 162},   // l
    {8, 127, 8, 8, 163},         // bs/del Old: {8 , 8, 127, 8, 163} Fixes same as CARDKB (Shift+BS => DEL)
    {},                          // no key (alt)
    {'z', 'Z', '7', 9, 165},     // z  Old: {'z', 'Z', '7', 186, 165}, Fixes Fn+Z => TAB
    {'x', 'X', '8', 187, 166},   // x
    {'c', 'C', '9', 188, 167},   // c
    {'v', 'V', '?', 189, 168},   // v
    {'b', 'B', '!', 190, 169},   // b
    {'n', 'N', ',', 191, 170},   // n
    {'m', 'M', '.', 192, 171},   // m
    {'$', '$', 194, 193, 172},   // $ Old: {'$', '$', 255, 193, 172}, Fixes speaker mark 255(invalid) to 194
    {13, 13, 13, 13, 173},       // enter
    {},                          // // no key (shift)
    {'0', '0', 27, '0', 175},    // 0 Old: {'0', '0', '0', '0', 175},Fixes Sym+0 => ESC
    {' ', ' ', ' ', ' ', 176},   // space
    {},                          // no key (sym)
    {}                           // no key (fn)
};
static_assert(m5::stl::size(key_map) == UnitFacesQWERTY::NUMBER_OF_KEYS, "Invalid size");

// ASCII to mode bit and key_index_t
// 1:normal 2:shift 4:symbol 8:fuction 16:alt
constexpr std::pair<uint8_t, key_index_t> character_map[] = {
    {0x00, 0xFF},                                 // NULL
    {0x00, 0xFF},                                 // SOH
    {0x00, 0xFF},                                 // STX
    {0x00, 0xFF},                                 // ETX
    {0x00, 0xFF},                                 // EOT
    {0x00, 0xFF},                                 // ENG
    {0x00, 0xFF},                                 // ACK
    {0x00, 0xFF},                                 // BEL
    {1 + 4 + 8, UnitFacesQWERTY::KEY_BS},         // BS
    {8, UnitFacesQWERTY::KEY_Z},                  // HT
    {1 + 2 + 4 + 8, UnitFacesQWERTY::KEY_ENTER},  // LF
    {0x00, 0xFF},                                 // VT
    {0x00, 0xFF},                                 // FF
    {1 + 2 + 4 + 8, UnitFacesQWERTY::KEY_ENTER},  // CR
    {0x00, 0xFF},                                 // SO
    {0x00, 0xFF},                                 // SI
    {0x00, 0xFF},                                 // DLE
    {0x00, 0xFF},                                 // DC1
    {0x00, 0xFF},                                 // DC2
    {0x00, 0xFF},                                 // DC3
    {0x00, 0xFF},                                 // DC4
    {0x00, 0xFF},                                 // NAK
    {0x00, 0xFF},                                 // SYN
    {0x00, 0xFF},                                 // ETB
    {0x00, 0xFF},                                 // CAN
    {0x00, 0xFF},                                 // EM
    {0x00, 0xFF},                                 // SUB
    {4, UnitFacesQWERTY::KEY_0},                  // ESC
    {0x00, 0xFF},                                 // FS
    {0x00, 0xFF},                                 // GS
    {0x00, 0xFF},                                 // RS
    {0x00, 0xFF},                                 // US
    {1 + 2 + 4 + 8, UnitFacesQWERTY::KEY_SPACE},  // SP
    {4, UnitFacesQWERTY::KEY_B},                  // !
    {4, UnitFacesQWERTY::KEY_L},                  // "
    {4, UnitFacesQWERTY::KEY_Q},                  // #
    {1 + 2, UnitFacesQWERTY::KEY_DOLLAR},         // $
    {8, UnitFacesQWERTY::KEY_F},                  // %
    {8, UnitFacesQWERTY::KEY_E},                  // &
    {4, UnitFacesQWERTY::KEY_K},                  // ' (apostrophe)
    {4, UnitFacesQWERTY::KEY_T},                  // (
    {4, UnitFacesQWERTY::KEY_Y},                  // )
    {4, UnitFacesQWERTY::KEY_A},                  // *
    {4, UnitFacesQWERTY::KEY_O},                  // +
    {4, UnitFacesQWERTY::KEY_N},                  // ,
    {4, UnitFacesQWERTY::KEY_I},                  // -
    {4, UnitFacesQWERTY::KEY_M},                  // .
    {4, UnitFacesQWERTY::KEY_G},                  // /
    {1 + 2 + 8, UnitFacesQWERTY::KEY_0},          // 0
    {4, UnitFacesQWERTY::KEY_W},                  // 1
    {4, UnitFacesQWERTY::KEY_E},                  // 2
    {4, UnitFacesQWERTY::KEY_R},                  // 3
    {4, UnitFacesQWERTY::KEY_S},                  // 4
    {4, UnitFacesQWERTY::KEY_D},                  // 5
    {4, UnitFacesQWERTY::KEY_F},                  // 6
    {4, UnitFacesQWERTY::KEY_Z},                  // 7
    {4, UnitFacesQWERTY::KEY_X},                  // 8
    {4, UnitFacesQWERTY::KEY_C},                  // 9
    {4, UnitFacesQWERTY::KEY_H},                  // :
    {4, UnitFacesQWERTY::KEY_J},                  // ;
    {8, UnitFacesQWERTY::KEY_T},                  // <
    {8, UnitFacesQWERTY::KEY_S},                  // =
    {8, UnitFacesQWERTY::KEY_Y},                  // >
    {4, UnitFacesQWERTY::KEY_V},                  // ?
    {4, UnitFacesQWERTY::KEY_P},                  // @
    {2, UnitFacesQWERTY::KEY_A},                  // A
    {2, UnitFacesQWERTY::KEY_B},                  // B
    {2, UnitFacesQWERTY::KEY_C},                  // C
    {2, UnitFacesQWERTY::KEY_D},                  // D
    {2, UnitFacesQWERTY::KEY_E},                  // E
    {2, UnitFacesQWERTY::KEY_F},                  // F
    {2, UnitFacesQWERTY::KEY_G},                  // G
    {2, UnitFacesQWERTY::KEY_H},                  // H
    {2, UnitFacesQWERTY::KEY_I},                  // I
    {2, UnitFacesQWERTY::KEY_J},                  // J
    {2, UnitFacesQWERTY::KEY_K},                  // K
    {2, UnitFacesQWERTY::KEY_L},                  // L
    {2, UnitFacesQWERTY::KEY_M},                  // M
    {2, UnitFacesQWERTY::KEY_N},                  // N
    {2, UnitFacesQWERTY::KEY_O},                  // O
    {2, UnitFacesQWERTY::KEY_P},                  // P
    {2, UnitFacesQWERTY::KEY_Q},                  // Q
    {2, UnitFacesQWERTY::KEY_R},                  // R
    {2, UnitFacesQWERTY::KEY_S},                  // S
    {2, UnitFacesQWERTY::KEY_T},                  // T
    {2, UnitFacesQWERTY::KEY_U},                  // U
    {2, UnitFacesQWERTY::KEY_V},                  // V
    {2, UnitFacesQWERTY::KEY_W},                  // W
    {2, UnitFacesQWERTY::KEY_X},                  // X
    {2, UnitFacesQWERTY::KEY_Y},                  // Y
    {2, UnitFacesQWERTY::KEY_Z},                  // Z
    {8, UnitFacesQWERTY::KEY_O},                  // [
    {8, UnitFacesQWERTY::KEY_D},                  // '\'
    {8, UnitFacesQWERTY::KEY_P},                  // ]
    {8, UnitFacesQWERTY::KEY_W},                  // ^
    {4, UnitFacesQWERTY::KEY_U},                  // _
    {8, UnitFacesQWERTY::KEY_R},                  // ` (grave accent)
    {1, UnitFacesQWERTY::KEY_A},                  // a
    {1, UnitFacesQWERTY::KEY_B},                  // b
    {1, UnitFacesQWERTY::KEY_C},                  // c
    {1, UnitFacesQWERTY::KEY_D},                  // d
    {1, UnitFacesQWERTY::KEY_E},                  // e
    {1, UnitFacesQWERTY::KEY_F},                  // f
    {1, UnitFacesQWERTY::KEY_G},                  // g
    {1, UnitFacesQWERTY::KEY_H},                  // h
    {1, UnitFacesQWERTY::KEY_I},                  // i
    {1, UnitFacesQWERTY::KEY_J},                  // j
    {1, UnitFacesQWERTY::KEY_K},                  // k
    {1, UnitFacesQWERTY::KEY_L},                  // l
    {1, UnitFacesQWERTY::KEY_M},                  // m
    {1, UnitFacesQWERTY::KEY_N},                  // n
    {1, UnitFacesQWERTY::KEY_O},                  // o
    {1, UnitFacesQWERTY::KEY_P},                  // p
    {1, UnitFacesQWERTY::KEY_Q},                  // q
    {1, UnitFacesQWERTY::KEY_R},                  // r
    {1, UnitFacesQWERTY::KEY_S},                  // s
    {1, UnitFacesQWERTY::KEY_T},                  // t
    {1, UnitFacesQWERTY::KEY_U},                  // u
    {1, UnitFacesQWERTY::KEY_V},                  // v
    {1, UnitFacesQWERTY::KEY_W},                  // w
    {1, UnitFacesQWERTY::KEY_X},                  // x
    {1, UnitFacesQWERTY::KEY_Y},                  // y
    {1, UnitFacesQWERTY::KEY_Z},                  // z
    {8, UnitFacesQWERTY::KEY_U},                  // {
    {8, UnitFacesQWERTY::KEY_A},                  // |
    {8, UnitFacesQWERTY::KEY_I},                  // }
    {8, UnitFacesQWERTY::KEY_Q},                  // ~
    {2, UnitFacesQWERTY::KEY_BS},                 // DEL
};
static_assert(m5::stl::size(character_map) == 128, "Invalid size");

constexpr std::pair<uint8_t, key_index_t> special_character_map[] = {
    {8, UnitFacesQWERTY::KEY_G},
    {8, UnitFacesQWERTY::KEY_H},
    {8, UnitFacesQWERTY::KEY_J},
    {8, UnitFacesQWERTY::KEY_K},  // Up cursor
    {8, UnitFacesQWERTY::KEY_L},  // INS
    {0, 0xFF},
    {0, 0xFF},
    {8, UnitFacesQWERTY::KEY_X},       // HOME
    {8, UnitFacesQWERTY::KEY_C},       // END
    {8, UnitFacesQWERTY::KEY_V},       // PU
    {8, UnitFacesQWERTY::KEY_B},       // PD
    {8, UnitFacesQWERTY::KEY_N},       // Left cursor
    {8, UnitFacesQWERTY::KEY_M},       // Down cursor
    {8, UnitFacesQWERTY::KEY_DOLLAR},  // Right cursor
    {4, UnitFacesQWERTY::KEY_DOLLAR},  // Speaker mark
};

constexpr uint8_t INTERRUPT_PIN{5};
bool input_irq{};
void IRAM_ATTR handle_faces_qwerty()
{
    input_irq = true;
}

}  // namespace

namespace m5 {
namespace unit {

// class UnitFacesQWERTY
const char UnitFacesQWERTY::name[] = "UnitFacesQWERTY";
const types::uid_t UnitFacesQWERTY::uid{"UnitFacesQWERTY"_mmh3};
const types::uid_t UnitFacesQWERTY::attr{0};

key_index_t UnitFacesQWERTY::character_to_key_index(const char ch)
{
    unsigned char uc = ch;
    // Special key?
    if (uc >= SCHAR_NOMARK_G && uc <= SCHAR_SPEAKER) {
        return special_character_map[uc - SCHAR_NOMARK_G].second;
    }
    // alt? (>= 0x90)
    if (uc >= 0x90) {
        key_index_t kidx = (key_index_t)(uc - 0x90);
        return static_cast<key_index_t>((kidx < m5::stl::size(key_map)) ? kidx : 0xFF);
    }
    // normal,shift,symbol and function
    return static_cast<key_index_t>((uc < m5::stl::size(character_map)) ? (character_map[uc].second) : 0xFF);
}

uint8_t UnitFacesQWERTY::character_to_mode_bits(const char ch)
{
    unsigned char uc = ch;
    // Special key?
    if (uc >= SCHAR_NOMARK_G && uc <= SCHAR_SPEAKER) {
        //        M5_LIB_LOGI("%c => %02X", ch, special_character_map[uc - SCHAR_NOMARK_G].first);
        return special_character_map[uc - SCHAR_NOMARK_G].first;
    }
    // alt? (>= 0x90)
    if (uc >= 0x90) {
        key_index_t kidx = (key_index_t)(uc - 0x90);
        //        M5_LIB_LOGI("%c => %02X", ch, (uc - 0x90));
        return (kidx < m5::stl::size(key_map)) ? 0x10 : 0x00;
    }
    // normal,shift,symbol and function
    //    M5_LIB_LOGI("%c => %02X", ch, (uc < m5::stl::size(character_map)) ? (character_map[uc].first) : 0x00);
    return (uc < m5::stl::size(character_map)) ? (character_map[uc].first) : 0x00;
}

bool UnitFacesQWERTY::begin()
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

    // Try read firmware version and hardware type
    readFirmwareVersion(_firmware_version);
    readFacesType(_type);
    M5_LIB_LOGI("Type:%02X Firmware:%02X", _type, _firmware_version);

    if (firmwareVersion()) {
        if (_type != TYPE_QWERTY) {
            M5_LIB_LOGE("Invalid faces type %02X", _type);
            return false;
        }
        if (!writeMode(_cfg.mode)) {
            M5_LIB_LOGE("Failed to write mode");
            return false;
        }
    }

    _handle_irq = _cfg.trigger_irq;

#if defined(ARDUINO)
    if (_handle_irq) {
        adapter()->pinMode(INTERRUPT_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handle_faces_qwerty, FALLING);
        _cfg.interval = std::numeric_limits<decltype(_cfg.interval)>::max();
    }
#else
    // TODO: ESP-IDF with M5HAL
#pragma message "trigger_irq is not supported"
#endif
    return UnitKeyboardBitwise::begin() && _cfg.start_periodic ? startPeriodicMeasurement(_cfg.interval) : true;
}

void UnitFacesQWERTY::update(const bool force)
{
    if (!inPeriodic()) {
        return;
    }

    switch (_mode) {
        case Mode::M5UnitUnified: {
            auto at = m5::utility::millis();
            if (force || input_irq || (!_handle_irq && (!_latest || at >= _latest + _interval))) {
                M5_LIB_LOGE("\t --- update %d/%d/%d", force, input_irq,
                            (!_handle_irq && (!_latest || at >= _latest + _interval)));

                _updated = update_new_firmware(at);
                if (_updated) {
                    _latest = at;
                }
                input_irq = false;
            }
        } break;
        default:
            if (_handle_irq) {
                if (input_irq) {
                    M5_LIB_LOGE("IRQ");
                    UnitKeyboardBitwise::update(true);
                }
            } else {
                UnitKeyboardBitwise::update();
            }
            //  Enter key is returned by 2 bytes of [0x0D, 0X0A] from old firmware or Conventional behavior
            if (released() == 0x0D) {
                uint8_t discard{};
                readWithTransaction(&discard, 1);  // Discard 0x0A
            }
            input_irq = false;
            break;
    }
}

bool UnitFacesQWERTY::update_new_firmware(const types::elapsed_time_t at)
{
    _wasHold = _wasPressed = _wasReleased = 0;
    _prev                                 = _now;
    auto prev_holding                     = _holding;

    uint8_t rbuf[(NUMBER_OF_KEYS + 7) / 8 + 1]{};
    if (!readRegister(CMD_SCAN_REG, rbuf, m5::stl::size(rbuf), 0)) {
        M5_LIB_LOGE("Failed to read");
        return false;
    }

    _now = (((uint64_t)rbuf[5]) << 56) | (((uint64_t)rbuf[4]) << 32) | (((uint64_t)rbuf[3]) << 24) |
           (((uint64_t)rbuf[2]) << 16) | (((uint64_t)rbuf[1]) << 8) | (((uint64_t)rbuf[0]) << 0);

    uint8_t mod8 = rbuf[5];
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
#if 0
        // Was released
        if (_wasReleased & bit) {
            push_back(_released.get(), i, mod);
        }
#endif
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

void UnitFacesQWERTY::push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx,
                                const uint8_t mod8)
{
    auto k = key_map[kidx][mod8 ? __builtin_ctz(mod8) + 1 : 0];
    if (k) {
        container->push_back(k);
    }
}

bool UnitFacesQWERTY::readFacesType(uint8_t& ftype)
{
    ftype = 0;
    return readRegister8(CMD_FACES_TYPE_REG, ftype, 0);
}

}  // namespace unit
}  // namespace m5
