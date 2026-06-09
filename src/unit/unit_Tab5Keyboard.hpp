/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_Tab5Keyboard.hpp
  @brief Tab5 Keyboard Unit for M5UnitUnified
*/
#ifndef M5_UNIT_KEYBOARD_UNIT_TAB5_KEYBOARD_HPP
#define M5_UNIT_KEYBOARD_UNIT_TAB5_KEYBOARD_HPP

#include "unit_Keyboard.hpp"

// Forward declaration for test access to protected members
class TestTab5Keyboard;

#include <array>
#include <bitset>
#include <memory>  // for std::unique_ptr
#include <string>

#include "../utility/bitwise_state.hpp"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
// isr_handler() is declared IRAM_ATTR; that macro is defined in <esp_attr.h> (esp_common),
// which ships in every ESP-IDF 4.x/5.x and Arduino-ESP32 core. Including only esp_attr.h keeps
// this public header free of the GPIO driver, so `driver` stays a .cpp-only PRIV_REQUIRES.
#include <esp_attr.h>
#endif

namespace m5 {
namespace unit {

/*!
  @namespace tab5_keyboard
  @brief Constants and types for Tab5 Keyboard
 */
namespace tab5_keyboard {

///@cond
namespace command {
constexpr uint8_t REG_INT_CFG{0x00};   // INT_CFG   (R/W) Interrupt enable: [2]=char [1]=HID [0]=normal, default 0x07
constexpr uint8_t REG_INT_STAT{0x01};  // INT_STAT  (R/W) Interrupt status: same bit layout; write 0 to clear
constexpr uint8_t REG_EVENT_NUM{
    0x02};  // EVENT_NUM (R/W) Queue length 0-32; auto-decrements on read; write 0 clears queue + INT
constexpr uint8_t REG_BRIGHTNESS{0x03};     // Brightness (R/W) RGB global brightness 0-100, default 20
constexpr uint8_t REG_MODE_KEYBOARD{0x10};  // Keyboard mode (R/W) 0=Normal 1=HID 2=Character, default 0
constexpr uint8_t REG_MODE_RGB{0x11};       // RGB mode      (R/W) 0=Bind  1=Custom,           default 0
constexpr uint8_t REG_KEY_EVENT{0x20};      // KEY_EVENT         (R) Normal mode: 1 byte
constexpr uint8_t REG_HID_EVENT{0x30};      // HID_EVENT         (R) HID mode: Modifier(0x30) + Key_code(0x31), 2 bytes
constexpr uint8_t REG_CHAR_EVENT_LENGTH{0x40};  // CHAR_EVENT_LENGTH (R) Char mode: queue-head string length
constexpr uint8_t REG_CHAR_EVENT{0x50};  // CHAR_EVENT        (R) Char mode: Modifier(0x50) + char0(0x51)..char8(0x59)
constexpr uint8_t REG_RGB1_B{0x60};      // RGB1 Blue  component (R/W) 0-255, default 0
constexpr uint8_t REG_RGB1_G{0x61};      // RGB1 Green component (R/W) 0-255, default 0
constexpr uint8_t REG_RGB1_R{0x62};      // RGB1 Red   component (R/W) 0-255, default 0
constexpr uint8_t REG_RGB2_B{0x64};      // RGB2 Blue  component (R/W) 0-255, default 0
constexpr uint8_t REG_RGB2_G{0x65};      // RGB2 Green component (R/W) 0-255, default 0
constexpr uint8_t REG_RGB2_R{0x66};      // RGB2 Red   component (R/W) 0-255, default 0
constexpr uint8_t REG_FIRMWARE_VERSION{0xFE};  // Firmware Version (R) Software version byte
constexpr uint8_t REG_I2C_ADDRESS{0xFF};       // I2C Address (R/W) 0x08-0x77, default 0x6D
}  // namespace command
///@endcond

//! @brief Sentinel value returned when the Normal-mode event queue is empty
//! @note Normal mode: empty = 0xFF (1 byte). HID mode: empty = 0xFF 0xFF (2 bytes).
//!       Char mode: empty when REG_CHAR_EVENT_LENGTH (0x40) reads 0.
constexpr uint8_t KEY_EVENT_EMPTY{0xFF};

//! @brief Maximum character payload length in one CHAR_EVENT (char0..char8)
constexpr uint8_t CHAR_EVENT_MAX_CHARS{9};

//! @brief Milliseconds to wait after writing I2C address to REG_I2C_ADDRESS
//! (datasheet: Flash erase ~20 ms; use 50 ms margin for hardware/firmware variance)
constexpr uint32_t I2C_ADDRESS_WRITE_DELAY_MS{50};

//! @brief Number of RGB LEDs on the device
constexpr uint8_t RGB_LED_COUNT{2};

///@name Modifier key positions in Normal mode (5x14 matrix)
///@brief Hardware-confirmed (row, col) of the four modifier keys.
///       In Normal mode these arrive as ordinary Key events; the library
///       excludes them from software auto-repeat via isModifierKey().
///@{
constexpr uint8_t MODIFIER_KEY_ROW_SYM{3};   //!< Sym key row
constexpr uint8_t MODIFIER_KEY_COL_SYM{0};   //!< Sym key column
constexpr uint8_t MODIFIER_KEY_ROW_AA{3};    //!< Aa key row
constexpr uint8_t MODIFIER_KEY_COL_AA{1};    //!< Aa key column
constexpr uint8_t MODIFIER_KEY_ROW_CTRL{4};  //!< Ctrl key row
constexpr uint8_t MODIFIER_KEY_COL_CTRL{0};  //!< Ctrl key column
constexpr uint8_t MODIFIER_KEY_ROW_ALT{4};   //!< Alt key row
constexpr uint8_t MODIFIER_KEY_COL_ALT{1};   //!< Alt key column
///@}

/*!
  @brief True if the given (row, col) corresponds to a modifier key (Sym/Aa/Ctrl/Alt)
  @param row Matrix row (0-4)
  @param col Matrix column (0-13)
  @return True if the position is a modifier key (Sym/Aa/Ctrl/Alt)
  @note Used by software auto-repeat to skip modifier keys (holding Sym must not
        spawn repeat events). User code can also call this to filter events.
 */
inline bool isModifierKey(const uint8_t row, const uint8_t col)
{
    return (row == MODIFIER_KEY_ROW_SYM && col == MODIFIER_KEY_COL_SYM) ||
           (row == MODIFIER_KEY_ROW_AA && col == MODIFIER_KEY_COL_AA) ||
           (row == MODIFIER_KEY_ROW_CTRL && col == MODIFIER_KEY_COL_CTRL) ||
           (row == MODIFIER_KEY_ROW_ALT && col == MODIFIER_KEY_COL_ALT);
}

//! @brief Total number of keys in the Tab5 Keyboard matrix (5 rows x 14 cols)
constexpr uint8_t KEY_COUNT{70};

//! @brief Number of columns in the Tab5 Keyboard matrix
constexpr uint8_t KEY_COL_COUNT{14};

//! @brief Bitset type for tracking per-key state (Normal mode)
using key_status_bits_t = std::bitset<KEY_COUNT>;

/*!
  @brief Convert (row, col) to flat key index (0..KEY_COUNT-1)
  @param row Matrix row (0..4)
  @param col Matrix column (0..13)
  @return Flat key index = row * 14 + col
  @note No bounds checking — caller is responsible for valid (row, col).
 */
inline uint8_t toKeyIndex(const uint8_t row, const uint8_t col)
{
    return static_cast<uint8_t>(row * KEY_COL_COUNT + col);
}

///@name Pre-computed key indices for the modifier keys
///@{
constexpr uint8_t KIDX_SYM  = MODIFIER_KEY_ROW_SYM * KEY_COL_COUNT + MODIFIER_KEY_COL_SYM;    //!< 42 (Sym)
constexpr uint8_t KIDX_AA   = MODIFIER_KEY_ROW_AA * KEY_COL_COUNT + MODIFIER_KEY_COL_AA;      //!< 43 (Aa)
constexpr uint8_t KIDX_CTRL = MODIFIER_KEY_ROW_CTRL * KEY_COL_COUNT + MODIFIER_KEY_COL_CTRL;  //!< 56 (Ctrl)
constexpr uint8_t KIDX_ALT  = MODIFIER_KEY_ROW_ALT * KEY_COL_COUNT + MODIFIER_KEY_COL_ALT;    //!< 57 (Alt)
///@}

/*!
  @struct HidMapping
  @brief HID Usage Code + base modifier byte for a Tab5 Keyboard matrix position
  @note Returned by keyMatrixToHidBase() / keyMatrixToHidSym() lookup tables. The
        modifier field captures the firmware-forced shift state (e.g. 0x02 for
        the `+` key which prints as Shift-`=` in HID). Callers may OR in extra
        modifier bits (Aa shift, Ctrl, Alt) before passing to hidUsageToChar().
 */
struct HidMapping {
    uint8_t keycode;   //!< HID Usage Code (USB HID Keyboard Page 0x07); 0 = unmapped
    uint8_t modifier;  //!< Base HID modifier byte (e.g. 0x02 if firmware forces shift)
};

/*!
  @brief Base (no Sym held) HID mapping for the given (row, col)
  @param row Matrix row (0..4)
  @param col Matrix column (0..13)
  @return HidMapping for the key. Returns @c {0, 0} for out-of-range coordinates
          and for the four modifier keys (Sym/Aa/Ctrl/Alt).
  @note US ANSI layout, derived from Tab5 firmware HID-mode logs.
 */
HidMapping keyMatrixToHidBase(const uint8_t row, const uint8_t col);

/*!
  @brief Sym-held variant of the HID mapping for the given (row, col)
  @param row Matrix row (0..4)
  @param col Matrix column (0..13)
  @return HidMapping that reflects the Sym-layer character. Keys whose Sym layer
          is identical to the base layer return the same value as
          keyMatrixToHidBase(). Modifier keys and out-of-range coordinates
          return @c {0, 0}.
 */
HidMapping keyMatrixToHidSym(const uint8_t row, const uint8_t col);

/*!
  @enum EventType
  @brief Discriminator for tab5_keyboard::Event tagged union
 */
enum class EventType : uint8_t {
    None      = 0,  //!< No event (queue empty sentinel)
    Key       = 1,  //!< Normal mode: matrix coordinate event
    Hid       = 2,  //!< HID mode: modifier + keycode
    Character = 3,  //!< Character mode: modifier + UTF-8 string (<=9 bytes)
};

/*!
  @struct Event
  @brief Unified event payload for all 3 operation modes (tagged union, POD)
  @note Always check `type` before accessing union members.
 */
struct Event {
    EventType type{EventType::None};
    uint8_t modifier{};  //!< Shared by HID + Character (unused for Key)
    //! @brief True when this event was synthesized by software auto-repeat.
    //! @note Only ever true in Normal mode and only when config_t::software_repeat is enabled.
    //!       Hardware-originated events (the initial press as well as the release) always have
    //!       repeat == false.
    bool repeat{false};

