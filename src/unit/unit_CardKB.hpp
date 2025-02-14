/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB.hpp
  @brief CardKB Unit for M5UnitUnified
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_CARDKB_HPP
#define M5_UNIT_KEYBOARD_UNIT_CARDKB_HPP

#include "unit_Keyboard.hpp"
#include <m5_utility/container/circular_buffer.hpp>

namespace m5 {
namespace unit {

/*!
  @namespace cardkb
  @brief For CardKB
 */
namespace cardkb {

///@sa m5::unit::UnitCardKB::readHardwareType
///@name Hardware type
///@{
constexpr uint8_t TYPE_CARDKB{0x01};      //!< SKU:U035
constexpr uint8_t TYPE_CARDKB_V11{0x11};  //!< V11 SKU:U035-B
///@}

/*!
  @enum Mode
  @brief Operqtion mode
 */
enum class Mode : uint8_t {
    Released,  //!< Get released key (conventional behavior)
    Scan,      //!< Get scaned key status (UnitUnified firmware must be written)
};

}  // namespace cardkb

/*!
  @class m5::unit::UnitCardKB
  @brief Card-size 50 key QWERTY keyboard
  @warning Note that older firmware can only detect if the key is released
*/
class UnitCardKB : public UnitKeyboard {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitCardKB, 0x5F);

public:
    static constexpr uint8_t NUMBER_OF_KEYS{48};

    /*!
      @enum key_index_t
      @brief Key index code (Not character code)
    */
    enum key_index_t : uint8_t {
        KEY_ESC,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_0,
        KEY_DEL,
        KEY_TAB,
        KEY_Q,
        KEY_W,
        KEY_E,
        KEY_R,
        KEY_T,
        KEY_Y,
        KEY_U,
        KEY_I,
        KEY_O,
        KEY_P,
        KEY_NO_KEY,  //!< Not used
        KEY_LEFT,
        KEY_UP,
        KEY_A,
        KEY_S,
        KEY_D,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_ENTER,
        KEY_DOWN,
        KEY_RIGHT,
        KEY_Z,
        KEY_X,
        KEY_C,
        KEY_V,
        KEY_B,
        KEY_N,
        KEY_M,
        KEY_COMMA,
        KEY_PERIOD,
        KEY_SPACE,
    };

    static constexpr uint8_t ALT_SHIFT_BIT{0x10};
    static constexpr uint8_t ALT_SYMBOL_BIT{0x80};
    static constexpr uint8_t ALT_FUNCTION_BIT{0x40};


    
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        ///@name For UnitUnified firmware
        ///@{
        /*! Mode */
        cardkb::Mode mode{cardkb::Mode::Scan};
        //! How many simultaneous inputs to stored
        uint32_t stored_keys{4};
        //! Periodic interval
        uint32_t interval{10};
        //! Threshold for key repeating (ms)
        uint32_t repeating_threshold{500};
        //! Threshold for key holding (ms)
        uint32_t holding_threshold{1000};
        ///@}
    };

    explicit UnitCardKB(const uint8_t addr = DEFAULT_ADDRESS)
        : UnitKeyboard(addr),
          _pressed{new m5::container::CircularBuffer<uint8_t>(1)},
          _released{new m5::container::CircularBuffer<uint8_t>(1)}
    {
    }
    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Settings for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    ///@warning API valid only if using UnitUnified firmware
    ///@note Bit 0...47 is key index, bit 52 is shift, bit 54 is function, bit 55 is symbol
    ///@name Key status bits if updated
    ///@{
    inline uint64_t nowBits() const
    {
        return _now;
    }
    inline uint64_t prevBits() const
    {
        return _prev;
    }
    inline uint64_t pressedBits() const
    {
        return _wasPressed;
    }
    inline uint64_t releasedBits() const
    {
        return _wasReleased;
    }
    inline uint64_t holidngBits() const
    {
        return _holding;
    }
    inline uint64_t holdgBits() const
    {
        return _wasHold;
    }
    inline uint64_t repeatgBits() const
    {
        return _repeating;
    }

    inline bool isAlt() const
    {
        return (_now >> (6 * 8)) & (ALT_SHIFT_BIT | ALT_SYMBOL_BIT | ALT_FUNCTION_BIT);
    }

    inline bool isShift() const
    {
        return (_now >> (6 * 8)) & ALT_SHIFT_BIT;
    }
    inline bool isSymbol() const
    {
        return (_now >> (6 * 8)) & ALT_SYMBOL_BIT;
    }
    inline bool isFunction() const
    {
        return (_now >> (6 * 8)) & ALT_FUNCTION_BIT;
    }
    ///@}

