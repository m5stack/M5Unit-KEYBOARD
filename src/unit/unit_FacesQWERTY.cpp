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

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::keyboard;
using namespace m5::unit::keyboard::command;
using namespace m5::unit::faces;
using namespace m5::unit::faces::command;

namespace m5 {
namespace unit {

// class UnitFacesQWERTY
const char UnitFacesQWERTY::name[] = "UnitFacesQWERTY";
const types::uid_t UnitFacesQWERTY::uid{"UnitFacesQWERTY"_mmh3};
const types::uid_t UnitFacesQWERTY::attr{0};

UnitKeyboardBitwise::key_index_t UnitFacesQWERTY::character_to_key_index(const char ch)
{
#if 0
    unsigned char uc = ch;
    // function? (>= 0x80)
    if (uc & 0x80) {
        key_index_t kidx = (key_index_t)(uc - 0x80);
        return static_cast<key_index_t>((kidx < m5::stl::size(key_map)) ? kidx : 0xFF);
    }
    // normal, shift or symbol
    return static_cast<key_index_t>((uc < m5::stl::size(character_map)) ? (character_map[uc] & 0xFF) : 0xFF);
#endif
    return 0x00;
}

uint8_t UnitFacesQWERTY::character_to_modifier_bit(const char ch)
{
#if 0
    unsigned char uc = ch;
    // function? (>= 0x80)
    if (uc & 0x80) {
        key_index_t kidx = (key_index_t)(uc - 0x80);
        return (kidx < m5::stl::size(key_map)) ? MODIFIER_FUNCTION_8BIT : 0x00;
    }
    // normal, shift or symbol
    return (uc < m5::stl::size(character_map)) ? (character_map[uc] >> 8) : 0x00;
#endif
    return 0x00;
}

bool UnitFacesQWERTY::begin()
{
    auto ssize = _cfg.stored_keys;
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _inputs->capacity()) {
        _inputs.reset(new m5::container::CircularBuffer<uint8_t>(ssize));
        if (!_inputs) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    _interval = _cfg.interval;
    _periodic = _cfg.start_periodic;

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
    return UnitKeyboardBitwise::begin();
}

void UnitFacesQWERTY::update(const bool force)
{
    if (!inPeriodic()) {
        return;
    }

    switch (_mode) {
        case Mode::Scan: {
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
            //  Enter key is returned by 2 bytes of [0x0D, 0X0A] from Firmware
            if (released() == 0x0D) {
                uint8_t discard{};
                readWithTransaction(&discard, 1);  // Discard 0x0A
            }
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
    // M5_LIB_LOGI("KB:%02X;%02X;%02X;%02X;%02X;%02X;%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5],
    // rbuf[6]);

    _now = (((uint64_t)rbuf[5]) << 40) | (((uint64_t)rbuf[4]) << 32) | (((uint64_t)rbuf[3]) << 24) |
           (((uint64_t)rbuf[2]) << 16) | (((uint64_t)rbuf[1]) << 8) | (((uint64_t)rbuf[0]) << 0);

    uint8_t mod = modifier_bits();
    uint64_t bit{1};

    _wasPressed  = (_now ^ _prev) & _now;
    _wasReleased = (_now ^ _prev) & ~_now;

    for (uint_fast8_t i = 0; i < NUMBER_OF_KEYS; ++i, bit <<= 1) {
        // Was pressed
        if (_wasPressed & bit) {
            //            push_back(_inputs.get(), i, mod);
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
            //            push_back(_inputs.get(), i, mod);
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
                                const uint8_t mod)
{
#if 0
    uint8_t midx{};
    uint8_t single_mod = (mod & MODIFIER_SHIFT_8BIT)      ? MODIFIER_SHIFT_8BIT
                         : (mod & MODIFIER_SYMBOL_8BIT)   ? MODIFIER_SYMBOL_8BIT
                         : (mod & MODIFIER_FUNCTION_8BIT) ? MODIFIER_FUNCTION_8BIT
                         : (mod & MODIFIER_ALT_8BIT)      ? MODIFIER_ALT_8BIT
                                                          : 0;
    if (single_mod) {
        midx = mod_table[(__builtin_ctz(single_mod)) & 0x03];
    }
    auto k = key_map[kidx][midx];
    if (k) {
        container->push_back(k);
    }
#endif
}

bool UnitFacesQWERTY::readFacesType(uint8_t& ftype)
{
    ftype = 0;
    return readRegister8(CMD_FACES_TYPE_REG, ftype, 0);
}

}  // namespace unit
}  // namespace m5
