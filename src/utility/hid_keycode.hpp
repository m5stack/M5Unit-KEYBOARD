/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file hid_keycode.hpp
  @brief HID Usage Code (Keyboard Page 0x07) → ASCII char translator (US ANSI layout)
 */
#ifndef M5_UNIT_KEYBOARD_UTILITY_HID_KEYCODE_HPP
#define M5_UNIT_KEYBOARD_UTILITY_HID_KEYCODE_HPP

#include <cstdint>

namespace m5 {
namespace unit {
namespace tab5_keyboard {

/*!
  @brief Translate HID Usage Code + modifier to ASCII character (US ANSI layout)
  @param keycode HID Usage ID per USB HID spec Keyboard/Keypad Page 0x07
  @param modifier HID modifier byte. Bit 1 (0x02) = Left Shift, Bit 5 (0x20) = Right Shift.
                  Other bits (Ctrl/Alt/GUI) are ignored.
  @retval !=0 ASCII character (printable or whitespace)
  @retval 0   Not translatable (modifier-only keys, F-keys, navigation, invalid)
  @note Caps Lock state is NOT tracked here; caller is responsible if needed.
  @note US ANSI layout only. JIS / DE / FR layouts unsupported.
  @note Covers HID Usage IDs 0x04..0x38 (letters, digits, common symbols, whitespace).
 */
char hidUsageToChar(const uint8_t keycode, const uint8_t modifier);

/*!
  @brief Convenience: returns true if hidUsageToChar result is printable ASCII (0x20-0x7E)
  @param keycode HID Usage ID
  @param modifier HID modifier byte
  @retval true  Suitable for direct %c display
  @retval false Either not translatable or non-printable control character
 */
bool isPrintableHidKey(const uint8_t keycode, const uint8_t modifier);

}  // namespace tab5_keyboard
}  // namespace unit
}  // namespace m5

#endif  // M5_UNIT_KEYBOARD_UTILITY_HID_KEYCODE_HPP