    ///@warning API valid only if using UnitUnified firmware
    ///@name Any key
    ///@{
    //! @brief  Is any key press?
    inline bool isPressed() const
    {
        return _now;
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

    ///@warning API valid only if using UnitUnified firmware
    ///@name Specified key (Key index code)
    ///@{
    /*!
      @brief Is the specified key pressed?
      @param kidx Key index code
      @return If so,true
    */
    inline bool isPressed(const key_index_t kidx) const
    {
        return _now & (1ULL << kidx);
    }
    /*!
      @brief Is the specified key released??
      @param kidx Key index code
      @return If so,true
    */
    inline bool isReleased(const key_index_t kidx) const
    {
        return !isPressed(kidx);
    }
    /*!
      @brief Was the specified key pressed?
      @param kidx Key index code
      @return If so,true
    */
    inline bool wasPressed(const key_index_t kidx) const
    {
        return _wasPressed & (1ULL << kidx);
    }
    /*!
      @brief Was the specified key released?
      @param kidx Key index code
      @return If so,true
    */
    inline bool wasReleased(const key_index_t kidx) const
    {
        return _wasReleased & (1ULL << kidx);
    }
    /*!
      @brief Is the specified key holding?
      @param kidx Key index code
      @return If so,true
    */
    inline bool isHolding(const key_index_t kidx) const
    {
        return _holding & (1ULL << kidx);
    }
    /*!
      @brief Was the specified key hold?
      @param kidx Key index code
      @return If so,true
    */
    inline bool wasHold(const key_index_t kidx) const
    {
        return _wasHold & (1ULL << kidx);
    }
    /*!
      @brief Is the specified key repeating?
      @param kidx Key index code
      @return If so,true
    */
    inline bool isRepeating(const key_index_t kidx) const
    {
        return _repeating & (1ULL << kidx);
    }
    ///@}

    /*!
      @brief Character to key index and alt
      @retval Low byte:key_index_t High byte: alt
      @retval No corresponding key index exists if key_index_t is 0xFFFF
     */
    static uint16_t character_to_key_index(const int ch);

    ///@warning API valid only if using UnitUnified firmware
    ///@name Specified Character
    ///@{
    /*!
      @brief Is the specified character pressed?
      @param ch Character
      @return If so,true
    */
    inline bool isPressed(const int ch) const
    {
        return is_character_condition(ch, [this](const key_index_t kidx) { return this->isPressed(kidx); });
    }
    /*!
      @brief Is the specified character released??
      @param ch Character
      @return If so,true
    */
    inline bool isReleased(const int ch) const
    {
        return !isPressed(ch);
    }
    /*!
      @brief Was the specified character pressed?
      @param ch Character
      @return If so,true
    */
    inline bool wasPressed(const int ch) const
    {
        return is_character_condition(ch, [this](const key_index_t kidx) { return this->wasPressed(kidx); });
    }
    /*!
      @brief Was the specified character released?
      @param ch Character
      @return If so,true
    */
    inline bool wasReleased(const int ch) const
    {
        return is_character_condition(ch, [this](const key_index_t kidx) { return this->wasReleased(kidx); });
    }
    /*!
      @brief Is the specified character holding?
      @param ch Character
      @return If so,true
    */
    inline bool isHolding(const int ch) const
    {
        return is_character_condition(ch, [this](const key_index_t kidx) { return this->isHolding(kidx); });
    }
    /*!
      @brief Was the specified character hold?
      @param ch Character
      @return If so,true
    */
    inline bool wasHold(const int ch) const
    {
        return is_character_condition(ch, [this](const key_index_t kidx) { return this->wasHold(kidx); });
    }
    /*!
      @brief Is the specified character repeating?
      @param ch Character
      @return If so,true
    */
    inline bool isRepeating(const int ch) const
    {
        return is_character_condition(ch, [this](const key_index_t kidx) { return this->isRepeating(kidx); });
    }
    ///@}

    ///@warning API valid only if using UnitUnified firmware
    ///@name Get key (Pressed) if updated
    ///@{
    //! @brief Available pressed keys buffer
    inline uint32_t availablePressed() const
    {
        return _pressed->size();
    }
    //! @brief Is the key pressed buffer empty?
    inline bool emptyPressed() const
    {
        return _pressed->empty();
    }
    //! @brief Is the key pressed buffer full?
    inline bool fullPressed() const
    {
        return _pressed->full();
    }
    //! @brief Discard oldest pressed
    inline void discardPressed() const
    {
        _pressed->pop_front();
    }
    //! @brief Discard all pressed
    inline void flushPressed() const
    {
        _pressed->clear();
    }
    //! @brief Get the oldest pressed key
    uint8_t oldestPressed() const
    {
        return !emptyPressed() ? _pressed->front().value() : 0x00;
    }
    //! @brief Get the latest pressed key
    uint8_t latestPressed() const
    {
        return !emptyPressed() ? _pressed->back().value() : 0x00;
    }
    ///@}

    ///@warning API valid only if using UnitUnified firmware
    ///@name Get key (Released) if updated
    ///@{
    //! @brief Available released keys buffer
    inline uint32_t availableReleased() const
    {
        return _released->size();
    }
    //! @brief Is the key released buffer empty?
    inline bool emptyReleased() const
    {
        return _released->empty();
    }
    //! @brief Is the key released buffer full?
    inline bool fullReleased() const
    {
        return _released->full();
    }
    //! @brief Discard oldest released
    inline void discardReleased() const
    {
        _released->pop_front();
    }
    //! @brief Discard all released
    inline void flushReleased() const
    {
        _released->clear();
    }
    //! @brief Get the oldest released key
    uint8_t oldestReleased() const
    {
        return !emptyReleased() ? _released->front().value() : 0x00;
    }
    //! @brief Get the latest released key
    uint8_t latestReleased() const
    {
        return !emptyReleased() ? _released->back().value() : 0x00;
    }
    ///@}

    ///@warning API valid only if using UnitUnified firmware
    ///@name
    ///@{
    /*!
      @brief Gets the firmware version
      @warning Valid after begin
    */
    uint8_t firmwareVersion() const
    {
        return _firmware_version;
    }
    /*!
      @brief Gets the hardware type
      @warning Valid after begin
    */
    uint8_t hardwareType() const
    {
        return _type;
    }
    /*!
      @brief Read the firmware version if firmware is new
      @param[out] ver Version high nibble:Major, low nibble:Minor
      @return True if successful
     */
    bool readFirmwareVersion(uint8_t& ver);
    /*!
      @brief Read the hardware type if firmware is new
      @param[out] htype Hardware type
      @return True if successful
     */
    bool readHardwareType(uint8_t& htype);
    ///@}

    ///@warning API valid only if using UnitUnified firmware
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

    ///@warning API valid only if using UnitUnified firmware
    ///@name Mode
    ///@{
    /*!
      @brief Read the mode
      @param[out] mode Mode
      @return True if successful
     */
    bool readMode(cardkb::Mode& mode);
    /*!
      @brief Read the mode
      @param mode Mode
      @return True if successful
     */
    bool writeMode(const cardkb::Mode mode);
    ///@}

protected:
    bool update_new_firmware(const types::elapsed_time_t at);
    void push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx, const uint8_t alt);