    union {
        struct {
            uint8_t length;                        //!< Number of valid bytes in `chars` (0..CHAR_EVENT_MAX_CHARS)
            char chars[CHAR_EVENT_MAX_CHARS + 1];  //!< Always NUL-terminated; printf("%s") safe
        } chr{};                                   //!< Character mode payload
        struct {
            uint8_t row;
            uint8_t col;
            bool pressed;
        } key;  //!< Normal mode payload
        struct {
            uint8_t keycode;
        } hid;  //!< HID mode payload
    };

    /*!
      @brief True if the Ctrl modifier is held (modifier bit 0x01 or 0x10)
      @note Applies to both HID and Character modes (firmware uses bit 0 in both).
     */
    inline bool isCtrl() const
    {
        return (modifier & 0x11U) != 0U;
    }
    /*!
      @brief True if a Shift modifier is held (modifier bit 0x02 or 0x20)
      @note HID mode only. In Character mode, Sym/Aa shifting is applied at firmware
            level and the modifier byte is 0x00 for those events.
     */
    inline bool isShift() const
    {
        return (modifier & 0x22U) != 0U;
    }
    /*!
      @brief True if the Alt modifier is held (modifier bit 0x04 or 0x40)
      @note Applies to both HID and Character modes (firmware uses bit 2 in both).
     */
    inline bool isAlt() const
    {
        return (modifier & 0x44U) != 0U;
    }
};

//! @brief Default internal CircularBuffer<Event> capacity (~256 bytes total)
constexpr uint32_t DEFAULT_STORED_SIZE{16};

/*!
  @enum Mode
  @brief Keyboard operation mode (REG_MODE_KEYBOARD 0x10)
 */
enum class Mode : uint8_t {
    Normal    = 0,  //!< Normal mode: returns matrix coordinate events (5 rows x 14 cols)
    HID       = 1,  //!< HID mode: returns HID modifier + keycode
    Character = 2,  //!< Character mode: returns modifier + UTF-8 string
};

/*!
  @enum RgbMode
  @brief RGB LED operation mode (REG_MODE_RGB 0x11)
 */
enum class RgbMode : uint8_t {
    //! Bind mode: LED color is driven by firmware. Observed behavior on real hardware:
    //!   - Right LED (RGB2) indicates the current keyboard mode:
    //!       Blue  = Mode::Normal
    //!       Green = Mode::HID
    //!       Purple= Mode::Character
    //!   - Left LED (RGB1) indicates run-time state (modifier / activity).
    //! Default after power-on (REG_MODE_RGB default = 0).
    Bind = 0,
    //! Custom mode: LED color is set via REG_RGB1_B/G/R and REG_RGB2_B/G/R.
    //! Use writeRgb()/readRgb() to control. writeRgbMode(Custom) must be called first.
    Custom = 1,
};

namespace command {
constexpr uint8_t CMD_INT_CFG{REG_INT_CFG};
constexpr uint8_t CMD_INT_STAT{REG_INT_STAT};
constexpr uint8_t CMD_EVENT_NUM{REG_EVENT_NUM};
constexpr uint8_t CMD_BRIGHTNESS{REG_BRIGHTNESS};
constexpr uint8_t CMD_MODE_KEYBOARD{REG_MODE_KEYBOARD};
constexpr uint8_t CMD_MODE_RGB{REG_MODE_RGB};
constexpr uint8_t CMD_KEY_EVENT{REG_KEY_EVENT};
constexpr uint8_t CMD_HID_EVENT{REG_HID_EVENT};
constexpr uint8_t CMD_CHAR_EVENT_LENGTH{REG_CHAR_EVENT_LENGTH};
constexpr uint8_t CMD_CHAR_EVENT{REG_CHAR_EVENT};
constexpr uint8_t CMD_RGB1_B{REG_RGB1_B};
constexpr uint8_t CMD_FIRMWARE_VERSION{REG_FIRMWARE_VERSION};
constexpr uint8_t CMD_I2C_ADDRESS{REG_I2C_ADDRESS};
}  // namespace command

}  // namespace tab5_keyboard

/*!
  @class m5::unit::UnitTab5Keyboard
  @brief Tab5 built-in keyboard unit connected via I2C ExtPort1

  The Tab5 Keyboard provides a 5-row x 14-column key matrix with three operation
  modes: Normal (coordinate), HID (modifier+keycode), and Character (modifier+string).
  Two RGB indicator LEDs and an active-low INT pin are also provided.

  INT pin usage is optional. When configured via config_t::irq_pin, begin() installs an
  ESP-IDF GPIO ISR (NEGEDGE) that sets _irq_pending. update() reads events when
  _irq_pending is true or gpio_get_level() == 0 (level still asserted). Without a
  configured pin, update() polls unconditionally.

  @note Modifiers: Sym / Aa / Ctrl / Alt (no Fn key).
  @note Default I2C address 0x6D (range 0x08-0x77, datasheet-configurable via REG_I2C_ADDRESS).
  @note ExtPort1 pin assignment: INT=GPIO50, SDA=GPIO0, SCL=GPIO1.
  @note Multiple UnitTab5Keyboard instances share the same gpio_install_isr_service();
        already-installed state is treated as OK (ESP_ERR_INVALID_STATE accepted).
*/
class UnitTab5Keyboard : public UnitKeyboard,
                         public PeriodicMeasurementAdapter<UnitTab5Keyboard, tab5_keyboard::Event> {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitTab5Keyboard, 0x6D);
    M5_UNIT_COMPONENT_PERIODIC_MEASUREMENT_ADAPTER_HPP_BUILDER(UnitTab5Keyboard, tab5_keyboard::Event);
    friend class ::TestTab5Keyboard;

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! Start periodic measurement during begin() (mirrors the other M5Unit keyboards).
        //! When true, begin() calls startPeriodicMeasurement(), which enables the INT for the
        //! active mode (only if irq_pin >= 0) and begins draining events. Set false to defer and
        //! call startPeriodicMeasurement() manually later.
        bool start_periodic{true};
        //! Initial keyboard operation mode applied during begin() (REG_MODE_KEYBOARD 0x10).
        //! Default is Mode::Normal (matrix coordinate events).
        tab5_keyboard::Mode mode{tab5_keyboard::Mode::Normal};
        //! GPIO pin number connected to the active-low INT signal.
        //! Default is 50 (Tab5 ExtPort1 J9 pin 10, confirmed via M5Tab5-UserDemo BSP).
        //! Set to a valid GPIO number (0..GPIO_NUM_MAX-1) to enable ISR-driven event polling.
        //! Set to -1 to disable INT-driven mode and use unconditional polling.
        int8_t irq_pin{50};
        //! Polling interval in milliseconds for non-interrupt-driven operation.
        //! @note Used only when update() is NOT INT-gated (irq_pin < 0, INT disabled, software_repeat,
        //!       or a forced update). In that mode the device event queue is drained at most once per
        //!       interval_ms. When INT-driven, draining is event-driven and this value is unused.
        uint32_t interval_ms{50};
        //! Enable software auto-repeat (Normal mode only).
        //! @note Effective only when @c mode == tab5_keyboard::Mode::Normal. In HID and Character
        //!       modes this flag is ignored because repeat handling is owned by the device firmware
        //!       (or there is no row/col context to repeat). The repeat state is automatically
        //!       cleared by a release event and by writeMode().
        bool software_repeat{false};
        //! Initial delay before the first repeat event is emitted (milliseconds).
        //! @note Measured from the press time recorded for the currently held key.
        //! @note Also used as the bitwise "repeating" threshold (see isRepeating()).
        uint32_t repeat_initial_ms{400};
        //! Interval between subsequent repeat events (milliseconds).
        uint32_t repeat_rate_ms{80};
        //! Hold detection threshold (milliseconds).
        //! @note Used by the bitwise tracker for isHolding() / wasHold().
        //!       Normal mode only; HID and Character modes do not track bitwise state.
        uint32_t holding_threshold_ms{800};
    };

