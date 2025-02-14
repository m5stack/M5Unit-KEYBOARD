/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_Keyboard.hpp
  @brief Base class for units
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_KEYBOARD_HPP
#define M5_UNIT_KEYBOARD_UNIT_KEYBOARD_HPP

#include <M5UnitComponent.hpp>

namespace m5 {
namespace unit {

/*!
  @class m5::unit::UnitKeyboard
  @brief Base class of the Keyoard Unit/Face
  @note The key can only be retrieved when it is released
*/
class UnitKeyboard : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitKeyboard, 0x5F);

public:
    explicit UnitKeyboard(const uint8_t addr = 0x00) : Component(addr)
    {
        auto ccfg  = component_config();
        ccfg.clock = 100 * 1000U;
        component_config(ccfg);
    }
    virtual ~UnitKeyboard()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    /*!
      @brief Get released key code if updated
      @retval !=0 Released character code
      @retval ==0 There is no released key
     */
    uint8_t released() const
    {
        return updated() ? _released_key : 0;
    }

protected:
    uint8_t _released_key{};
};

}  // namespace unit
}  // namespace m5
#endif
