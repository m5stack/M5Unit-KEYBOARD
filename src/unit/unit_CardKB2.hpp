/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB2.hpp
  @brief CardKB2 Unit for M5UnitUnified (I2C mode)
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_CARD_KB2_HPP
#define M5_UNIT_KEYBOARD_UNIT_CARD_KB2_HPP

#include "unit_CardKB2_defs.hpp"

namespace m5 {
namespace unit {
/*!
  @class m5::unit::UnitCardKB2
  @brief Card-size 42 key QWERTY keyboard — I2C mode (SKU:U215)

  CardKB2 supports I2C and UART communication via GROVE port, selectable on the device:
  - **Fn+Sym+1**: I2C mode (factory default) — returns ASCII per keypress
  - **Fn+Sym+2**: UART mode (115200-8N1) — use UnitCardKB2UART for this mode

  The selected mode is saved and persists across power cycles.

  In I2C mode, the firmware returns ASCII on key press (not release) and auto-repeats
  after 300ms hold at 50ms intervals.  Use `getchar()` after `updated()` to retrieve
  the pressed character.

  @note Sym key operates as a toggle (press once to activate, press again to deactivate).
  Blue LED indicates Sym mode is active.
  @note Fn+D/Z/X/C arrow keys are not available in I2C/UART mode.
  @note Unlike CardKB/FacesQWERTY, CardKB2 does not support software mode switching via readMode()/writeMode().
  @warning After switching communication mode (Fn+Sym+1/2), press the RST button on
  CardKB2 to reset modifier state. This will be fixed in a future firmware update.
*/
class UnitCardKB2 : public UnitKeyboard {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitCardKB2, 0x5F);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Periodic interval (ms)
        uint32_t interval{10};
    };

    explicit UnitCardKB2(const uint8_t addr = DEFAULT_ADDRESS) : UnitKeyboard(addr)
    {
    }
    //! @copydoc Component::begin
    virtual bool begin() override;
    //! @copydoc Component::update
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configuration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configuration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    /*!
      @brief Gets the character if input
      @retval != 0 Pressed character
      @retval == 0 Not input or invalid character
      @note CardKB2 I2C firmware sends ASCII on key press (not release)
     */
    inline virtual char getchar() const override
    {
        return updated() ? _pressed_key : 0;
    }
    //! @brief Number of available characters (0 or 1)
    //! @note Provided for API compatibility with UnitKeyboardBitwise (UnitCardKB2UART)
    inline uint8_t available() const
    {
        return (updated() && _pressed_key) ? 1 : 0;
    }
    //! @brief Discard current character
    //! @note Provided for API compatibility with UnitKeyboardBitwise (UnitCardKB2UART)
    inline void discard()
    {
        _pressed_key = 0;
    }

    ///@name Firmware
    ///@{
    /*!
      @brief Gets the firmware version
      @retval Firmware version read during begin()
      @warning Valid after begin
    */
    inline uint8_t firmwareVersion() const
    {
        return _firmware_version;
    }
    /*!
      @brief Read the firmware version
      @param[out] ver Version
      @return True if successful
     */
    bool readFirmwareVersion(uint8_t& ver);
    ///@}

protected:
    config_t _cfg{};
    uint8_t _pressed_key{};
    uint8_t _firmware_version{};
};

}  // namespace unit
}  // namespace m5
#endif
