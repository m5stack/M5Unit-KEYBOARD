/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB2_defs.hpp
  @brief Shared constants and types for CardKB2 I2C and UART classes
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_CARD_KB2_DEFS_HPP
#define M5_UNIT_KEYBOARD_UNIT_CARD_KB2_DEFS_HPP

#include "unit_Keyboard.hpp"

namespace m5 {
namespace unit {
namespace cardkb2 {

constexpr uint8_t NUMBER_OF_KEYS{43};

///@name key index (bit position in scan result)
///@{
constexpr keyboard::key_index_t KEY_1{0};
constexpr keyboard::key_index_t KEY_2{1};
constexpr keyboard::key_index_t KEY_3{2};
constexpr keyboard::key_index_t KEY_4{3};
constexpr keyboard::key_index_t KEY_5{4};
constexpr keyboard::key_index_t KEY_6{5};
constexpr keyboard::key_index_t KEY_7{6};
constexpr keyboard::key_index_t KEY_8{7};
constexpr keyboard::key_index_t KEY_9{8};
constexpr keyboard::key_index_t KEY_0{9};
constexpr keyboard::key_index_t KEY_Q{11};
constexpr keyboard::key_index_t KEY_W{12};
constexpr keyboard::key_index_t KEY_E{13};
constexpr keyboard::key_index_t KEY_R{14};
constexpr keyboard::key_index_t KEY_T{15};
constexpr keyboard::key_index_t KEY_Y{16};
constexpr keyboard::key_index_t KEY_U{17};
constexpr keyboard::key_index_t KEY_I{18};
constexpr keyboard::key_index_t KEY_O{19};
constexpr keyboard::key_index_t KEY_P{20};
constexpr keyboard::key_index_t KEY_DELETE{21};
constexpr keyboard::key_index_t KEY_AA{22};
constexpr keyboard::key_index_t KEY_A{23};
constexpr keyboard::key_index_t KEY_S{24};
constexpr keyboard::key_index_t KEY_D{25};
constexpr keyboard::key_index_t KEY_F{26};
constexpr keyboard::key_index_t KEY_G{27};
constexpr keyboard::key_index_t KEY_H{28};
constexpr keyboard::key_index_t KEY_J{29};
constexpr keyboard::key_index_t KEY_K{30};
constexpr keyboard::key_index_t KEY_L{31};
constexpr keyboard::key_index_t KEY_ENTER{32};
constexpr keyboard::key_index_t KEY_FN{33};
constexpr keyboard::key_index_t KEY_SYM{34};
constexpr keyboard::key_index_t KEY_Z{35};
constexpr keyboard::key_index_t KEY_X{36};
constexpr keyboard::key_index_t KEY_C{37};
constexpr keyboard::key_index_t KEY_V{38};
constexpr keyboard::key_index_t KEY_B{39};
constexpr keyboard::key_index_t KEY_N{40};
constexpr keyboard::key_index_t KEY_M{41};
constexpr keyboard::key_index_t KEY_SPACE{42};
///@}

///@name Character code for special keys
///@{
constexpr char SCHAR_LEFT{(char)180};
constexpr char SCHAR_UP{(char)181};
constexpr char SCHAR_DOWN{(char)182};
constexpr char SCHAR_RIGHT{(char)183};
///@}

/*!
  @brief Character to key index
  @retval != 0xFF keyboard::key_index_t
  @retval == 0xFF No corresponding key index exists
 */
keyboard::key_index_t character_to_key_index(const char ch);

/*!
  @brief Character to mode bits
  @retval == 0 Not exists
  @retval != 0 Bits in corresponding mode
  @note 0x01:normal 0x02:shift 0x04:symbol 0x08:function
 */
uint8_t character_to_mode_bits(const char ch);

}  // namespace cardkb2
}  // namespace unit
}  // namespace m5
#endif
