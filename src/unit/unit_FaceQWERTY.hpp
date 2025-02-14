/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_FaceQWERTY.hpp
  @brief Face QWERTY Unit for M5UnitUnified
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_FACE_QWERTY_HPP
#define M5_UNIT_KEYBOARD_UNIT_FACE_QWERTY_HPP

#include "unit_Keyboard.hpp"

namespace m5 {
namespace unit {

/*!
  @class m5::unit::UnitFaceQWERTY
  @brief Faces QWERTY keyboard
  @warning Note that this can only be detected if the key is released.
*/
class UnitFaceQWERTY : public UnitKeyboard {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitFaceQWERTY, 0x08);

public:
    explicit UnitFaceQWERTY(const uint8_t addr = DEFAULT_ADDRESS) : UnitKeyboard(addr)
    {
    }
    virtual bool begin() override;
    virtual void update(const bool force = false) override;

#if defined(DOXYGEN_PROCESS)
    /*!
     @copydoc m5::unit::Keyboard::released
     @note Enter key is returned by 2 bytes of [0x0D, 0X0A] from Firmware, but this class treats it as 0x0D
    */
    uint8_t released() const;
#endif
};

}  // namespace unit
}  // namespace m5
#endif
