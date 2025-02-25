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
#include <vector>

namespace m5 {
namespace unit {

/*!
  @namespace keyboard
  @brief For keyboard
 */
namespace keyboard {
using key_index_t = uint8_t;  //!< @brief Key index (Not character)

using key_status_bits_t = uint64_t;  //!< @brief key state bits [56...63]:modifier, bits [0...55]:key bits
// clang-format off
///@name Modifier's bit [56...63]
///@{
constexpr key_status_bits_t MODIFIER_SHIFT_BIT   {0x0100000000000000};  //!< Shift
constexpr key_status_bits_t MODIFIER_SYMBOL_BIT  {0x0200000000000000};  //!< Symbol
constexpr key_status_bits_t MODIFIER_FUNCTION_BIT{0x0400000000000000};  //!< Function
constexpr key_status_bits_t MODIFIER_ALT_BIT     {0x0800000000000000};  //!< Alt
constexpr key_status_bits_t MODIFIER_CONTROL_BIT {0x1000000000000000};  //!< Control
constexpr key_status_bits_t MODIFIER_OPTION_BIT  {0x2000000000000000};  //!< Option
///@}
// clang-format on

//! @brief Gets the modifier bits from key_status_bits_t
constexpr inline key_status_bits_t modifier_bits(const key_status_bits_t kbs)
{
    return kbs & (MODIFIER_SHIFT_BIT | MODIFIER_SYMBOL_BIT | MODIFIER_FUNCTION_BIT | MODIFIER_ALT_BIT |
                  MODIFIER_CONTROL_BIT | MODIFIER_OPTION_BIT);
}

/*!
  @enum Mode
  @brief Operation mode for M5UnitU-KEYBOARD firmware
 */
enum class Mode : uint8_t {
    /*!
      Conventional behavior
      @details CardKB,FacesQWERTY:Gets the released key
     */
    Conventional,
    /*!
      M5Unit-KEYBOARD mode  behavior
      @details CardKB, FacesQWERTY:Gets the pressed key status
      @warning M5Unit-KEYBOARD firmware must be written
     */
    M5UnitUnified,
};
}  // namespace keyboard

/*!
  @class m5::unit::UnitKeyboard
  @brief Base class of the Keyboard Unit
*/
class UnitKeyboard : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitKeyboard, 0x00);

public:
    // 0x5F:CardKB 0x08:FacesQWERTY
    explicit UnitKeyboard(const uint8_t addr = DEFAULT_ADDRESS) : Component(addr)
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
      @brief Gets the character if input
      @retval != 0 Character
      @retval == 0 Not input or invalid character
      @note Whether the input is a released or pressed key depends on the derived class
     */
    inline virtual char getchar() const
    {
        return released();
    }

    /*!
      @brief Gets the released key character code if updated
      @retval != 0 Released character code
      @retval == 0 There is no released key
     */
    inline virtual uint8_t released() const
    {
        return updated() ? _released_key : 0;
    }

private:
    uint8_t _released_key{};  // for old firmware
};

/*!
  @class m5::unit::UnitKeyboard
  @brief Class supporting keyboard state acquisition by key press bits
  @warning To make it work, M5Unit-KEYBOARD firmware must be written to the target (CardKB, FacesQWERTY...)
*/
class UnitKeyboardBitwise : public UnitKeyboard, public PeriodicMeasurementAdapter<UnitKeyboardBitwise, uint8_t> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitKeyboardBitwise, 0x00);