    explicit UnitTab5Keyboard(const uint8_t addr = DEFAULT_ADDRESS)
        : UnitKeyboard(addr),
          _data{new m5::container::CircularBuffer<tab5_keyboard::Event>(tab5_keyboard::DEFAULT_STORED_SIZE)}
    {
        auto ccfg        = component_config();
        ccfg.clock       = 400 * 1000U;  // Tab5 keyboard supports I2C Fast mode (400 kHz).
        ccfg.stored_size = tab5_keyboard::DEFAULT_STORED_SIZE;
        component_config(ccfg);
    }
    virtual ~UnitTab5Keyboard();

    ///@name Settings for begin
    ///@{
    //! @brief Gets the configuration
    inline config_t config() const
    {
        return _cfg;
    }
    //! @brief Sets the configuration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    //! @copydoc Component::begin
    virtual bool begin() override;
    //! @copydoc Component::update
    virtual void update(const bool force = false) override;

    ///@name Interrupt enable / status
    ///@{
    /*!
      @brief Configure per-mode interrupt enable (REG_INT_CFG 0x00)
      @param normal  Enable INT for Normal mode events    (bit 0)
      @param hid     Enable INT for HID mode events       (bit 1)
      @param character Enable INT for Character mode events (bit 2)
      @return True if successful
      @note Default register value is 0x07 (all modes enabled).
     */
    bool writeInterruptEnable(const bool normal, const bool hid, const bool character);

