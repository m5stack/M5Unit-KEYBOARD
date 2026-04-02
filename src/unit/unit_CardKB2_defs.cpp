/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB2_defs.cpp
  @brief Shared key maps and lookup functions for CardKB2
*/
#include "unit_CardKB2_defs.hpp"
#include <M5Utility.hpp>

using namespace m5::unit::keyboard;

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
static_assert(m5::stl::size(key_map) == m5::unit::cardkb2::NUMBER_OF_KEYS, "Invalid size");

// ASCII to mode bit and key_index_t
// 1:normal 2:shift 4:symbol 8:function
// 0x0: no key, 0xFF: invalid char
constexpr std::pair<uint8_t, key_index_t> character_map[] = {
    {0x00, 0xFF},                               // NULL (0)
    {0x00, 0xFF},                               // SOH (1)
    {0x00, 0xFF},                               // STX (2)
    {0x00, 0xFF},                               // ETX (3)
    {0x00, 0xFF},                               // EOT (4)
    {0x00, 0xFF},                               // ENG (5)
    {0x00, 0xFF},                               // ACK (6)
    {0x00, 0xFF},                               // BEL (7)
    {1 + 4, m5::unit::cardkb2::KEY_DELETE},     // BS (8)
    {0x00, 0xFF},                               // HT (9)
    {1 + 2 + 4, m5::unit::cardkb2::KEY_ENTER},  // LF (10)
    {0x00, 0xFF},                               // VT (11)
    {0x00, 0xFF},                               // FF (12)
    {1 + 2 + 4, m5::unit::cardkb2::KEY_ENTER},  // CR (13)
    {0x00, 0xFF},                               // SO (14)
    {0x00, 0xFF},                               // SI (15)
    {0x00, 0xFF},                               // DLE (16)
    {0x00, 0xFF},                               // DC1 (17)
    {0x00, 0xFF},                               // DC2 (18)
    {0x00, 0xFF},                               // DC3 (19)
    {0x00, 0xFF},                               // DC4 (20)
    {0x00, 0xFF},                               // NAK (21)
    {0x00, 0xFF},                               // SYN (22)
    {0x00, 0xFF},                               // ETB (23)
    {0x00, 0xFF},                               // CAN (24)
    {0x00, 0xFF},                               // EM (25)
    {0x00, 0xFF},                               // SUB (26)
    {0x08, m5::unit::cardkb2::KEY_1},           // ESC (27) — Fn+1
    {0x00, 0xFF},                               // FS (28)
    {0x00, 0xFF},                               // GS (29)
    {0x00, 0xFF},                               // RS (30)
    {0x00, 0xFF},                               // US (31)
    {1 + 2 + 4, m5::unit::cardkb2::KEY_SPACE},  // SP (32)
    {4, m5::unit::cardkb2::KEY_1},              // ! (33)
    {4, m5::unit::cardkb2::KEY_H},              // " (34)
    {4, m5::unit::cardkb2::KEY_3},              // # (35)
    {4, m5::unit::cardkb2::KEY_4},              // $ (36)
    {4, m5::unit::cardkb2::KEY_5},              // % (37)
    {4, m5::unit::cardkb2::KEY_7},              // & (38)
    {4, m5::unit::cardkb2::KEY_J},              // ' (39)
    {4, m5::unit::cardkb2::KEY_9},              // ( (40)
    {4, m5::unit::cardkb2::KEY_0},              // ) (41)
    {4, m5::unit::cardkb2::KEY_8},              // * (42)
    {4, m5::unit::cardkb2::KEY_O},              // + (43)
    {4, m5::unit::cardkb2::KEY_N},              // , (44)
    {4, m5::unit::cardkb2::KEY_I},              // - (45)
    {4, m5::unit::cardkb2::KEY_M},              // . (46)
    {4, m5::unit::cardkb2::KEY_T},              // / (47)
    {1 + 2, m5::unit::cardkb2::KEY_0},          // 0 (48)
    {1 + 2, m5::unit::cardkb2::KEY_1},          // 1 (49)
    {1 + 2, m5::unit::cardkb2::KEY_2},          // 2 (50)
    {1 + 2, m5::unit::cardkb2::KEY_3},          // 3 (51)
    {1 + 2, m5::unit::cardkb2::KEY_4},          // 4 (52)
    {1 + 2, m5::unit::cardkb2::KEY_5},          // 5 (53)
    {1 + 2, m5::unit::cardkb2::KEY_6},          // 6 (54)
    {1 + 2, m5::unit::cardkb2::KEY_7},          // 7 (55)
    {1 + 2, m5::unit::cardkb2::KEY_8},          // 8 (56)
    {1 + 2, m5::unit::cardkb2::KEY_9},          // 9 (57)
    {4, m5::unit::cardkb2::KEY_L},              // : (58)
    {4, m5::unit::cardkb2::KEY_K},              // ; (59)
    {4, m5::unit::cardkb2::KEY_V},              // < (60)
    {4, m5::unit::cardkb2::KEY_P},              // = (61)
    {4, m5::unit::cardkb2::KEY_B},              // > (62)
    {4, m5::unit::cardkb2::KEY_E},              // ? (63)
    {4, m5::unit::cardkb2::KEY_2},              // @ (64)
    {2, m5::unit::cardkb2::KEY_A},              // A (65)
    {2, m5::unit::cardkb2::KEY_B},              // B (66)
    {2, m5::unit::cardkb2::KEY_C},              // C (67)
    {2, m5::unit::cardkb2::KEY_D},              // D (68)
    {2, m5::unit::cardkb2::KEY_E},              // E (69)
    {2, m5::unit::cardkb2::KEY_F},              // F (70)
    {2, m5::unit::cardkb2::KEY_G},              // G (71)
    {2, m5::unit::cardkb2::KEY_H},              // H (72)
    {2, m5::unit::cardkb2::KEY_I},              // I (73)
    {2, m5::unit::cardkb2::KEY_J},              // J (74)
    {2, m5::unit::cardkb2::KEY_K},              // K (75)
    {2, m5::unit::cardkb2::KEY_L},              // L (76)
    {2, m5::unit::cardkb2::KEY_M},              // M (77)
    {2, m5::unit::cardkb2::KEY_N},              // N (78)
    {2, m5::unit::cardkb2::KEY_O},              // O (79)
    {2, m5::unit::cardkb2::KEY_P},              // P (80)
    {2, m5::unit::cardkb2::KEY_Q},              // Q (81)
    {2, m5::unit::cardkb2::KEY_R},              // R (82)
    {2, m5::unit::cardkb2::KEY_S},              // S (83)
    {2, m5::unit::cardkb2::KEY_T},              // T (84)
    {2, m5::unit::cardkb2::KEY_U},              // U (85)
    {2, m5::unit::cardkb2::KEY_V},              // V (86)
    {2, m5::unit::cardkb2::KEY_W},              // W (87)
    {2, m5::unit::cardkb2::KEY_X},              // X (88)
    {2, m5::unit::cardkb2::KEY_Y},              // Y (89)
    {2, m5::unit::cardkb2::KEY_Z},              // Z (90)
    {4, m5::unit::cardkb2::KEY_F},              // [ (91)
    {4, m5::unit::cardkb2::KEY_R},              // \ (92)
    {4, m5::unit::cardkb2::KEY_G},              // ] (93)
    {4, m5::unit::cardkb2::KEY_D},              // ^ (94)
    {4, m5::unit::cardkb2::KEY_U},              // _ (95)
    {4, m5::unit::cardkb2::KEY_W},              // ` (96)
    {1, m5::unit::cardkb2::KEY_A},              // a (97)
    {1, m5::unit::cardkb2::KEY_B},              // b (98)
    {1, m5::unit::cardkb2::KEY_C},              // c (99)
    {1, m5::unit::cardkb2::KEY_D},              // d (100)
    {1, m5::unit::cardkb2::KEY_E},              // e (101)
    {1, m5::unit::cardkb2::KEY_F},              // f (102)
    {1, m5::unit::cardkb2::KEY_G},              // g (103)
    {1, m5::unit::cardkb2::KEY_H},              // h (104)
    {1, m5::unit::cardkb2::KEY_I},              // i (105)
    {1, m5::unit::cardkb2::KEY_J},              // j (106)
    {1, m5::unit::cardkb2::KEY_K},              // k (107)
    {1, m5::unit::cardkb2::KEY_L},              // l (108)
    {1, m5::unit::cardkb2::KEY_M},              // m (109)
    {1, m5::unit::cardkb2::KEY_N},              // n (110)
    {1, m5::unit::cardkb2::KEY_O},              // o (111)
    {1, m5::unit::cardkb2::KEY_P},              // p (112)
    {1, m5::unit::cardkb2::KEY_Q},              // q (113)
    {1, m5::unit::cardkb2::KEY_R},              // r (114)
    {1, m5::unit::cardkb2::KEY_S},              // s (115)
    {1, m5::unit::cardkb2::KEY_T},              // t (116)
    {1, m5::unit::cardkb2::KEY_U},              // u (117)
    {1, m5::unit::cardkb2::KEY_V},              // v (118)
    {1, m5::unit::cardkb2::KEY_W},              // w (119)
    {1, m5::unit::cardkb2::KEY_X},              // x (120)
    {1, m5::unit::cardkb2::KEY_Y},              // y (121)
    {1, m5::unit::cardkb2::KEY_Z},              // z (122)
    {4, m5::unit::cardkb2::KEY_A},              // { (123)
    {4, m5::unit::cardkb2::KEY_Y},              // | (124)
    {4, m5::unit::cardkb2::KEY_S},              // } (125)
    {4, m5::unit::cardkb2::KEY_Q},              // ~ (126)
    {2, m5::unit::cardkb2::KEY_DELETE},         // DEL (127)
};
static_assert(m5::stl::size(character_map) == 128, "Invalid size");