    template <typename Func>
    bool is_character_condition(const int ch, Func func) const
    {
        auto ka     = character_to_key_index(ch);
        uint8_t alt = ka >> 8;
        return (ka != 0xFFFF) ? func((key_index_t)(ka & 0xFF)) && (!alt || ((_now >> (6 * 8)) & alt) == alt) : false;
    }

protected:
    std::unique_ptr<m5::container::CircularBuffer<uint8_t>> _pressed{};
    std::unique_ptr<m5::container::CircularBuffer<uint8_t>> _released{};
    uint64_t _now{}, _prev{}, _wasPressed{}, _wasReleased{}, _wasHold{}, _holding{}, _repeating{};
    types::elapsed_time_t _repeat_start_at[NUMBER_OF_KEYS]{};
    types::elapsed_time_t _hold_start_at[NUMBER_OF_KEYS]{};
    uint32_t _repeating_threshold{}, _holding_threshold{};
    uint8_t _type{}, _firmware_version{};
    cardkb::Mode _mode{cardkb::Mode::Released};
    config_t _cfg{};
};

///@cond
namespace cardkb {
namespace command {
constexpr uint8_t CMD_SCAN_REG{0x10};
constexpr uint8_t CMD_MODE_REG{0x20};
constexpr uint8_t CMD_HARDWARE_TYPE_REG{0xFD};
constexpr uint8_t CMD_FIRMWARE_VERSION_REG{0xFE};
}  // namespace command
}  // namespace cardkb
///@endcond

}  // namespace unit
}  // namespace m5
#endif
