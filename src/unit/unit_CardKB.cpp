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

namespace {
constexpr uint8_t key_map[m5::unit::UnitCardKB::NUMBER_OF_KEYS][4 /*normal, shift, sym,fn */] = {
    {27, 27, 27, 128},      // esc 0
    {'1', '1', '!', 129},   // 1
    {'2', '2', '@', 130},   // 2
    {'3', '3', '#', 131},   // 3
    {'4', '4', '$', 132},   // 4
    {'5', '5', '%', 133},   // 5
    {'6', '6', '^', 134},   // 6
    {'7', '7', '&', 135},   // 7
    {'8', '8', '*', 136},   // 8
    {'9', '9', '(', 137},   // 9
    {'0', '0', ')', 138},   // 0 10
    {8, 127, 8, 139},       // bs/del
    {9, 9, 9, 140},         // tab
    {'q', 'Q', '{', 141},   // q
    {'w', 'W', '}', 142},   // w
    {'e', 'E', '[', 143},   // e
    {'r', 'R', ']', 144},   // r
    {'t', 'T', '/', 145},   // t
    {'y', 'Y', '\\', 146},  // y
    {'u', 'U', '|', 147},   // u
    {'i', 'I', '~', 148},   // i 20
    {'o', 'O', '\'', 149},  // o
    {'p', 'P', '"', 150},   // p
    {0, 0, 0, 0},           // no key
    {180, 180, 180, 152},   // LEFT
    {181, 181, 181, 153},   // UP
    {'a', 'A', ';', 154},   // a
    {'s', 'S', ':', 155},   // s
    {'d', 'D', '`', 156},   // d
    {'f', 'F', '+', 157},   // f
    {'g', 'G', '-', 158},   // g  30
    {'h', 'H', '_', 159},   // h
    {'j', 'J', '=', 160},   // j
    {'k', 'K', '?', 161},   // k
    {'l', 'L', 0, 162},     // l
    {13, 13, 13, 163},      // enter
    {182, 182, 182, 164},   // DOWN
    {183, 183, 183, 165},   // RIGHT
    {'z', 'Z', 0, 166},     // z
    {'x', 'X', 0, 167},     // x
    {'c', 'C', 0, 168},     // c 40
    {'v', 'V', 0, 169},     // v
    {'b', 'B', 0, 170},     // b
    {'n', 'N', 0, 171},     // n
    {'m', 'M', 0, 172},     // m
    {',', ',', '<', 173},   //,
    {'.', '.', '>', 174},   //.
    {' ', ' ', ' ', 175}    // space
};

// modifier bit to key_map category index
constexpr uint8_t mod_table[] = {1, 0, 3, 2};  // 0x01:Shift, 0x80:Symbol 0x40:Fucntion

constexpr uint16_t character_map[] = {
    //
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    // BS      TAB     \n                  \r
    0x000B, 0x000C, 0x0023, 0xFFFF, 0xFFFF, 0x0023, 0xFFFF, 0xFFFF,
    //
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    //                         ESC
    0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    // SPC      !       "       #       $       %       &       '
    0x002F, 0x8001, 0x8016, 0x8003, 0x8004, 0x8005, 0x8007, 0x8015,
    // (       )       *       +       ,       -       .       /
    0x8009, 0x800A, 0x8008, 0x801D, 0x002D, 0x801E, 0x002E, 0x8011,
    // 0       1       2       3       4       5       6       7
    0x000A, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    // 8       9       :       ;       <       =       >       ?
    0x0008, 0x0009, 0x801B, 0x801A, 0x802D, 0x8020, 0x802E, 0x8021,
    // @       A       B       C       D       E       F       G
    0x8002, 0x101A, 0x102A, 0x1028, 0x101C, 0x100F, 0x101D, 0x101E,
    // H       I       J       K       L       M       N       O
    0x101F, 0x1014, 0x1020, 0x1021, 0x1022, 0x102C, 0x102B, 0x1015,
    // P       Q       R       S       T       U       V       W
    0x1016, 0x100D, 0x1010, 0x101B, 0x1011, 0x1013, 0x1029, 0x100E,
    // X       Y       Z      [        \       ]       ^       _
    0x1027, 0x1012, 0x1026, 0x800F, 0x8012, 0x8010, 0x8006, 0x801F,
    // `       a       b       c       d       e       f       g
    0x801C, 0x001A, 0x002A, 0x0028, 0x001C, 0x000F, 0x001D, 0x001E,
    // h       i       j       k       l       m       n       o
    0x001F, 0x0014, 0x0020, 0x0021, 0x0022, 0x002C, 0x002B, 0x0015,
    // p       q       r       s       t       u       v       w
    0x0016, 0x000D, 0x0010, 0x001B, 0x0011, 0x0013, 0x0029, 0x000E,
    // x       y       z       (       |       )       ~       DEL
    0x0027, 0x0012, 0x0026, 0x800D, 0x8013, 0x800E, 0x8014, 0x100B};

}  // namespace

