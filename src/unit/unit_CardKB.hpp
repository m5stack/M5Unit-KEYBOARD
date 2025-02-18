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
///@name Hardware type
///@{
constexpr uint8_t TYPE_CARDKB{0x01};      //!< SKU:U035
constexpr uint8_t TYPE_CARDKB_V11{0x11};  //!< V11 SKU:U035-B
///@}
}  // namespace cardkb

/*!
  @class m5::unit::UnitCardKB
  @brief Card-size 50 key QWERTY keyboard
  @warning Note that older firmware can only detect if the key is released
*/
class UnitCardKB : public UnitKeyboardBitwise {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitCardKB, 0x5F);

public:
    static constexpr uint8_t NUMBER_OF_KEYS{48};

    ///@name key index
    ///@{
    static constexpr key_index_t KEY_ESC{0};
    static constexpr key_index_t KEY_1{1};
    static constexpr key_index_t KEY_2{2};
    static constexpr key_index_t KEY_3{3};
    static constexpr key_index_t KEY_4{4};
    static constexpr key_index_t KEY_5{5};
    static constexpr key_index_t KEY_6{6};
    static constexpr key_index_t KEY_7{7};
    static constexpr key_index_t KEY_8{8};
    static constexpr key_index_t KEY_9{9};
    static constexpr key_index_t KEY_0{10};
    static constexpr key_index_t KEY_DEL{11};
    static constexpr key_index_t KEY_TAB{12};
    static constexpr key_index_t KEY_Q{13};
    static constexpr key_index_t KEY_W{14};
    static constexpr key_index_t KEY_E{15};
    static constexpr key_index_t KEY_R{16};
    static constexpr key_index_t KEY_T{17};
    static constexpr key_index_t KEY_Y{18};
    static constexpr key_index_t KEY_U{19};
    static constexpr key_index_t KEY_I{20};
    static constexpr key_index_t KEY_O{21};
    static constexpr key_index_t KEY_P{22};
    static constexpr key_index_t KEY_NO_KEY{23};
    static constexpr key_index_t KEY_LEFT{24};
    static constexpr key_index_t KEY_UP{25};
    static constexpr key_index_t KEY_A{26};
    static constexpr key_index_t KEY_S{27};
    static constexpr key_index_t KEY_D{28};
    static constexpr key_index_t KEY_F{29};
    static constexpr key_index_t KEY_G{30};
    static constexpr key_index_t KEY_H{31};
    static constexpr key_index_t KEY_J{32};
    static constexpr key_index_t KEY_K{33};
    static constexpr key_index_t KEY_L{34};
    static constexpr key_index_t KEY_ENTER{35};
    static constexpr key_index_t KEY_DOWN{36};
    static constexpr key_index_t KEY_RIGHT{37};
    static constexpr key_index_t KEY_Z{38};
    static constexpr key_index_t KEY_X{39};
    static constexpr key_index_t KEY_C{40};
    static constexpr key_index_t KEY_V{41};
    static constexpr key_index_t KEY_B{42};
    static constexpr key_index_t KEY_N{43};
    static constexpr key_index_t KEY_M{44};
    static constexpr key_index_t KEY_COMMA{45};
    static constexpr key_index_t KEY_PERIOD{46};
    static constexpr key_index_t KEY_SPACE{47};
    ///@}

    ///@name Modifier key bit
    ///@{
    static constexpr uint64_t MODIFIER_SHIFT_64BIT{0x10000000000000};     //!< Shift
    static constexpr uint64_t MODIFIER_SYMBOL_64BIT{0x80000000000000};    //!< Symbol
    static constexpr uint64_t MODIFIER_FUNCTION_64BIT{0x40000000000000};  //!< Function
    static constexpr uint8_t MODIFIER_SHIFT_8BIT{0x10};                   //!< Shift
    static constexpr uint8_t MODIFIER_SYMBOL_8BIT{0x80};                  //!< Symbol
    static constexpr uint8_t MODIFIER_FUNCTION_8BIT{0x40};                //!< Function
    ///@}

    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        ///@name For M5Unit-KEYBOARD firmware
        ///@{
        /*! Mode */
        keyboard::Mode mode{keyboard::Mode::Released};
        //! How many simultaneous inputs to stored
        uint32_t stored_keys{1};
        //! Periodic interval
        uint32_t interval{10};
        //! Threshold for key repeating (ms)
        uint32_t repeating_threshold{400};
        //! Threshold for key holding (ms)
        uint32_t holding_threshold{800};
        ///@}
    };

    explicit UnitCardKB(const uint8_t addr = DEFAULT_ADDRESS) : UnitKeyboardBitwise(addr)
    {
        _repeat_start_at.resize(NUMBER_OF_KEYS);
        _hold_start_at.resize(NUMBER_OF_KEYS);
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

    inline virtual uint64_t modifierBits() const
    {
        return _now & (MODIFIER_SHIFT_64BIT | MODIFIER_SYMBOL_64BIT | MODIFIER_FUNCTION_64BIT);
    }
    inline virtual bool isShift() const override
    {
        return _now & MODIFIER_SHIFT_64BIT;
    }
    inline virtual bool isSymbol() const override
    {
        return _now & MODIFIER_SYMBOL_64BIT;
    }
    inline virtual bool isFunction() const override
    {
        return _now & MODIFIER_FUNCTION_64BIT;
    }
    inline virtual char getchar() const override
    {
        return (_mode == keyboard::Mode::Scan) ? pressed() : released();
    }

    /*!
      @brief Character to key index
      @retval != 0xFF key_index_t
      @retval == 0xFF No corresponding key index exists
     */
    static key_index_t character_to_key_index(const char ch);
    /*!
      @brief Character to modifier bit
      @return Corresponding modifier key bit
     */
    static uint8_t character_to_modifier_bit(const char ch);

    ///@warning API valid only if using M5Unit-KEYBOARD firmware
    ///@name Hardware type
    ///@{
    /*!
      @brief Gets the hardware type
      @warning Valid after begin
    */
    uint8_t hardwareType() const
    {
        return _type;
    }
    /*!
      @brief Read the hardware type
      @param[out] htype Hardware type
      @return True if successful
     */
    bool readHardwareType(uint8_t& htype);
    ///@}

protected:
    bool update_new_firmware(const types::elapsed_time_t at);
    void push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx, const uint8_t mod);

    inline virtual key_index_t _character_to_key_index(const char ch) const override
    {
        return character_to_key_index(ch);
    }
    inline virtual uint8_t _character_to_modifier_bit(const char ch) const override
    {
        return character_to_modifier_bit(ch);
    }
    inline virtual uint8_t modifier_bits() const override
    {
        return (_now >> (6 * 8)) & 0xF0;
    }
    inline virtual bool equal_modifier(const uint8_t mbit) const override
    {
        return mbit ? ((mbit & modifier_bits()) == mbit) : (modifier_bits() == 0x00);
    }

protected:
    uint8_t _type{};
    config_t _cfg{};
};

///@cond
namespace cardkb {
namespace command {
constexpr uint8_t CMD_HARDWARE_TYPE_REG{0xFD};
}  // namespace command
}  // namespace cardkb
///@endcond

}  // namespace unit
}  // namespace m5
#endif