public:
    explicit UnitKeyboardBitwise(const uint8_t addr = DEFAULT_ADDRESS)
        : UnitKeyboard(addr), _data{new m5::container::CircularBuffer<uint8_t>(1)}
    {
    }
    virtual ~UnitKeyboardBitwise()
    {
    }

    virtual void update(const bool force = false) override;

    ///@name Measurement data by periodic
    ///@{
    /*!
      @brief Gets the character if input
      @retval != 0 Character
      @retval == 0 Not input or invalid character
    */
    inline virtual char getchar() const override
    {
        return (_mode == keyboard::Mode::M5UnitUnified) ? pressed() : released();
    }
    /*!
      @brief Get the oldest pressed key
      @warning API valid only if using M5Unit-KEYBOARD firmware
    */
    inline uint8_t pressed() const
    {
        return !empty() ? oldest() : 0x00;
    }
    /*!
      @brief Get the oldest released key
      @warning Disabled in all modes except Conventional mode
    */
    inline virtual uint8_t released() const override
    {
        return (_mode == keyboard::Mode::Conventional) ? UnitKeyboard::released() : 0x00;
    }
    ///@}

    ///@name Periodic measurement
    ///@{
    /*!
      @brief Start periodic measurement
      @param interval Update interval time(ms)
      @return True if successful
      @warning If config_t::trigger_irq is set to true, arguments is ignored
    */
    inline bool startPeriodicMeasurement(const uint32_t interval)
    {
        return PeriodicMeasurementAdapter<UnitKeyboardBitwise, uint8_t>::startPeriodicMeasurement(interval);
    }
    //! @brief Start periodic measurement using current settings
    inline bool startPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitKeyboardBitwise, uint8_t>::startPeriodicMeasurement();
    }
    /*!
      @brief Stop periodic measurement
      @return True if successful
    */
    inline bool stopPeriodicMeasurement()
    {
        return PeriodicMeasurementAdapter<UnitKeyboardBitwise, uint8_t>::stopPeriodicMeasurement();
    }
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@note Which bits represent what depends on the target unit
    ///@name Key status bits if updated
    ///@{
    //! @brief Get the bits of the key being pressed
    inline keyboard::key_status_bits_t nowBits() const
    {
        return _now;
    }
    //! @brief Get the bits of the previous key pressed
    inline keyboard::key_status_bits_t previousBits() const
    {
        return _prev;
    }
    //! @brief Get the key bits at the moment they are pressed
    inline keyboard::key_status_bits_t pressedBits() const
    {
        return _wasPressed;
    }
    //! @brief Get the key bits at the moment they are released
    inline keyboard::key_status_bits_t releasedBits() const
    {
        return _wasReleased;
    }
    //! @brief Get the bits of the held key
    inline keyboard::key_status_bits_t holidngBits() const
    {
        return _holding;
    }
    //! @brief Get the bits of the key at the moment of hold
    inline keyboard::key_status_bits_t holdgBits() const
    {
        return _wasHold;
    }
    //! @brief Get the bits of the key that the software is repeatedly pressing
    inline keyboard::key_status_bits_t repeatingBits() const
    {
        return _repeating;
    }
    //! @brief Get the bits of the modifier key being pressed
    inline keyboard::key_status_bits_t modifierBits() const
    {
        return modifier_bits();
    }
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@note Some keys do not exist depending on the target Unit
    ///@name Modifier
    ///@{
    //! @brief Is any modifier keys pressed?
    inline bool isModifier() const
    {
        return modifier_bits() != 0;
    }
    //! @brief Is the shift key pressed?
    inline bool isShift() const
    {
        return modifier_bits() & keyboard::MODIFIER_SHIFT_BIT;
    }
    //! @brief Is the symbol key pressed?
    inline bool isSymbol() const
    {
        return modifier_bits() & keyboard::MODIFIER_SYMBOL_BIT;
    }
    //! @brief Is the function key pressed?
    inline bool isFunction() const
    {
        return modifier_bits() & keyboard::MODIFIER_FUNCTION_BIT;
    }
    //! @brief Is the alt key pressed?
    inline bool isAlt() const
    {
        return modifier_bits() & keyboard::MODIFIER_ALT_BIT;
    }
    //! @brief Is the control key pressed?
    inline bool isControl() const
    {
        return modifier_bits() & keyboard::MODIFIER_CONTROL_BIT;
    }
    //! @brief Is the option key pressed?
    inline bool isOption() const
    {
        return modifier_bits() & keyboard::MODIFIER_OPTION_BIT;
    }

    //! @brief Is only shift pressed among the modifier keys?
    inline bool isShiftEqual() const
    {
        return modifier_bits() == keyboard::MODIFIER_SHIFT_BIT;
    }
    //! @brief Is only symbol pressed among the modifier keys?
    inline bool isSymbolEqual() const
    {
        return modifier_bits() == keyboard::MODIFIER_SYMBOL_BIT;
    }
    //! @brief Is only function pressed among the modifier keys?
    inline bool isFunctionEqual() const
    {
        return modifier_bits() == keyboard::MODIFIER_FUNCTION_BIT;
    }
    //! @brief Is only alt pressed among the modifier keys?
    inline bool isAltEqual() const
    {
        return modifier_bits() == keyboard::MODIFIER_ALT_BIT;
    }
    //! @brief Is only control pressed among the modifier keys?
    inline bool isControlEqual() const
    {
        return modifier_bits() == keyboard::MODIFIER_CONTROL_BIT;
    }
    //! @brief Is only option pressed among the modifier keys?
    inline bool isOptionEqual() const
    {
        return modifier_bits() == keyboard::MODIFIER_OPTION_BIT;
    }
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@name Any key
    ///@{
    //! @brief  Is any key press?
    inline bool isPressed() const
    {
        return _now != 0;
    }
    //! @brief Is all key release?
    inline bool isReleased() const
    {
        return !isPressed();
    }
    //! @brief Was any key pressed?
    inline bool wasPressed() const
    {
        return _wasPressed;
    }
    //! @brief Was any key released?
    inline bool wasReleased() const
    {
        return _wasReleased;
    }
    //! @brief Is any key holding?
    inline bool isHolding() const
    {
        return _holding;
    }
    //! @brief Was any key hold?
    inline bool wasHold() const
    {
        return _wasHold;
    }
    //! @brief Is any key repeating?
    inline bool isRepeating() const
    {
        return _repeating;
    }
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@name Specified key (Key index)
    ///@{
    /*!
      @brief Is the specified key pressed?
      @param kidx Key index code
      @return If so,true
    */
    // inline
    bool isPressed(const keyboard::key_index_t kidx) const
    {
        return _now & (1ULL << kidx);
    }
    /*!
      @brief Is the specified key released??
      @param kidx Key index code
      @return If so,true
    */
    inline bool isReleased(const keyboard::key_index_t kidx) const
    {
        return !isPressed(kidx);
    }
    /*!
      @brief Was the specified key pressed?
      @param kidx Key index code
      @return If so,true
    */
    inline bool wasPressed(const keyboard::key_index_t kidx) const
    {
        return _wasPressed & (1ULL << kidx);
    }
    /*!
      @brief Was the specified key released?
      @param kidx Key index code
      @return If so,true
    */
    inline bool wasReleased(const keyboard::key_index_t kidx) const
    {
        return _wasReleased & (1ULL << kidx);
    }
    /*!
      @brief Is the specified key holding?
      @param kidx Key index code
      @return If so,true
    */
    inline bool isHolding(const keyboard::key_index_t kidx) const
    {
        return _holding & (1ULL << kidx);
    }
    /*!
      @brief Was the specified key hold?
      @param kidx Key index code
      @return If so,true
    */
    inline bool wasHold(const keyboard::key_index_t kidx) const
    {
        return _wasHold & (1ULL << kidx);
    }
    /*!
      @brief Is the specified key repeating?
      @param kidx Key index code
      @return If so,true
    */
    inline bool isRepeating(const keyboard::key_index_t kidx) const
    {
        return _repeating & (1ULL << kidx);
    }
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@name Specified Character
    ///@{
    /*!
      @brief Obtains the key index corresponding to the specified character
      @retval != 0xFF key index
      @retval == 0xFF No corresponding key index
     */
    inline virtual keyboard::key_index_t toKeyIndex(const char) const
    {
        return 0x00;
    }
    /*!
      @brief Is the specified character pressed?
      @param ch Character
      @return If so,true
    */
    inline bool isPressed(const char ch) const
    {
        return isPressed(toKeyIndex(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Is the specified character released??
      @param ch Character
      @return If so,true
    */
    inline bool isReleased(const char ch) const
    {
        return !isPressed(ch);
    }
    /*!
      @brief Was the specified character pressed?
      @param ch Character
      @return If so,true
    */
    inline bool wasPressed(const char ch) const
    {
        return wasPressed(toKeyIndex(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Was the specified character released?
      @param ch Character
      @return If so,true
    */
    inline bool wasReleased(const char ch) const
    {
        return wasReleased(toKeyIndex(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Is the specified character holding?
      @param ch Character
      @return If so,true
    */
    inline bool isHolding(const char ch) const
    {
        return isHolding(toKeyIndex(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Was the specified character hold?
      @param ch Character
      @return If so,true
    */
    inline bool wasHold(const char ch) const
    {
        return wasHold(toKeyIndex(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Is the specified character repeating?
      @param ch Character
      @return If so,true
    */
    inline bool isRepeating(const char ch) const
    {
        return isRepeating(toKeyIndex(ch)) && permitted_mode(to_mode_bits(ch));
    }
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@name Firmware
    ///@{
    /*!
      @brief Gets the firmware version
      @retval == 0 Conventional firmware
      @retval != 0 M5UnitUnified firmware version
      @warning Valid after begin
    */
    inline uint8_t firmwareVersion() const
    {
        return _firmware_version;
    }
    /*!
      @brief Read the firmware version
      @param[out] ver Version high nibble:Major, low nibble:Minor
      @return True if successful
     */
    bool readFirmwareVersion(uint8_t& ver);
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@name Repeat/Hold threshold
    ///@{
    inline uint32_t holdingThreshold() const
    {
        return _holding_threshold;
    }
    inline uint32_t repeatingThreshold() const
    {
        return _repeating_threshold;
    }
    inline void setHoldingThreshold(const uint32_t ms)
    {
        _holding_threshold = ms;
    }
    inline void setRepeatingThreshold(const uint32_t ms)
    {
        _repeating_threshold = ms;
    }
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@name Mode
    ///@{
    /*!
      @brief Read the mode
      @param[out] mode Mode
      @return True if successful
     */
    bool readMode(keyboard::Mode& mode);
    /*!
      @brief Read the mode
      @param mode Mode
      @return True if successful
     */
    bool writeMode(const keyboard::Mode mode);
    ///@}

protected:
    bool start_periodic_measurement();
    bool start_periodic_measurement(const uint32_t interval);
    bool stop_periodic_measurement();

    bool update_new_firmware(const types::elapsed_time_t at);
    void push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx, const uint8_t alt);

    inline keyboard::key_status_bits_t modifier_bits() const
    {
        return keyboard::modifier_bits(_now);
    }

    inline virtual uint8_t to_mode_bits(const char) const
    {
        return 0x00;
    }

    bool permitted_mode(const uint8_t mbits) const
    {
        uint8_t mod8 = _now >> 56;
        return mbits & (1U << (mod8 ? __builtin_ctz(mod8) + 1 : 0));
    }

    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitKeyboardBitwise, uint8_t);

protected:
    std::unique_ptr<m5::container::CircularBuffer<uint8_t>> _data{};  // was Presed keys
    keyboard::key_status_bits_t _now{}, _prev{}, _wasPressed{}, _wasReleased{}, _wasHold{}, _holding{},
        _repeating{};  // key bits
    std::vector<types::elapsed_time_t> _repeat_start_at{}, _hold_start_at{};
    uint32_t _repeating_threshold{400}, _holding_threshold{800};
    uint8_t _firmware_version{};
    keyboard::Mode _mode{keyboard::Mode::Conventional};
};

namespace keyboard {
namespace command {
constexpr uint8_t CMD_SCAN_REG{0x10};
constexpr uint8_t CMD_MODE_REG{0x20};
constexpr uint8_t CMD_FIRMWARE_VERSION_REG{0xFE};
}  // namespace command
}  // namespace keyboard

}  // namespace unit
}  // namespace m5
#endif
