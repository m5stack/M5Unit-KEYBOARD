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
/*!
  @enum Mode
  @brief Operation mode for M5UnitU-KEYBOARD firmware
 */
enum class Mode : uint8_t {
    Released,  //!< Gets the released key (Conventional behavior)
    Scan,      //!< Gets the pressed key status (M5Unit-KEYBOARD firmware must be written)
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
      @retval != 0 Released character code
      @retval == 0 There is no released key
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
class UnitKeyboardBitwise : public UnitKeyboard {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitKeyboardBitwise, 0x00);

public:
    explicit UnitKeyboardBitwise(const uint8_t addr = DEFAULT_ADDRESS)
        : UnitKeyboard(addr), _inputs{new m5::container::CircularBuffer<uint8_t>(1)}
    {
    }
    virtual ~UnitKeyboardBitwise()
    {
    }

    virtual void update(const bool force = false) override;

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@note Which bits represent what depends on the target unit
    ///@name Key status bits if updated
    ///@{
    //! @brief Get the bits of the key being pressed
    inline uint64_t nowBits() const
    {
        return _now;
    }
    //! @brief Get the bits of the previous key pressed
    inline uint64_t previousBits() const
    {
        return _prev;
    }
    //! @brief Get the key bits at the moment they are pressed
    inline uint64_t pressedBits() const
    {
        return _wasPressed;
    }
    //! @brief Get the key bits at the moment they are released
    inline uint64_t releasedBits() const
    {
        return _wasReleased;
    }
    //! @brief Get the bits of the held key
    inline uint64_t holidngBits() const
    {
        return _holding;
    }
    //! @brief Get the bits of the key at the moment of hold
    inline uint64_t holdgBits() const
    {
        return _wasHold;
    }
    //! @brief Get the bits of the key that the software is repeatedly pressing
    inline uint64_t repeatingBits() const
    {
        return _repeating;
    }
    //! @brief Get the bits of the modifier key being pressed
    inline uint64_t modifierBits() const
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
        return modifier_bits();
    }
    //! @brief Is the shift key pressed?
    inline virtual bool isShift() const
    {
        return false;
    }
    //! @brief Is the symbol key pressed?
    inline virtual bool isSymbol() const
    {
        return false;
    }
    //! @brief Is the function key pressed?
    inline virtual bool isFunction() const
    {
        return false;
    }
    //! @brief Is the alt key pressed?
    inline virtual bool isAlt() const
    {
        return false;
    }
    //! @brief Is only Shift pressed among the modifier keys?
    inline virtual bool isShiftEqual() const
    {
        return false;
    }
    //! @brief Is only Symbol pressed among the modifier keys?
    inline virtual bool isSymbolEqual() const
    {
        return false;
    }
    //! @brief Is only Function pressed among the modifier keys?
    inline virtual bool isFunctionEqual() const
    {
        return false;
    }
    //! @brief Is only Alt pressed among the modifier keys?
    inline virtual bool isAltEqual() const
    {
        return false;
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
      @brief Is the specified character pressed?
      @param ch Character
      @return If so,true
    */
    inline bool isPressed(const char ch) const
    {
        return isPressed(to_key_index(ch)) && permitted_mode(to_mode_bits(ch));
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
        return wasPressed(to_key_index(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Was the specified character released?
      @param ch Character
      @return If so,true
    */
    inline bool wasReleased(const char ch) const
    {
        return wasReleased(to_key_index(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Is the specified character holding?
      @param ch Character
      @return If so,true
    */
    inline bool isHolding(const char ch) const
    {
        return isHolding(to_key_index(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Was the specified character hold?
      @param ch Character
      @return If so,true
    */
    inline bool wasHold(const char ch) const
    {
        return wasHold(to_key_index(ch)) && permitted_mode(to_mode_bits(ch));
    }
    /*!
      @brief Is the specified character repeating?
      @param ch Character
      @return If so,true
    */
    inline bool isRepeating(const char ch) const
    {
        return isRepeating(to_key_index(ch)) && permitted_mode(to_mode_bits(ch));
    }
    ///@}

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@name Get key (Was Pressed) if updated
    ///@{
    //! @brief Get the oldest pressed key
    inline uint8_t pressed() const
    {
        return !empty() ? oldest() : 0x00;
    }
    //! @brief Get the oldest released key
    inline virtual uint8_t released() const override
    {
        return (_mode == keyboard::Mode::Released) ? UnitKeyboard::released() : 0x00;
    }
    //! @brief Available pressed keys buffer
    inline uint32_t available() const
    {
        return _inputs->size();
    }
    //! @brief Is the key pressed buffer empty?
    inline bool empty() const
    {
        return _inputs->empty();
    }
    //! @brief Is the key pressed buffer full?
    inline bool full() const
    {
        return _inputs->full();
    }
    //! @brief Discard oldest pressed
    inline void discard() const
    {
        _inputs->pop_front();
    }
    //! @brief Discard all pressed
    inline void flush() const
    {
        _inputs->clear();
    }
    //! @brief Get the oldest pressed key
    inline uint8_t oldest() const
    {
        return !empty() ? _inputs->front().value() : 0x00;
    }
    //! @brief Get the latest pressed key
    inline uint8_t latest() const
    {
        return !empty() ? _inputs->back().value() : 0x00;
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
    bool update_new_firmware(const types::elapsed_time_t at);
    void push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx, const uint8_t alt);

    inline virtual keyboard::key_index_t to_key_index(const char) const
    {
        return 0x00;
    }
    inline virtual uint8_t to_mode_bits(const char) const
    {
        return 0x00;
    }
    inline virtual uint64_t modifier_bits() const
    {
        return (uint64_t)0;
    }
    inline virtual uint8_t mode_bits() const
    {
        return 0x00;
    }
    inline bool permitted_mode(const uint8_t mbits) const
    {
        return mbits & mode_bits();
    }

protected:
    std::unique_ptr<m5::container::CircularBuffer<uint8_t>> _inputs{};                              // was Presed keys
    uint64_t _now{}, _prev{}, _wasPressed{}, _wasReleased{}, _wasHold{}, _holding{}, _repeating{};  // key bits
    std::vector<types::elapsed_time_t> _repeat_start_at{}, _hold_start_at{};
    uint32_t _repeating_threshold{400}, _holding_threshold{800};
    uint8_t _firmware_version{};
    keyboard::Mode _mode{keyboard::Mode::Released};
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