    /*!
      @brief Read per-mode interrupt enable bits from REG_INT_CFG (0x00)
      @param[out] normal True if Normal mode INT enabled (bit 0)
      @param[out] hid True if HID mode INT enabled (bit 1)
      @param[out] character True if Character mode INT enabled (bit 2)
      @return True if I2C read succeeded
     */
    bool readInterruptEnable(bool& normal, bool& hid, bool& character);

    /*!
      @brief Read per-mode interrupt pending status from REG_INT_STAT (0x01)
      @param[out] normal True if a Normal mode interrupt is pending (bit 0)
      @param[out] hid True if a HID mode interrupt is pending (bit 1)
      @param[out] character True if a Character mode interrupt is pending (bit 2)
      @return True if I2C read succeeded
      @note REG_INT_STAT (pending) is distinct from REG_INT_CFG (enable, see readInterruptEnable()).
     */
    bool readInterruptStatus(bool& normal, bool& hid, bool& character);

    /*!
      @brief Clear all interrupt status bits in REG_INT_STAT (0x01) by writing 0x00
      @return True if successful
      @note Datasheet: writing 0 releases the INT signal and clears the status.
            This call clears all pending modes (Normal, HID, Character) in a single write.
     */
    bool clearInterrupt();

    /*!
      @brief Clear the current-mode event queue and release INT signal
      @details Writes 0 to REG_EVENT_NUM (0x02). Per datasheet: "writing 0 clears
               current mode queue and releases interrupt signal". Internal
               CircularBuffer is also flushed for consistency.
      @return True if I2C write succeeded
     */
    bool clearEventQueue();
    ///@}