namespace m5 {
namespace unit {

// class UnitCardKB
const char UnitCardKB::name[] = "UnitCardKB";
const types::uid_t UnitCardKB::uid{"UnitCardKB"_mmh3};
const types::uid_t UnitCardKB::attr{0};

UnitKeyboardBitwise::key_index_t UnitCardKB::character_to_key_index(const char ch)
{
    unsigned char uc = ch;
    // function? (>= 0x80)
    if (uc & 0x80) {
        key_index_t kidx = (key_index_t)(uc - 0x80);
        return static_cast<key_index_t>((kidx < m5::stl::size(key_map)) ? kidx : 0xFF);
    }
    // normal, shift or symbol
    return static_cast<key_index_t>((uc < m5::stl::size(character_map)) ? (character_map[uc] & 0xFF) : 0xFF);
}

uint8_t UnitCardKB::character_to_modifier_bit(const char ch)
{
    unsigned char uc = ch;
    // function? (>= 0x80)
    if (uc & 0x80) {
        key_index_t kidx = (key_index_t)(uc - 0x80);
        return (kidx < m5::stl::size(key_map)) ? MODIFIER_FUNCTION_8BIT : 0x00;
    }
    // normal, shift or symbol
    return (uc < m5::stl::size(character_map)) ? (character_map[uc] >> 8) : 0x00;
}

bool UnitCardKB::begin()
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
    return UnitKeyboardBitwise::begin();
}

void UnitCardKB::update(const bool force)
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
            break;
    }
}

bool UnitCardKB::update_new_firmware(const types::elapsed_time_t at)
{
    _wasHold = _wasPressed = _wasReleased = 0;
    _prev                                 = _now;
    auto prev_holding                     = _holding;

    uint8_t rbuf[NUMBER_OF_KEYS / 8 + 1]{};
    if (!readRegister(CMD_SCAN_REG, rbuf, m5::stl::size(rbuf), 0)) {
        M5_LIB_LOGE("Failed to read");
        return false;
    }
    // M5_LIB_LOGI("KB:%02X;%02X;%02X;%02X;%02X;%02X;%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5],
    // rbuf[6]);

    _now = (((uint64_t)rbuf[6]) << 48) | (((uint64_t)rbuf[5]) << 40) | (((uint64_t)rbuf[4]) << 32) |
           (((uint64_t)rbuf[3]) << 24) | (((uint64_t)rbuf[2]) << 16) | (((uint64_t)rbuf[1]) << 8) |
           (((uint64_t)rbuf[0]) << 0);

    uint8_t mod = modifier_bits();
    uint64_t bit{1};

    _wasPressed  = (_now ^ _prev) & _now;
    _wasReleased = (_now ^ _prev) & ~_now;

    for (uint_fast8_t i = 0; i < NUMBER_OF_KEYS; ++i, bit <<= 1) {
        // Was pressed
        if (_wasPressed & bit) {
            push_back(_inputs.get(), i, mod);
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
            push_back(_inputs.get(), i, mod);
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

void UnitCardKB::push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx, const uint8_t mod)
{
    uint8_t midx{};
    uint8_t single_mod = (mod & MODIFIER_SHIFT_8BIT)      ? MODIFIER_SHIFT_8BIT
                         : (mod & MODIFIER_SYMBOL_8BIT)   ? MODIFIER_SYMBOL_8BIT
                         : (mod & MODIFIER_FUNCTION_8BIT) ? MODIFIER_FUNCTION_8BIT
                                                          : 0;
    if (single_mod) {
        midx = mod_table[(__builtin_ctz(single_mod)) & 0x03];
    }
    auto k = key_map[kidx][midx];
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
