/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
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
  @namespace faces
  @brief For Faces
 */
namespace faces {
///@name Faces type
///@{
constexpr uint8_t TYPE_QWERTY{0x01};  //!< SKU:A003
///@}
}  // namespace faces

/*!
  @class m5::unit::UnitFacesQWERTY
  @brief QWERTY is a full-featured keyboard panel adapted to FACE_BOTTOM
  @warning Note that this can only be detected if the key is released.
*/
class UnitFacesQWERTY : public UnitKeyboardBitwise {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitFacesQWERTY, 0x08);

public:
    static constexpr uint8_t NUMBER_OF_KEYS{35};

    ///@name key index (left top to right bottom)
    ///@{
    static constexpr keyboard::key_index_t KEY_Q{0};
    static constexpr keyboard::key_index_t KEY_W{1};
    static constexpr keyboard::key_index_t KEY_E{2};
    static constexpr keyboard::key_index_t KEY_R{3};
    static constexpr keyboard::key_index_t KEY_T{4};
    static constexpr keyboard::key_index_t KEY_Y{5};
    static constexpr keyboard::key_index_t KEY_U{6};
    static constexpr keyboard::key_index_t KEY_I{7};
    static constexpr keyboard::key_index_t KEY_O{8};
    static constexpr keyboard::key_index_t KEY_P{9};
    static constexpr keyboard::key_index_t KEY_A{10};
    static constexpr keyboard::key_index_t KEY_S{11};
    static constexpr keyboard::key_index_t KEY_D{12};
    static constexpr keyboard::key_index_t KEY_F{13};
    static constexpr keyboard::key_index_t KEY_G{14};
    static constexpr keyboard::key_index_t KEY_H{15};
    static constexpr keyboard::key_index_t KEY_J{16};
    static constexpr keyboard::key_index_t KEY_K{17};
    static constexpr keyboard::key_index_t KEY_L{18};
    static constexpr keyboard::key_index_t KEY_BS{19};
    static constexpr keyboard::key_index_t KEY_NO_KEY_ALT{20};
    static constexpr keyboard::key_index_t KEY_Z{21};
    static constexpr keyboard::key_index_t KEY_X{22};
    static constexpr keyboard::key_index_t KEY_C{23};
    static constexpr keyboard::key_index_t KEY_V{24};
    static constexpr keyboard::key_index_t KEY_B{25};
    static constexpr keyboard::key_index_t KEY_N{26};
    static constexpr keyboard::key_index_t KEY_M{27};
    static constexpr keyboard::key_index_t KEY_DOLLAR{28};
    static constexpr keyboard::key_index_t KEY_ENTER{29};
    static constexpr keyboard::key_index_t KEY_NO_KEY_SHIFT{30};
    static constexpr keyboard::key_index_t KEY_0{31};
    static constexpr keyboard::key_index_t KEY_SPACE{32};
    static constexpr keyboard::key_index_t KEY_NO_KEY_SYM{33};
    static constexpr keyboard::key_index_t KEY_NO_KEY_FN{34};
    ///@}

    ///@name Character code for special keys
    ///@{
    static constexpr char SCHAR_NOMARK_G{static_cast<char>(180)};
    static constexpr char SCHAR_NOMARK_H{static_cast<char>(181)};
    static constexpr char SCHAR_NOMARK_J{static_cast<char>(182)};
    static constexpr char SCHAR_UP{static_cast<char>(183)};
    static constexpr char SCHAR_INS{static_cast<char>(184)};
    static constexpr char SCHAR_HOME{static_cast<char>(187)};
    static constexpr char SCHAR_END{static_cast<char>(188)};
    static constexpr char SCHAR_PAGE_UP{static_cast<char>(189)};
    static constexpr char SCHAR_PAGE_DOWN{static_cast<char>(190)};
    static constexpr char SCHAR_LEFT{static_cast<char>(191)};
    static constexpr char SCHAR_DOWN{static_cast<char>(192)};
    static constexpr char SCHAR_RIGHT{static_cast<char>(193)};
    static constexpr char SCHAR_SPEAKER{static_cast<char>(194)};
    ///@}

    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement on begin?
        bool start_periodic{true};
        //! Triggering an interrupt to update? (interval setting is ignored)
        bool trigger_irq{true};
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
    explicit UnitFacesQWERTY(const uint8_t addr = DEFAULT_ADDRESS) : UnitKeyboardBitwise(addr)
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
      @note 0x01:normal 0x02:shift 0x04:symbol 0x08:function 0x10:alt
     */
    static uint8_t character_to_mode_bits(const char ch);

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
    //! @copydoc m5::unit::UnitKeyboard::released
    //! @note Enter key is returned by 2 bytes of [0x0D, 0X0A] from old firmware, but this class treats it as 0x0D
    uint8_t released() const;
#endif

protected:
    bool update_new_firmware(const types::elapsed_time_t at);
    void push_back(m5::container::CircularBuffer<uint8_t>* container, const uint8_t kidx, const uint8_t mod8);

    inline virtual uint8_t to_mode_bits(const char ch) const override
    {
        return character_to_mode_bits(ch);
    }

protected:
    bool _handle_irq{};
    uint8_t _type{};
    config_t _cfg{};
};

///@cond
namespace faces {
namespace command {
constexpr uint8_t CMD_FACES_TYPE_REG{0xFC};
}  // namespace command
}  // namespace faces
///@endcond
}  // namespace unit
}  // namespace m5
#endif
