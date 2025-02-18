/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_FacesQWERTY.hpp
  @brief Faces QWERTY Unit for M5UnitUnified
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_FACES_QWERTY_HPP
#define M5_UNIT_KEYBOARD_UNIT_FACES_QWERTY_HPP

#include "unit_Keyboard.hpp"

namespace m5 {
namespace unit {

/*!
  @class m5::unit::UnitFacesQWERTY
  @brief QWERTY is a full-featured keyboard panel adapted to FACE_BOTTOM
  @warning Note that this can only be detected if the key is released.
*/
class UnitFacesQWERTY : public UnitKeyboardBitwise {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitFacesQWERTY, 0x08);

public:
    static constexpr uint8_t NUMBER_OF_KEYS{35};

    ///@name key index
    ///@{
    static constexpr key_index_t KEY_Q{0};
    static constexpr key_index_t KEY_W{1};
    static constexpr key_index_t KEY_E{2};
    static constexpr key_index_t KEY_R{3};
    static constexpr key_index_t KEY_T{4};
    static constexpr key_index_t KEY_Y{5};
    static constexpr key_index_t KEY_U{6};
    static constexpr key_index_t KEY_I{7};
    static constexpr key_index_t KEY_O{8};
    static constexpr key_index_t KEY_P{9};
    static constexpr key_index_t KEY_A{10};
    static constexpr key_index_t KEY_S{11};
    static constexpr key_index_t KEY_D{12};
    static constexpr key_index_t KEY_F{13};
    static constexpr key_index_t KEY_G{14};
    static constexpr key_index_t KEY_H{15};
    static constexpr key_index_t KEY_J{16};
    static constexpr key_index_t KEY_K{17};
    static constexpr key_index_t KEY_L{18};
    static constexpr key_index_t KEY_DEL{19};
    static constexpr key_index_t KEY_NO_KEY_ALT{20};
    static constexpr key_index_t KEY_Z{21};
    static constexpr key_index_t KEY_X{22};
    static constexpr key_index_t KEY_C{23};
    static constexpr key_index_t KEY_V{24};
    static constexpr key_index_t KEY_B{25};
    static constexpr key_index_t KEY_N{26};
    static constexpr key_index_t KEY_M{27};
    static constexpr key_index_t KEY_DOLLAR{28};
    static constexpr key_index_t KEY_ENTER{29};
    static constexpr key_index_t KEY_NO_KEY_SHIFT{30};
    static constexpr key_index_t KEY_0{31};
    static constexpr key_index_t KEY_SPACE{32};
    static constexpr key_index_t KEY_NO_KEY_SYM{33};
    static constexpr key_index_t KEY_NO_KEY_FN{34};
    ///@}

    ///@name Modifier key bit
    ///@{
    static constexpr uint64_t MODIFIER_SHIFT_64BIT{0x200000000000};     //!< Shift
    static constexpr uint64_t MODIFIER_SYMBOL_64BIT{0x800000000000};    //!< Symbol
    static constexpr uint64_t MODIFIER_FUNCTION_64BIT{0x080000000000};  //!< Function
    static constexpr uint64_t MODIFIER_ALT_64BIT{0x100000000000};       //!< Alt
    /*
    static constexpr uint8_t MODIFIER_SHIFT_8BIT{0x10};                   //!< Shift
    static constexpr uint8_t MODIFIER_SYMBOL_8BIT{0x80};                  //!< Symbol
    static constexpr uint8_t MODIFIER_FUNCTION_8BIT{0x40};                //!< Function
    static constexpr uint8_t MODIFIER_ALT_8BIT{0x40};                //!< Function
    */
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

    explicit UnitFacesQWERTY(const uint8_t addr = DEFAULT_ADDRESS) : UnitKeyboardBitwise(addr)
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
        return _now & (MODIFIER_SHIFT_64BIT | MODIFIER_SYMBOL_64BIT | MODIFIER_FUNCTION_64BIT | MODIFIER_ALT_64BIT);
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
    inline virtual bool isAlt() const override
    {
        return _now & MODIFIER_ALT_64BIT;
    }

    inline virtual char getchar() const override {
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
    ///@name Faces type
    ///@{
    /*!
      @brief Gets the Faces type
      @warning Valid after begin
    */
    uint8_t facesType() const
    {
        return _type;
    }
    /*!
      @brief Read the faces type
      @param[out] ftype Hardware type
      @return True if successful
     */
    bool readFacesType(uint8_t& ftype);
    ///@}

#if defined(DOXYGEN_PROCESS)
    //! @copydoc m5::unit::Keyboard::released
    //! @note Enter key is returned by 2 bytes of [0x0D, 0X0A] from old firmware, but this class treats it as 0x0D
    uint8_t released() const;
#endif

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
        return 0x00;
    }
    inline virtual bool equal_modifier(const uint8_t mbit) const override
    {
        return mbit ? ((mbit & modifier_bits()) == mbit) : (modifier_bits() == 0x00);
    }

protected:
    uint8_t _type{};
    config_t _cfg{};
};

/*!
  @namespacde faces
  @brief For Faces
 */
namespace faces {
///@name Faces type
///@{
constexpr uint8_t TYPE_QWERTY{0x01};  //!< SKU:A003
///@}
///@cond
namespace command {
constexpr uint8_t CMD_FACES_TYPE_REG{0xFC};
}  // namespace command
///@endcond
}  // namespace faces

}  // namespace unit
}  // namespace m5
#endif