    ///@name Firmware / device info
    ///@{
    /*!
      @brief Read the firmware version from REG_FIRMWARE_VERSION (0xFE)
      @param[out] ver Firmware version byte
      @return True if successful
     */
    bool readFirmwareVersion(uint8_t& ver);

    /*!
      @brief Read the current I2C address stored in flash (REG_I2C_ADDRESS 0xFF)
      @param[out] addr I2C address (0x08-0x77)
      @return True if successful
     */
    bool readI2CAddress(uint8_t& addr);

    /*!
      @brief Write a new I2C address to flash and update the active bus address
      @param i2c_address New I2C address (0x08-0x77)
      @return True if successful
      @note Flash storage must be erased before writing (~20 ms). This function blocks for
            I2C_ADDRESS_WRITE_DELAY_MS after writing. The new address takes effect immediately
            and is stored in the internal Flash.
      @warning To prolong Flash lifetime, avoid frequent writes. The firmware skips the actual
               write if the value matches the current Flash contents.
     */
    bool changeI2CAddress(const uint8_t i2c_address);
    ///@}

    ///@name Keyboard mode
    ///@{
    /*!
      @brief Read the keyboard operation mode from REG_MODE_KEYBOARD (0x10)
      @param[out] mode Current keyboard mode
      @return True if successful
     */
    bool readMode(tab5_keyboard::Mode& mode);

    /*!
      @brief Write the keyboard operation mode to REG_MODE_KEYBOARD (0x10)
      @param mode Target keyboard mode
      @return True if successful
      @note Mode switch clears the previous mode's event queue and releases the INT signal.
     */
    bool writeMode(const tab5_keyboard::Mode mode);

    /*!
      @brief Read the RGB LED operation mode from REG_MODE_RGB (0x11)
      @param[out] mode Current RGB mode
      @return True if successful
     */
    bool readRgbMode(tab5_keyboard::RgbMode& mode);

    /*!
      @brief Write the RGB LED operation mode to REG_MODE_RGB (0x11)
      @param mode Target RGB mode
      @return True if successful
     */
    bool writeRgbMode(const tab5_keyboard::RgbMode mode);
    ///@}

    ///@name Event reading
    ///@{
    /*!
      @brief Read the pending event count from REG_EVENT_NUM (0x02)
      @param[out] count Number of pending events in the current mode's queue (0-32)
      @return True if successful
     */
    bool readEventCount(uint8_t& count);
    ///@}

    ///@name RGB LED control (custom mode only)
    ///@{
    /*!
      @brief Write RGB color for one LED
      @param idx LED index: 0 = RGB1 (left, base address 0x60), 1 = RGB2 (right, base address 0x64)
      @param r Red component (0-255)
      @param g Green component (0-255)
      @param b Blue component (0-255)
      @return True if successful
      @note Effective only when RgbMode::Custom is active.
      @note Register layout: RGB1 uses 0x60(B)/0x61(G)/0x62(R); RGB2 uses 0x64(B)/0x65(G)/0x66(R).
            0x63 is unused (gap between RGB1 and RGB2).
      @note The B/G/R byte order follows the datasheet register map. Verify on actual hardware
            if colors appear incorrect (possible R/B swap in firmware).
     */
    bool writeRgb(const uint8_t idx, const uint8_t r, const uint8_t g, const uint8_t b);

