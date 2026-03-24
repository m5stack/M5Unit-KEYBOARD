/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB2.hpp
  @brief CardKB2 Unit for M5UnitUnified
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_CARD_KB2_HPP
#define M5_UNIT_KEYBOARD_UNIT_CARD_KB2_HPP

#include "unit_Keyboard.hpp"
#include <m5_utility/container/circular_buffer.hpp>
#include <vector>

namespace m5 {
namespace unit {
/*!
  @class m5::unit::UnitCardKB2
  @brief Card-size 50 key QWERTY keyboard
  @warning Note that older firmware can only detect if the key is released
*/
class UnitCardKB2 : public UnitKeyboardBitwise {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitCardKB2, 0x5F);

public:
    using Packet = std::vector<uint8_t>;

    static constexpr uint8_t NUMBER_OF_KEYS{43};

    ///@name key index (bit position in scan result)
    ///@{
    static constexpr keyboard::key_index_t KEY_1{0};
    static constexpr keyboard::key_index_t KEY_2{1};
    static constexpr keyboard::key_index_t KEY_3{2};
    static constexpr keyboard::key_index_t KEY_4{3};
    static constexpr keyboard::key_index_t KEY_5{4};
    static constexpr keyboard::key_index_t KEY_6{5};
    static constexpr keyboard::key_index_t KEY_7{6};
    static constexpr keyboard::key_index_t KEY_8{7};
    static constexpr keyboard::key_index_t KEY_9{8};
    static constexpr keyboard::key_index_t KEY_0{9};
    static constexpr keyboard::key_index_t KEY_Q{11};
    static constexpr keyboard::key_index_t KEY_W{12};
    static constexpr keyboard::key_index_t KEY_E{13};
    static constexpr keyboard::key_index_t KEY_R{14};
    static constexpr keyboard::key_index_t KEY_T{15};
    static constexpr keyboard::key_index_t KEY_Y{16};
    static constexpr keyboard::key_index_t KEY_U{17};
    static constexpr keyboard::key_index_t KEY_I{18};
    static constexpr keyboard::key_index_t KEY_O{19};
    static constexpr keyboard::key_index_t KEY_P{20};
    static constexpr keyboard::key_index_t KEY_DELETE{21};
    static constexpr keyboard::key_index_t KEY_AA{22};
    static constexpr keyboard::key_index_t KEY_A{23};
    static constexpr keyboard::key_index_t KEY_S{24};
    static constexpr keyboard::key_index_t KEY_D{25};
    static constexpr keyboard::key_index_t KEY_F{26};
    static constexpr keyboard::key_index_t KEY_G{27};
    static constexpr keyboard::key_index_t KEY_H{28};
    static constexpr keyboard::key_index_t KEY_J{29};
    static constexpr keyboard::key_index_t KEY_K{30};
    static constexpr keyboard::key_index_t KEY_L{31};
    static constexpr keyboard::key_index_t KEY_ENTER{32};
    static constexpr keyboard::key_index_t KEY_FN{33};
    static constexpr keyboard::key_index_t KEY_SYM{34};
    static constexpr keyboard::key_index_t KEY_Z{35};
    static constexpr keyboard::key_index_t KEY_X{36};
    static constexpr keyboard::key_index_t KEY_C{37};
    static constexpr keyboard::key_index_t KEY_V{38};
    static constexpr keyboard::key_index_t KEY_B{39};
    static constexpr keyboard::key_index_t KEY_N{40};
    static constexpr keyboard::key_index_t KEY_M{41};
    static constexpr keyboard::key_index_t KEY_SPACE{42};
    ///@}

    ///@name Character code for special keys
    ///@{
    static constexpr char SCHAR_LEFT{(char)180};
    static constexpr char SCHAR_UP{(char)181};
    static constexpr char SCHAR_DOWN{(char)182};
    static constexpr char SCHAR_RIGHT{(char)183};
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

    explicit UnitCardKB2(const uint8_t addr = DEFAULT_ADDRESS) : UnitKeyboardBitwise(addr)
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

    inline virtual keyboard::key_index_t toKeyIndex(const char ch) const override
    {
        return character_to_key_index(ch);
    }

    /*!
      @brief Character to key index
      @retval != 0xFF keyboard::key_index_t
      @retval == 0xFF No corresponding key index exists
     */
    static keyboard::key_index_t character_to_key_index(const char ch);
    /*!
      @brief Character to mode bits
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
    uint8_t read_data(Packet& rbuf);

    inline virtual uint8_t to_mode_bits(const char ch) const override
    {
        return character_to_mode_bits(ch);
    }

protected:
    uint8_t _type{};
    config_t _cfg{};

private:
    bool _sym_was_pressed{false};
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
