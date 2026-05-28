/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB.hpp
  @brief CardKB Unit for M5UnitUnified
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_CARD_KB_HPP
#define M5_UNIT_KEYBOARD_UNIT_CARD_KB_HPP

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
    //! @brief Number of keys in the scan matrix (Shift/Sym/Fn modifiers are tracked separately, not included here)
    static constexpr uint8_t NUMBER_OF_KEYS{48};

    ///@name key index (left top to right bottom)
    ///@{
    static constexpr keyboard::key_index_t KEY_ESC{0};
    static constexpr keyboard::key_index_t KEY_1{1};
    static constexpr keyboard::key_index_t KEY_2{2};
    static constexpr keyboard::key_index_t KEY_3{3};
    static constexpr keyboard::key_index_t KEY_4{4};
    static constexpr keyboard::key_index_t KEY_5{5};
    static constexpr keyboard::key_index_t KEY_6{6};
    static constexpr keyboard::key_index_t KEY_7{7};
    static constexpr keyboard::key_index_t KEY_8{8};
    static constexpr keyboard::key_index_t KEY_9{9};
    static constexpr keyboard::key_index_t KEY_0{10};
    static constexpr keyboard::key_index_t KEY_BS{11};
    static constexpr keyboard::key_index_t KEY_TAB{12};
    static constexpr keyboard::key_index_t KEY_Q{13};
    static constexpr keyboard::key_index_t KEY_W{14};
    static constexpr keyboard::key_index_t KEY_E{15};
    static constexpr keyboard::key_index_t KEY_R{16};
    static constexpr keyboard::key_index_t KEY_T{17};
    static constexpr keyboard::key_index_t KEY_Y{18};
    static constexpr keyboard::key_index_t KEY_U{19};
    static constexpr keyboard::key_index_t KEY_I{20};
    static constexpr keyboard::key_index_t KEY_O{21};
    static constexpr keyboard::key_index_t KEY_P{22};
    static constexpr keyboard::key_index_t KEY_NO_KEY{23};
    static constexpr keyboard::key_index_t KEY_LEFT{24};
    static constexpr keyboard::key_index_t KEY_UP{25};
    static constexpr keyboard::key_index_t KEY_A{26};
    static constexpr keyboard::key_index_t KEY_S{27};
    static constexpr keyboard::key_index_t KEY_D{28};
    static constexpr keyboard::key_index_t KEY_F{29};
    static constexpr keyboard::key_index_t KEY_G{30};
    static constexpr keyboard::key_index_t KEY_H{31};
    static constexpr keyboard::key_index_t KEY_J{32};
    static constexpr keyboard::key_index_t KEY_K{33};
    static constexpr keyboard::key_index_t KEY_L{34};
    static constexpr keyboard::key_index_t KEY_ENTER{35};
    static constexpr keyboard::key_index_t KEY_DOWN{36};
    static constexpr keyboard::key_index_t KEY_RIGHT{37};
    static constexpr keyboard::key_index_t KEY_Z{38};
    static constexpr keyboard::key_index_t KEY_X{39};
    static constexpr keyboard::key_index_t KEY_C{40};
    static constexpr keyboard::key_index_t KEY_V{41};
    static constexpr keyboard::key_index_t KEY_B{42};
    static constexpr keyboard::key_index_t KEY_N{43};
    static constexpr keyboard::key_index_t KEY_M{44};
    static constexpr keyboard::key_index_t KEY_COMMA{45};
    static constexpr keyboard::key_index_t KEY_PERIOD{46};
    static constexpr keyboard::key_index_t KEY_SPACE{47};
    ///@}

    ///@name Character code for special keys
    ///@{
    static constexpr char SCHAR_LEFT{static_cast<char>(180)};
    static constexpr char SCHAR_UP{static_cast<char>(181)};
    static constexpr char SCHAR_DOWN{static_cast<char>(182)};
    static constexpr char SCHAR_RIGHT{static_cast<char>(183)};
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
        keyboard::Mode mode{keyboard::Mode::Conventional};
        //! Periodic interval
        uint32_t interval{10};
        //! Threshold for key repeating (ms)
        uint32_t repeating_threshold{400};
        //! Threshold for key holding (ms)
        uint32_t holding_threshold{800};
        ///@}
    };

    /*!
      @brief Constructor
      @param addr I2C address (default: DEFAULT_ADDRESS)
     */
    explicit UnitCardKB(const uint8_t addr = DEFAULT_ADDRESS) : UnitKeyboardBitwise(addr)
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
        return character_to_key_index(ch);
    }

    /*!
      @brief Character to key index
      @param ch ASCII character
      @retval != 0xFF keyboard::key_index_t
      @retval == 0xFF No corresponding key index exists
     */
    static keyboard::key_index_t character_to_key_index(const char ch);
    /*!
      @brief Character to mode bits
      @param ch ASCII character
      @retval == 0 Not exists
      @retval != 0 Bits in corresponding mode
      @note 0x01:normal 0x02:shift 0x04:symbol 0x08:function
     */
    static uint8_t character_to_mode_bits(const char ch);

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
    void push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx, const uint8_t mod8);

    inline virtual uint8_t to_mode_bits(const char ch) const override
    {
        return character_to_mode_bits(ch);
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