    /*!
      @brief Read current RGB color of one LED in Custom mode buffer
      @param idx LED index: 0 = RGB1 (left), 1 = RGB2 (right)
      @param[out] r Red component (0-255)
      @param[out] g Green component (0-255)
      @param[out] b Blue component (0-255)
      @return True if idx valid and I2C reads succeeded
      @note Reads from REG_RGB1_B/G/R or REG_RGB2_B/G/R (skipping 0x63 gap)
     */
    bool readRgb(const uint8_t idx, uint8_t& r, uint8_t& g, uint8_t& b);
    ///@}

    ///@name Brightness
    ///@{
    /*!
      @brief Write global RGB brightness to REG_BRIGHTNESS (0x03)
      @param pct Brightness percentage (0-100)
      @return True if successful
     */
    bool writeBrightness(const uint8_t pct);

    /*!
      @brief Read current RGB global brightness from REG_BRIGHTNESS (0x03)
      @param[out] pct Brightness 0-100 (default 20 per datasheet)
      @return True if I2C read succeeded
     */
    bool readBrightness(uint8_t& pct);
    ///@}

    /*!
      @brief Get the firmware version cached by begin()
      @retval Firmware version byte (0 if begin() has not yet succeeded)
     */
    inline uint8_t firmwareVersion() const
    {
        return _firmware_version;
    }

    ///@name Bitwise key state (Normal mode only)
    ///@brief Per-key state tracking exposed as std::bitset<KEY_COUNT> snapshots.
    ///       HID and Character modes never update these bitsets, so all queries
    ///       return empty / false in those modes.
    ///@{
    //! @brief Bits of the keys currently pressed
    inline const tab5_keyboard::key_status_bits_t& nowBits() const
    {
        return _state.now;
    }
    //! @brief Snapshot of nowBits() taken at the end of the previous update()
    inline const tab5_keyboard::key_status_bits_t& previousBits() const
    {
        return _state.prev;
    }
    //! @brief Bits of keys that transitioned from released to pressed this update
    inline const tab5_keyboard::key_status_bits_t& pressedBits() const
    {
        return _state.pressed;
    }
    //! @brief Bits of keys that transitioned from pressed to released this update
    inline const tab5_keyboard::key_status_bits_t& releasedBits() const
    {
        return _state.released;
    }
    //! @brief Bits of keys whose press duration has reached holding_threshold_ms
    inline const tab5_keyboard::key_status_bits_t& holdingBits() const
    {
        return _state.holding;
    }
    //! @brief Bits of keys that became "holding" during this update (one-shot)
    inline const tab5_keyboard::key_status_bits_t& wasHoldBits() const
    {
        return _state.was_hold;
    }
    //! @brief Bits of keys for which a software-repeat tick fired this update
    inline const tab5_keyboard::key_status_bits_t& repeatingBits() const
    {
        return _state.repeating;
    }
    ///@}

    ///@name Any-key state queries (no arguments)
    ///@{
    //! @brief Is any key currently pressed?
    inline bool isPressed() const
    {
        return bitwise_active() && _state.now.any();
    }
    //! @brief Did any key get pressed (released-to-pressed transition) this update?
    inline bool wasPressed() const
    {
        return bitwise_active() && _state.pressed.any();
    }
    //! @brief Did any key get released (pressed-to-released transition) this update?
    inline bool wasReleased() const
    {
        return bitwise_active() && _state.released.any();
    }
    //! @brief Is any key currently being held (press duration >= holding_threshold_ms)?
    inline bool isHolding() const
    {
        return bitwise_active() && _state.holding.any();
    }
    //! @brief Did any key become "holding" during this update? (one-shot)
    inline bool wasHold() const
    {
        return bitwise_active() && _state.was_hold.any();
    }
    //! @brief Did any key fire a software-repeat tick during this update?
    inline bool isRepeating() const
    {
        return bitwise_active() && _state.repeating.any();
    }
    ///@}

    ///@name Per-key state queries (by flat key index)
    ///@brief Returns false when kidx >= KEY_COUNT (defensive, no assert).
    ///@{
    /*!
      @brief Is the specified key currently pressed?
      @param kidx Flat key index (0..KEY_COUNT-1)
      @return True if the key is currently held down
     */
    inline bool isPressed(const uint8_t kidx) const
    {
        return bitwise_active() && kidx < tab5_keyboard::KEY_COUNT && _state.now.test(kidx);
    }
    /*!
      @brief Did the specified key get pressed this update?
      @param kidx Flat key index (0..KEY_COUNT-1)
      @return True if a press transition occurred this update
     */
    inline bool wasPressed(const uint8_t kidx) const
    {
        return bitwise_active() && kidx < tab5_keyboard::KEY_COUNT && _state.pressed.test(kidx);
    }
    /*!
      @brief Did the specified key get released this update?
      @param kidx Flat key index (0..KEY_COUNT-1)
      @return True if a release transition occurred this update
     */
    inline bool wasReleased(const uint8_t kidx) const
    {
        return bitwise_active() && kidx < tab5_keyboard::KEY_COUNT && _state.released.test(kidx);
    }
    /*!
      @brief Is the specified key currently being held?
      @param kidx Flat key index (0..KEY_COUNT-1)
      @return True if the key has passed the hold threshold and is still held
     */
    inline bool isHolding(const uint8_t kidx) const
    {
        return bitwise_active() && kidx < tab5_keyboard::KEY_COUNT && _state.holding.test(kidx);
    }
    /*!
      @brief Did the specified key become "holding" during this update?
      @param kidx Flat key index (0..KEY_COUNT-1)
      @return True if the hold threshold was crossed this update
     */
    inline bool wasHold(const uint8_t kidx) const
    {
        return bitwise_active() && kidx < tab5_keyboard::KEY_COUNT && _state.was_hold.test(kidx);
    }
    /*!
      @brief Did the specified key fire a software-repeat tick during this update?
      @param kidx Flat key index (0..KEY_COUNT-1)
      @return True if a software repeat event fired this update
     */
    inline bool isRepeating(const uint8_t kidx) const
    {
        return bitwise_active() && kidx < tab5_keyboard::KEY_COUNT && _state.repeating.test(kidx);
    }
    ///@}

