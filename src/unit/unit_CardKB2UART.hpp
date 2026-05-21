/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB2UART.hpp
  @brief CardKB2 Unit for M5UnitUnified (UART mode)
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_CARD_KB2_UART_HPP
#define M5_UNIT_KEYBOARD_UNIT_CARD_KB2_UART_HPP

#include "unit_CardKB2_defs.hpp"
#include "../utility/button_event_detector.hpp"
#include <m5_utility/container/circular_buffer.hpp>
#include <array>

namespace m5 {
namespace unit {
/*!
  @class m5::unit::UnitCardKB2UART
  @brief Card-size 42 key QWERTY keyboard — UART mode (SKU:U215)

  CardKB2 supports I2C and UART communication via GROVE port, selectable on the device:
  - **Fn+Sym+1**: I2C mode (factory default) — use UnitCardKB2 for this mode
  - **Fn+Sym+2**: UART mode (115200-8N1) — sends KEY_ID + KEY_STATE packets

  The selected mode is saved and persists across power cycles.

  In UART mode, press/release key events are received as packets, enabling full
  bitwise key state tracking (isPressed, isHolding, isRepeating, etc.).

  @note Sym key operates as a toggle (press once to activate, press again to deactivate).
  Blue LED indicates Sym mode is active.
  @note Fn+D/Z/X/C arrow keys work in UART mode (hold Fn, then press D/Z/X/C).
  Fn+1 (ESC) may require a long press due to a firmware issue.
  @note Unlike CardKB/FacesQWERTY, CardKB2 does not support software mode switching via readMode()/writeMode().
  @warning After switching communication mode (Fn+Sym+1/2), press the RST button on
  CardKB2 to reset modifier state. This will be fixed in a future firmware update.
*/
class UnitCardKB2UART : public UnitKeyboardBitwise {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitCardKB2UART, 0x5F);

public:
    //! @brief 5-byte UART packet: {0xAA, 0x03, key_id, key_state, checksum}
    using Packet = std::array<uint8_t, 5>;

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

    /*!
      @brief Constructor
      @param addr Reserved for API compatibility (UART transport ignores I2C address)
     */
    explicit UnitCardKB2UART(const uint8_t addr = DEFAULT_ADDRESS) : UnitKeyboardBitwise(addr)
    {
    }
    //! @copydoc Component::begin
    virtual bool begin() override;
    //! @copydoc Component::update
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configuration */
    inline config_t config() const
    {
        return _cfg;
    }
    /*!
      @brief Set the configuration
      @param cfg Configuration to apply
     */
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    //! @copydoc UnitKeyboardBitwise::toKeyIndex
    inline virtual keyboard::key_index_t toKeyIndex(const char ch) const override
    {
        return cardkb2::character_to_key_index(ch);
    }

    ///@name Firmware
    ///@{
    /*!
      @brief Read the firmware version
      @param[out] ver Version
      @return Always false (UART protocol has no version query command)
     */
    virtual bool readFirmwareVersion(uint8_t& ver) override;
    ///@}

    ///@name Mode
    ///@{
    /*!
      @brief Not supported on CardKB2UART
      @return Always false
     */
    virtual bool readMode(keyboard::Mode&) override
    {
        return false;
    }
    /*!
      @brief Not supported on CardKB2UART
      @return Always false
     */
    virtual bool writeMode(const keyboard::Mode) override
    {
        return false;
    }
    ///@}

protected:
    void update_uart(const bool force);
    uint8_t read_data(Packet& rbuf);

    inline virtual uint8_t to_mode_bits(const char ch) const override
    {
        return cardkb2::character_to_mode_bits(ch);
    }

protected:
    config_t _cfg{};

private:
    m5::unit::keyboard_bitwise::ButtonEventDetector _sym_detector{};
    bool _sym_mode{false};  // Sym toggle (mirrors firmware sym_mode / blue LED)
    bool _caps_shift_once{false};
    bool _caps_lock{false};
    bool _caps_hold_active{false};
    bool _caps_pressing{false};
    uint8_t _caps_click_count{0};
    uint32_t _caps_pressed_at{0};
    uint32_t _caps_last_release_at{0};
};

}  // namespace unit
}  // namespace m5
#endif