constexpr std::pair<uint8_t, key_index_t> special_character_map[] = {
    {0x08, m5::unit::cardkb2::KEY_Z},  // Left cursor (Fn+Z, BLE only)
    {0x08, m5::unit::cardkb2::KEY_D},  // Up cursor (Fn+D, BLE only)
    {0x08, m5::unit::cardkb2::KEY_X},  // Down cursor (Fn+X, BLE only)
    {0x08, m5::unit::cardkb2::KEY_C},  // Right cursor (Fn+C, BLE only)
};

// Reverse lookup: Fn character code (128-161) to key index
// Index: fn_char - 128, Value: key_index (0xFF = invalid)
constexpr key_index_t fn_char_to_key_index[] = {
    1,    2,  3,  4,  5,  6,  7,  8,  9,       // 128-136: keys 1-9
    11,   12, 13, 14, 15, 16, 17, 18, 19, 20,  // 137-146: keys 11-20
    0xFF,                                      // 147: gap
    23,   24,                                  // 148-149: keys 23-24
    26,   27, 28, 29, 30, 31,                  // 150-155: keys 26-31
    0xFF,                                      // 156: gap
    38,   39, 40, 41, 42,                      // 157-161: keys 38-42
};

}  // namespace

namespace m5 {
namespace unit {
namespace cardkb2 {

key_index_t character_to_key_index(const char ch)
{
    unsigned char uc = ch;
    // function (>= 0x80)
    if (uc & 0x80) {
        // Special key? (cursor keys)
        if (uc >= (unsigned char)SCHAR_LEFT && uc <= (unsigned char)SCHAR_RIGHT) {
            return special_character_map[uc - (unsigned char)SCHAR_LEFT].second;
        }
        // Fn character reverse lookup
        uint8_t idx = uc - 128;
        return (idx < m5::stl::size(fn_char_to_key_index)) ? fn_char_to_key_index[idx] : 0xFF;
    }
    // normal,shift or symbol
    return static_cast<key_index_t>((uc < m5::stl::size(character_map)) ? (character_map[uc].second) : 0xFF);
}

uint8_t character_to_mode_bits(const char ch)
{
    unsigned char uc = ch;
    // function? (>= 0x80)
    if (uc & 0x80) {
        key_index_t kidx = static_cast<key_index_t>(uc - 0x80);
        // Special key?
        if (uc >= (unsigned char)SCHAR_LEFT && uc <= (unsigned char)SCHAR_RIGHT) {
            // M5_LIB_LOGI("%c => %02X", ch, special_character_map[uc - (unsigned char)SCHAR_LEFT].first);
            return special_character_map[uc - (unsigned char)SCHAR_LEFT].first;
        }

        // M5_LIB_LOGI("%c => %02X", ch, (kidx < m5::stl::size(key_map)) ? 0x08 : 0x00);
        return (kidx < m5::stl::size(key_map)) ? 0x08 : 0x00;
    }
    // normal,shift or symbol
    // M5_LIB_LOGI("%c => %02X", ch, (uc < m5::stl::size(character_map)) ? (character_map[uc].first) : 0x00);
    return (uc < m5::stl::size(character_map)) ? (character_map[uc].first) : 0x00;
}

}  // namespace cardkb2
}  // namespace unit
}  // namespace m5