    ///@name Per-key state queries (by (row, col))
    ///@{
    //! @brief Is the specified key currently pressed?
    inline bool isPressed(const uint8_t row, const uint8_t col) const
    {
        return isPressed(tab5_keyboard::toKeyIndex(row, col));
    }
    //! @brief Did the specified key get pressed this update?
    inline bool wasPressed(const uint8_t row, const uint8_t col) const
    {
        return wasPressed(tab5_keyboard::toKeyIndex(row, col));
    }
    //! @brief Did the specified key get released this update?
    inline bool wasReleased(const uint8_t row, const uint8_t col) const
    {
        return wasReleased(tab5_keyboard::toKeyIndex(row, col));
    }
    //! @brief Is the specified key currently being held?
    inline bool isHolding(const uint8_t row, const uint8_t col) const
    {
        return isHolding(tab5_keyboard::toKeyIndex(row, col));
    }
    //! @brief Did the specified key become "holding" during this update?
    inline bool wasHold(const uint8_t row, const uint8_t col) const
    {
        return wasHold(tab5_keyboard::toKeyIndex(row, col));
    }
    //! @brief Did the specified key fire a software-repeat tick during this update?
    inline bool isRepeating(const uint8_t row, const uint8_t col) const
    {
        return isRepeating(tab5_keyboard::toKeyIndex(row, col));
    }
    ///@}

    ///@name Modifier shortcuts (Normal mode only)
    ///@brief Equivalent to isPressed(KIDX_<name>). Guarded by bitwise_active(), so these
    ///       return false in HID/Character modes (where _state is not tracked).
    ///@{
    //! @brief Is the Sym modifier currently pressed?
    inline bool isSym() const
    {
        return bitwise_active() && _state.now.test(tab5_keyboard::KIDX_SYM);
    }
    //! @brief Is the Aa modifier currently pressed?
    inline bool isAa() const
    {
        return bitwise_active() && _state.now.test(tab5_keyboard::KIDX_AA);
    }
    //! @brief Is the Ctrl modifier currently pressed?
    inline bool isCtrl() const
    {
        return bitwise_active() && _state.now.test(tab5_keyboard::KIDX_CTRL);
    }
    //! @brief Is the Alt modifier currently pressed?
    inline bool isAlt() const
    {
        return bitwise_active() && _state.now.test(tab5_keyboard::KIDX_ALT);
    }
    ///@}

    ///@name (row, col) → ASCII conversion (Normal mode only)
    ///@brief Convenience wrappers that combine the static HID lookup tables
    ///       (keyMatrixToHidBase / keyMatrixToHidSym) with the current Sym/Aa
    ///       state stored in @c nowBits() and dispatch through hidUsageToChar().
    ///@{
    /*!
      @brief Convert a matrix coordinate to its ASCII character using the
             currently-held modifier state.
      @param row Matrix row (0..4)
      @param col Matrix column (0..13)
      @retval !=0 Printable / whitespace ASCII character
      @retval 0   Out-of-range, modifier key, or non-printable HID code
      @note Reads @c nowBits() to determine Sym/Aa state. In HID and Character
            modes @c nowBits() stays empty, so this always returns 0 there.
      @note Aa is treated as Shift (OR 0x02 into the HID modifier byte). Sym
            selects the alternate (Sym) lookup table.
     */
    char keyMatrixToChar(const uint8_t row, const uint8_t col) const;

    /*!
      @brief Convert a flat key index (0..KEY_COUNT-1) to its ASCII character.
      @param kidx Flat key index
      @retval !=0 Printable / whitespace ASCII character
      @retval 0   Out-of-range, modifier key, or non-printable HID code
     */
    char keyMatrixToChar(const uint8_t kidx) const;
    ///@}

protected:
    /// @cond INTERNAL
    ///@name Internal mode-specific event readers (used by update() drain logic)
    ///@{
    /*!
      @brief Read one Normal-mode key event from REG_KEY_EVENT (0x20)
      @param[out] row Matrix row (0-4)
      @param[out] col Matrix column (0-13)
      @param[out] pressed True = key press, false = key release
      @return True if the I2C transaction succeeded (including when the queue is empty)
      @note When the queue is empty, row is set to KEY_EVENT_EMPTY (0xFF).
      @note Normal mode key event byte format: [7]=press/release, [6:4]=row(0-4), [3:0]=col(0-13).
     */
    bool read_key_event(uint8_t& row, uint8_t& col, bool& pressed);

    /*!
      @brief Read one HID-mode event from REG_HID_EVENT (0x30)
      @param[out] modifier HID modifier byte
      @param[out] keycode HID key code byte (0 on key release)
      @return True if the I2C transaction succeeded (including when the queue is empty)
      @note When the queue is empty, both modifier and keycode are set to KEY_EVENT_EMPTY (0xFF).
     */
    bool read_hid_event(uint8_t& modifier, uint8_t& keycode);

    /*!
      @brief Read one Character-mode event from REG_CHAR_EVENT_LENGTH (0x40) + REG_CHAR_EVENT (0x50)
      @param[out] modifier Modifier byte (bit0=Ctrl, bit2=Alt)
      @param[out] chars Character string for this event (up to 9 bytes)
      @return True if the I2C transaction succeeded (including when the queue is empty)
      @note When the queue is empty, length reads 0; modifier is set to 0 and chars is cleared.
      @note Reads length from REG_CHAR_EVENT_LENGTH (0x40) first.
            If length > 0, reads exactly `length` bytes from REG_CHAR_EVENT (0x50):
            byte[0]=modifier, byte[1..length-1]=char payload.
            (The datasheet note "+1" is a documentation error per firmware author clarification;
             REG_CHAR_EVENT_LENGTH already includes the modifier byte.)
     */
    bool read_char_event(uint8_t& modifier, std::string& chars);
    ///@}
    /// @endcond

    //! @brief Read one event from device for the given mode, fill `evt`. Returns false on I/O error.
    //! @note If queue is empty, sets `evt.type = EventType::None` and returns true.
    bool read_event_for_mode(const tab5_keyboard::Mode mode, tab5_keyboard::Event& evt);

    // PeriodicMeasurementAdapter lifecycle (called via startPeriodicMeasurement/stopPeriodicMeasurement)
    bool start_periodic_measurement();
    bool stop_periodic_measurement();

    /*!
      @brief ESP-IDF GPIO ISR handler (IRAM-safe, minimal work)
      @param arg Pointer to the owning UnitTab5Keyboard instance
     */
    static void IRAM_ATTR isr_handler(void* arg);

    /*!
      @brief Configure gpio_config + isr_handler_add for _cfg.irq_pin
      @return true if successful; false on failure (caller should fallback to polling)
     */
    bool configure_irq_pin();

    /*!
      @brief Remove isr_handler and reset _irq_pin_configured for a given pin
      @param pin Pin to remove (-1 is a no-op)
     */
    void remove_irq_pin(const int8_t pin);

private:
    //! @brief True only in Normal mode, where the bitwise key tracking (_state) is populated.
    //! HID/Character modes never update _state, so all is*/was* queries are guarded by this and
    //! return false there. (The raw *Bits() accessors are not guarded; they expose _state as-is.)
    inline bool bitwise_active() const
    {
        return _mode == tab5_keyboard::Mode::Normal;
    }

    // update() internals (split out to keep the per-tick flow readable).
    //! Decide whether update() should drain this tick. INT-driven gates on the INT signal;
    //! otherwise throttles to _interval (config_t::interval_ms), bypassed by force. On a true
    //! return it has consumed the latched IRQ flag and advanced _latest.
    bool ready_to_measure(const bool force);
    bool ready_to_measure_irq(const bool force);
    bool ready_to_measure_polling(const bool force);
    //! Drain the device event queue into _data, then release the INT latch.
    void drain_events(const tab5_keyboard::Mode mode);
    //! Normal-mode bitwise edge/hold/repeat processing + software auto-repeat synthesis.
    void process_normal_state();

    std::unique_ptr<m5::container::CircularBuffer<tab5_keyboard::Event>> _data{};

    config_t _cfg{};
    //! True once gpio_config() + gpio_isr_handler_add() have been called for _cfg.irq_pin
    bool _irq_pin_configured{false};
    //! True while the firmware INT is enabled (mirrors REG_INT_CFG; set by writeInterruptEnable()).
    //! update() uses INT-driven gating only when this is true; otherwise it polls unconditionally.
    bool _int_enabled{false};

    // Cached active mode. Updated by begin()/writeMode()/readMode(); read by update()
    // to avoid an extra I2C transaction per cycle.
    tab5_keyboard::Mode _mode{tab5_keyboard::Mode::Normal};

    // Bitwise key state (Normal mode only)
    m5::unit::keyboard_bitwise::BitwiseState<tab5_keyboard::KEY_COUNT> _state;

    uint8_t _firmware_version{};
    //! Set to true by isr_handler (ISR context); consumed and cleared in update()
    volatile bool _irq_pending{false};
};

}  // namespace unit
}  // namespace m5
#endif
