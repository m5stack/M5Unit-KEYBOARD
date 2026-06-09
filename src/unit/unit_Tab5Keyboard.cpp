/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_Tab5Keyboard.cpp
  @brief Tab5 Keyboard Unit for M5UnitUnified
*/
#include "unit_Tab5Keyboard.hpp"

#include <algorithm>
#include <cstring>

#include <M5Utility.hpp>

#include "../utility/hid_keycode.hpp"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
// GPIO ISR API (gpio_config / gpio_install_isr_service / gpio_isr_handler_add / ...) used below.
// The public header only pulls in <esp_attr.h> for IRAM_ATTR, so the driver include lives here.
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#endif

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::tab5_keyboard;
using namespace m5::unit::tab5_keyboard::command;

namespace m5 {
namespace unit {

const char UnitTab5Keyboard::name[] = "UnitTab5Keyboard";
const types::uid_t UnitTab5Keyboard::uid{"UnitTab5Keyboard"_mmh3};
const types::attr_t UnitTab5Keyboard::attr{attribute::AccessI2C};

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------

UnitTab5Keyboard::~UnitTab5Keyboard()
{
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
    remove_irq_pin(_cfg.irq_pin);
#endif
}

}  // namespace unit
}  // namespace m5

// ---------------------------------------------------------------------------
// HID mapping tables (Tab5 Keyboard 5x14 matrix → HID Usage Code + modifier)
//
// Two tables: base (no Sym held) and Sym (Sym held). Modifier keys
// (Sym/Aa/Ctrl/Alt) map to {0, 0} — they generate no character themselves.
// Tables were captured from device HID-mode logs on real hardware (US ANSI).
// ---------------------------------------------------------------------------
namespace m5 {
namespace unit {
namespace tab5_keyboard {

namespace {

// Row-major (row * 14 + col) — 5 rows x 14 cols = 70 entries each.
//
// Modifier byte 0x02 = Left Shift (US HID modifier bit 1). The firmware
// reports it pre-baked for keys that physically print a shifted character
// (e.g. `!` shares keycode 0x1E with `1` plus a forced 0x02 shift).
constexpr HidMapping KEY_MATRIX_HID_BASE[KEY_COUNT] = {
    // Row 0: Esc 1 2 3 4 5 6 7 8 9 0 - + Del
    {0x29, 0x00},
    {0x1E, 0x00},
    {0x1F, 0x00},
    {0x20, 0x00},
    {0x21, 0x00},
    {0x22, 0x00},
    {0x23, 0x00},
    {0x24, 0x00},
    {0x25, 0x00},
    {0x26, 0x00},
    {0x27, 0x00},
    {0x2D, 0x00},
    {0x2E, 0x02},
    {0x4C, 0x00},
    // Row 1: ` ! @ # $ % ^ & * ( ) [ ] backslash
    {0x35, 0x00},
    {0x1E, 0x02},
    {0x1F, 0x02},
    {0x20, 0x02},
    {0x21, 0x02},
    {0x22, 0x02},
    {0x23, 0x02},
    {0x24, 0x02},
    {0x25, 0x02},
    {0x26, 0x02},
    {0x27, 0x02},
    {0x2F, 0x00},
    {0x30, 0x00},
    {0x31, 0x00},
    // Row 2: Tab q w e r t y u i o p ; ' Backspace
    {0x2B, 0x00},
    {0x14, 0x00},
    {0x1A, 0x00},
    {0x08, 0x00},
    {0x15, 0x00},
    {0x17, 0x00},
    {0x1C, 0x00},
    {0x18, 0x00},
    {0x0C, 0x00},
    {0x12, 0x00},
    {0x13, 0x00},
    {0x33, 0x00},
    {0x34, 0x00},
    {0x2A, 0x00},
    // Row 3: Sym Aa a s d f g h j k l ↑ _ Enter
    {0x00, 0x00},
    {0x00, 0x00},
    {0x04, 0x00},
    {0x16, 0x00},
    {0x07, 0x00},
    {0x09, 0x00},
    {0x0A, 0x00},
    {0x0B, 0x00},
    {0x0D, 0x00},
    {0x0E, 0x00},
    {0x0F, 0x00},
    {0x52, 0x00},
    {0x2D, 0x02},
    {0x28, 0x00},
    // Row 4: Ctrl Alt z x c v b n m . ← ↓ → Space
    {0x00, 0x00},
    {0x00, 0x00},
    {0x1D, 0x00},
    {0x1B, 0x00},
    {0x06, 0x00},
    {0x19, 0x00},
    {0x05, 0x00},
    {0x11, 0x00},
    {0x10, 0x00},
    {0x37, 0x00},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x4F, 0x00},
    {0x2C, 0x00},
};

// Sym-layer table: most entries are identical to the base table; only the keys
// whose printed character changes when Sym is held are different. We start
// from a copy of the base table and override the deltas via an aggregate
// initializer that re-lists all 70 entries to keep the array immutable
// (constexpr-able once the codebase moves to C++14+).
constexpr HidMapping KEY_MATRIX_HID_SYM[KEY_COUNT] = {
    // Row 0 — identical to base (no Sym variants on the number row)
    {0x29, 0x00},
    {0x1E, 0x00},
    {0x1F, 0x00},
    {0x20, 0x00},
    {0x21, 0x00},
    {0x22, 0x00},
    {0x23, 0x00},
    {0x24, 0x00},
    {0x25, 0x00},
    {0x26, 0x00},
    {0x27, 0x00},
    {0x2D, 0x00},
    {0x2E, 0x02},
    {0x4C, 0x00},
    // Row 1 — Sym deltas: ` → ~, ! → ?, * → /, ( → <, ) → >, [ → {, ] → }, backslash → |
    {0x35, 0x02},  // (1,0)  ` → ~
    {0x38, 0x02},  // (1,1)  ! → ?
    {0x1F, 0x02},  // (1,2)  @  (unchanged)
    {0x20, 0x02},  // (1,3)  #  (unchanged)
    {0x21, 0x02},  // (1,4)  $  (unchanged)
    {0x22, 0x02},  // (1,5)  %  (unchanged)
    {0x23, 0x02},  // (1,6)  ^  (unchanged)
    {0x24, 0x02},  // (1,7)  &  (unchanged)
    {0x38, 0x00},  // (1,8)  * → /
    {0x36, 0x02},  // (1,9)  ( → <
    {0x37, 0x02},  // (1,10) ) → >
    {0x2F, 0x02},  // (1,11) [ → {
    {0x30, 0x02},  // (1,12) ] → }
    {0x31, 0x02},  // (1,13) backslash → |
    // Row 2 — Sym deltas: ; → :, ' → "
    {0x2B, 0x00},
    {0x14, 0x00},
    {0x1A, 0x00},
    {0x08, 0x00},
    {0x15, 0x00},
    {0x17, 0x00},
    {0x1C, 0x00},
    {0x18, 0x00},
    {0x0C, 0x00},
    {0x12, 0x00},
    {0x13, 0x00},
    {0x33, 0x02},  // (2,11) ; → :
    {0x34, 0x02},  // (2,12) ' → "
    {0x2A, 0x00},
    // Row 3 — Sym delta: _ → = (modifier cleared)
    {0x00, 0x00},
    {0x00, 0x00},
    {0x04, 0x00},
    {0x16, 0x00},
    {0x07, 0x00},
    {0x09, 0x00},
    {0x0A, 0x00},
    {0x0B, 0x00},
    {0x0D, 0x00},
    {0x0E, 0x00},
    {0x0F, 0x00},
    {0x52, 0x00},
    {0x2E, 0x00},  // (3,12) _ → =
    {0x28, 0x00},
    // Row 4 — Sym delta: . → ,
    {0x00, 0x00},
    {0x00, 0x00},
    {0x1D, 0x00},
    {0x1B, 0x00},
    {0x06, 0x00},
    {0x19, 0x00},
    {0x05, 0x00},
    {0x11, 0x00},
    {0x10, 0x00},
    {0x36, 0x00},  // (4,9)  . → ,
    {0x50, 0x00},
    {0x51, 0x00},
    {0x4F, 0x00},
    {0x2C, 0x00},
};

}  // namespace

HidMapping keyMatrixToHidBase(const uint8_t row, const uint8_t col)
{
    if (row >= 5U || col >= KEY_COL_COUNT) {
        return HidMapping{0, 0};
    }
    return KEY_MATRIX_HID_BASE[toKeyIndex(row, col)];
}

HidMapping keyMatrixToHidSym(const uint8_t row, const uint8_t col)
{
    if (row >= 5U || col >= KEY_COL_COUNT) {
        return HidMapping{0, 0};
    }
    return KEY_MATRIX_HID_SYM[toKeyIndex(row, col)];
}

}  // namespace tab5_keyboard
}  // namespace unit
}  // namespace m5

namespace m5 {
namespace unit {

// ---------------------------------------------------------------------------
// (row, col) → ASCII conversion (Normal mode helpers)
// ---------------------------------------------------------------------------

char UnitTab5Keyboard::keyMatrixToChar(const uint8_t row, const uint8_t col) const
{
    if (row >= 5U || col >= tab5_keyboard::KEY_COL_COUNT) {
        return 0;
    }
    if (tab5_keyboard::isModifierKey(row, col)) {
        return 0;
    }
    const tab5_keyboard::HidMapping mapping =
        isSym() ? tab5_keyboard::keyMatrixToHidSym(row, col) : tab5_keyboard::keyMatrixToHidBase(row, col);
    if (mapping.keycode == 0U) {
        return 0;
    }
    // Aa behaves like Shift: OR 0x02 into the HID modifier byte.
    const uint8_t modifier = static_cast<uint8_t>(mapping.modifier | (isAa() ? 0x02U : 0x00U));
    return tab5_keyboard::hidUsageToChar(mapping.keycode, modifier);
}

char UnitTab5Keyboard::keyMatrixToChar(const uint8_t kidx) const
{
    if (kidx >= tab5_keyboard::KEY_COUNT) {
        return 0;
    }
    const uint8_t row = static_cast<uint8_t>(kidx / tab5_keyboard::KEY_COL_COUNT);
    const uint8_t col = static_cast<uint8_t>(kidx % tab5_keyboard::KEY_COL_COUNT);
    return keyMatrixToChar(row, col);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)

void IRAM_ATTR UnitTab5Keyboard::isr_handler(void* arg)
{
    auto* self = static_cast<UnitTab5Keyboard*>(arg);
    if (self) {
        self->_irq_pending = true;
    }
}

bool UnitTab5Keyboard::configure_irq_pin()
{
    // _cfg.irq_pin must be valid before calling.
    const gpio_num_t gpio = static_cast<gpio_num_t>(_cfg.irq_pin);

    // Configure GPIO: input, pull-up, trigger on falling edge (INT active-low).
    // Use member-assignment instead of designated init to remain compatible
    // across ESP-IDF versions that add new fields (e.g., hys_ctrl_mode in 5.5+).
    gpio_config_t io_conf{};
    io_conf.pin_bit_mask    = (1ULL << gpio);
    io_conf.mode            = GPIO_MODE_INPUT;
    io_conf.pull_up_en      = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en    = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type       = GPIO_INTR_NEGEDGE;
    const esp_err_t cfg_err = gpio_config(&io_conf);
    if (cfg_err != ESP_OK) {
        M5_LIB_LOGE("gpio_config failed for pin %d: %s", _cfg.irq_pin, esp_err_to_name(cfg_err));
        return false;
    }

    // Install the per-pin ISR service (shared across all GPIO users).
    // ESP_ERR_INVALID_STATE means already installed — that is acceptable.
    const esp_err_t svc_err = gpio_install_isr_service(0);
    if (svc_err != ESP_OK && svc_err != ESP_ERR_INVALID_STATE) {
        M5_LIB_LOGE("gpio_install_isr_service failed: %s", esp_err_to_name(svc_err));
        return false;
    }
    if (svc_err == ESP_ERR_INVALID_STATE) {
        M5_LIB_LOGD("gpio_install_isr_service: already installed (OK)");
    }

    // Register per-pin handler.
    const esp_err_t add_err = gpio_isr_handler_add(gpio, &UnitTab5Keyboard::isr_handler, this);
    if (add_err != ESP_OK) {
        M5_LIB_LOGE("gpio_isr_handler_add failed for pin %d: %s; falling back to polling mode", _cfg.irq_pin,
                    esp_err_to_name(add_err));
        // Disable the interrupt type so the GPIO does not trigger spuriously.
        gpio_set_intr_type(gpio, GPIO_INTR_DISABLE);
        // Reset pin config so update() falls back to unconditional polling.
        _cfg.irq_pin = -1;
        return false;
    }

    _irq_pin_configured = true;
    M5_LIB_LOGI("INT pin %d configured (NEGEDGE ISR)", _cfg.irq_pin);
    return true;
}

void UnitTab5Keyboard::remove_irq_pin(const int8_t pin)
{
    if (pin < 0 || !_irq_pin_configured) {
        return;
    }
    const gpio_num_t gpio   = static_cast<gpio_num_t>(pin);
    const esp_err_t rem_err = gpio_isr_handler_remove(gpio);
    if (rem_err != ESP_OK) {
        M5_LIB_LOGW("gpio_isr_handler_remove failed for pin %d: %s", pin, esp_err_to_name(rem_err));
    }
    _irq_pin_configured = false;
    _irq_pending        = false;
    M5_LIB_LOGD("ISR removed for pin %d", pin);
}

#else  // !(ARDUINO_ARCH_ESP32 || ESP_PLATFORM)

// Stub implementations for non-ESP32 builds (native test hosts, etc.).
void UnitTab5Keyboard::isr_handler(void* /*arg*/)
{
}
bool UnitTab5Keyboard::configure_irq_pin()
{
    return false;
}
void UnitTab5Keyboard::remove_irq_pin(const int8_t /*pin*/)
{
}

#endif  // ARDUINO_ARCH_ESP32 || ESP_PLATFORM

// ---------------------------------------------------------------------------
// begin / update
// ---------------------------------------------------------------------------

bool UnitTab5Keyboard::begin()
{
    // Resize internal event buffer if user changed stored_size via component_config()
    const auto ssize = stored_size();
    if (ssize < 1) {
        M5_LIB_LOGE("stored_size must be >= 1");
        return false;
    }
    if (!_data || ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<tab5_keyboard::Event>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate event buffer (size=%u)", ssize);
            return false;
        }
    }

    // Verify I2C connectivity via base class (includes SoftwareI2C retry workaround).
    if (!UnitKeyboard::begin()) {
        M5_LIB_LOGE("Failed to communicate with Tab5 Keyboard");
        return false;
    }

    // Read firmware version from REG_FIRMWARE_VERSION (0xFE) as a basic sanity check.
    if (!readFirmwareVersion(_firmware_version)) {
        M5_LIB_LOGE("Failed to read firmware version");
        return false;
    }
    M5_LIB_LOGI("Type:%s Firmware:%02X", name, _firmware_version);

    // Apply user-configured initial keyboard mode (REG_MODE_KEYBOARD 0x10).
    // writeMode() also clears the previous mode's event queue and releases INT.
    Mode cur{};
    if (!readMode(cur) || !writeMode(_cfg.mode)) {
        M5_LIB_LOGE("Failed to set initial mode %u -> (%u)", static_cast<unsigned>(cur),
                    static_cast<unsigned>(_cfg.mode));
        return false;
    }

    // Clear any residual interrupts and flush the event queue before normal operation.
    if (!clearInterrupt()) {
        M5_LIB_LOGW("Failed to clear interrupts on begin");
    }
    if (!writeRegister8(REG_EVENT_NUM, static_cast<uint8_t>(0U))) {
        M5_LIB_LOGW("Failed to clear event queue on begin");
    }

    // Configure INT GPIO pin if the user has set one via config().
    if (_cfg.irq_pin >= 0 && !_irq_pin_configured) {
        if (!configure_irq_pin()) {
            M5_LIB_LOGW("IRQ pin setup failed; falling back to polling mode");
            // _cfg.irq_pin has already been reset to -1 inside configure_irq_pin() on failure.
        }
    }

    // Clear internal buffer to ensure clean state after (re-)begin().
    flush();

    // Start periodic measurement when configured (mirrors the other M5Unit keyboards).
    // startPeriodicMeasurement() enables the INT for the active mode via the lifecycle hook.
    return _cfg.start_periodic ? startPeriodicMeasurement() : true;
}

bool UnitTab5Keyboard::ready_to_measure(const bool force)
{
    return (_cfg.irq_pin >= 0 && _int_enabled) ? ready_to_measure_irq(force) : ready_to_measure_polling(force);
}

bool UnitTab5Keyboard::ready_to_measure_irq(const bool force)
{
    // Purely interrupt-driven: drain only when the ISR has latched a NEGEDGE (no GPIO polling).
    // drain_events() empties the whole queue and clearInterrupt() releases the line, so each
    // later device event produces a fresh edge -> _irq_pending. Hold/repeat timing is advanced
    // by update() every tick, so this need not stay true during a sustained hold.
    if (force || _irq_pending) {
        _irq_pending = false;  // consume the latched flag now that we are committing to a drain
        return true;
    }
    return false;
}

bool UnitTab5Keyboard::ready_to_measure_polling(const bool force)
{
    // Non-interrupt path: drain at most once per _interval (config_t::interval_ms).
    if (force) {
        return true;
    }
    const auto at = m5::utility::millis();
    return _latest == 0 || (at - _latest) >= _interval;
}

void UnitTab5Keyboard::drain_events(const tab5_keyboard::Mode mode)
{
    // Side effect for Normal mode: read_event_for_mode() updates _state via setKey().
    uint8_t remaining = 0;
    while (readEventCount(remaining) && remaining > 0) {
        tab5_keyboard::Event evt{};
        if (!read_event_for_mode(mode, evt)) break;
        if (evt.type == tab5_keyboard::EventType::None) {
            break;
        }
        _data->push_back(evt);
    }

    // re-asserts INT immediately, so there is no risk of missed edges.
    if (_cfg.irq_pin >= 0) {
        clearInterrupt();
    }
}

void UnitTab5Keyboard::process_normal_state()
{
    // Edge detection: presses / releases that occurred since the previous measurement.
    _state.computeEdges();

    // Time-based per-key transitions (hold / repeat).
    _state.holding_threshold_ms = _cfg.holding_threshold_ms;
    _state.repeat_initial_ms    = _cfg.repeat_initial_ms;
    _state.repeat_rate_ms       = _cfg.repeat_rate_ms;
    _state.tickHoldRepeat(m5::utility::millis());

    // Software auto-repeat synthesis: emit a synthetic key-press Event for each
    // key flagged in _state.repeating, excluding modifier keys (Sym/Aa/Ctrl/Alt).
    if (_cfg.software_repeat) {
        for (uint8_t i = 0; i < tab5_keyboard::KEY_COUNT; ++i) {
            if (!_state.repeating.test(i)) continue;
            const uint8_t row = static_cast<uint8_t>(i / tab5_keyboard::KEY_COL_COUNT);
            const uint8_t col = static_cast<uint8_t>(i % tab5_keyboard::KEY_COL_COUNT);
            if (isModifierKey(row, col)) continue;
            tab5_keyboard::Event evt{};
            evt.type        = tab5_keyboard::EventType::Key;
            evt.repeat      = true;
            evt.key.row     = row;
            evt.key.col     = col;
            evt.key.pressed = true;
            if (_data) _data->push_back(evt);
        }
    }
}

void UnitTab5Keyboard::update(const bool force)
{
    // Honor the periodic-measurement lifecycle (mirrors the other M5Unit keyboards):
    // do nothing until startPeriodicMeasurement() has been called (begin() does this
    // automatically when config_t::start_periodic is true).
    if (!inPeriodic()) {
        return;
    }

    UnitKeyboard::update(force);

    const bool do_drain = ready_to_measure(force);

    // Use the cached mode (kept in sync by begin()/writeMode()/readMode()) to avoid an
    // extra I2C transaction per update cycle.
    const tab5_keyboard::Mode current = _mode;

    if (current != tab5_keyboard::Mode::Normal) {
        // HID / Character: no bitwise state to track; only drain the device queue when ready.
        if (do_drain) {
            _latest = m5::utility::millis();  // time key state was last (re)read
            drain_events(current);
        }
        return;
    }

    // Normal mode: run the bitwise pipeline EVERY tick so hold/repeat timing advances even when
    // no device events arrive (e.g. a sustained hold under interrupt drive emits no new edges).
    // Reading the device queue itself stays gated by ready_to_measure() (IRQ flag / poll interval).
    _state.commitPrev();
    _state.resetOneShot();
    if (do_drain) {
        _latest = m5::utility::millis();  // time key state was last (re)read
        drain_events(current);            // setKey() updates _state.now
    }
    process_normal_state();  // computeEdges + tickHoldRepeat + software-repeat synthesis
}

// ---------------------------------------------------------------------------
// writeInterruptEnable / readInterruptEnable / readInterruptStatus / clearInterrupt
// ---------------------------------------------------------------------------

bool UnitTab5Keyboard::writeInterruptEnable(const bool normal, const bool hid, const bool character)
{
    // Compose INT_CFG bitmask: bit0=Normal, bit1=HID, bit2=Character
    const uint8_t mask = static_cast<uint8_t>((normal ? 0x01U : 0U) | (hid ? 0x02U : 0U) | (character ? 0x04U : 0U));
    if (!writeRegister8(REG_INT_CFG, mask)) {
        return false;
    }
    _int_enabled = (mask != 0U);
    return true;
}

bool UnitTab5Keyboard::readInterruptEnable(bool& normal, bool& hid, bool& character)
{
    // Read back the per-mode enable bits from REG_INT_CFG (distinct from the pending
    // status in REG_INT_STAT; see readInterruptStatus()).
    uint8_t raw{};
    normal    = false;
    hid       = false;
    character = false;
    if (!readRegister8(REG_INT_CFG, raw, 0U)) {
        return false;
    }
    normal    = (raw & 0x01U) != 0U;
    hid       = (raw & 0x02U) != 0U;
    character = (raw & 0x04U) != 0U;
    return true;
}

bool UnitTab5Keyboard::readInterruptStatus(bool& normal, bool& hid, bool& character)
{
    // Read the per-mode pending bits from REG_INT_STAT (distinct from the enable
    // configuration in REG_INT_CFG; see readInterruptEnable()).
    uint8_t raw{};
    normal    = false;
    hid       = false;
    character = false;
    if (!readRegister8(REG_INT_STAT, raw, 0U)) {
        return false;
    }
    normal    = (raw & 0x01U) != 0U;
    hid       = (raw & 0x02U) != 0U;
    character = (raw & 0x04U) != 0U;
    return true;
}

bool UnitTab5Keyboard::clearInterrupt()
{
    // Datasheet: write 0 to REG_INT_STAT releases the INT signal and clears all pending flags.
    return writeRegister8(REG_INT_STAT, 0x00U);
}

bool UnitTab5Keyboard::clearEventQueue()
{
    // Datasheet: writing 0 to REG_EVENT_NUM clears the current mode queue and releases INT.
    if (!writeRegister8(REG_EVENT_NUM, static_cast<uint8_t>(0U))) {
        return false;
    }
    flush();
    _irq_pending = false;
    return true;
}

// ---------------------------------------------------------------------------
// Firmware / device info
// ---------------------------------------------------------------------------

bool UnitTab5Keyboard::readFirmwareVersion(uint8_t& ver)
{
    ver = 0;
    return readRegister8(REG_FIRMWARE_VERSION, ver, 0U);
}

bool UnitTab5Keyboard::readI2CAddress(uint8_t& addr)
{
    addr = 0;
    return readRegister8(REG_I2C_ADDRESS, addr, 0U);
}

bool UnitTab5Keyboard::changeI2CAddress(const uint8_t i2c_address)
{
    // Validate I2C address range (7-bit: 0x08-0x77).
    if (i2c_address < 0x08U || i2c_address > 0x77U) {
        M5_LIB_LOGE("Invalid I2C address: 0x%02X (valid range: 0x08-0x77)", i2c_address);
        return false;
    }
    if (!writeRegister8(REG_I2C_ADDRESS, i2c_address)) {
        return false;
    }
    // Wait for Flash erase + write to complete (~20 ms per datasheet).
    m5::utility::delay(I2C_ADDRESS_WRITE_DELAY_MS);
    // Update the adapter's internal address so subsequent transactions use the new address.
    return changeAddress(i2c_address);
}

// ---------------------------------------------------------------------------
// Keyboard mode / RGB mode
// ---------------------------------------------------------------------------

bool UnitTab5Keyboard::readMode(tab5_keyboard::Mode& mode)
{
    uint8_t raw{};
    mode = tab5_keyboard::Mode::Normal;
    if (!readRegister8(REG_MODE_KEYBOARD, raw, 0U)) {
        return false;
    }
    // Only values 0/1/2 are defined; reject anything else.
    if (raw > static_cast<uint8_t>(Mode::Character)) {
        M5_LIB_LOGW("Unknown keyboard mode value: 0x%02X", raw);
        return false;
    }
    mode  = static_cast<Mode>(raw);
    _mode = mode;
    return true;
}

bool UnitTab5Keyboard::writeMode(const tab5_keyboard::Mode mode)
{
    const uint8_t raw = static_cast<uint8_t>(mode);
    if (!writeRegister8(REG_MODE_KEYBOARD, raw)) return false;
    flush();               // mirror device's queue-clear-on-mode-switch behavior
    _irq_pending = false;  // device released INT per datasheet
    // Drop all bitwise tracking state on mode change. Software auto-repeat
    // synthesis halts naturally because _state.now is empty.
    _state.resetAll();
    _mode = mode;
    // INT_CFG is per-mode and persists across this write, so re-point it at the new mode
    // when INT is currently enabled. Without this the INT would stop firing after a switch.
    if (_int_enabled && _cfg.irq_pin >= 0) {
        writeInterruptEnable(mode == tab5_keyboard::Mode::Normal, mode == tab5_keyboard::Mode::HID,
                             mode == tab5_keyboard::Mode::Character);
    }
    return true;
}

bool UnitTab5Keyboard::readRgbMode(tab5_keyboard::RgbMode& mode)
{
    uint8_t raw{};
    mode = tab5_keyboard::RgbMode::Bind;
    if (!readRegister8(REG_MODE_RGB, raw, 0U)) {
        return false;
    }
    // Only values 0/1 are defined; reject anything else.
    if (raw > static_cast<uint8_t>(RgbMode::Custom)) {
        M5_LIB_LOGW("Unknown RGB mode value: 0x%02X", raw);
        return false;
    }
    mode = static_cast<RgbMode>(raw);
    return true;
}

bool UnitTab5Keyboard::writeRgbMode(const tab5_keyboard::RgbMode mode)
{
    return writeRegister8(REG_MODE_RGB, static_cast<uint8_t>(mode));
}

// ---------------------------------------------------------------------------
// Event reading
// ---------------------------------------------------------------------------

bool UnitTab5Keyboard::read_key_event(uint8_t& row, uint8_t& col, bool& pressed)
{
    uint8_t raw{};
    row = col = 0;
    pressed   = false;
    if (!readRegister8(REG_KEY_EVENT, raw, 0U)) {
        return false;
    }
    // Queue empty sentinel: 0xFF.
    // Return true (I2C succeeded) but signal empty via row == KEY_EVENT_EMPTY.
    if (raw == KEY_EVENT_EMPTY) {
        row     = KEY_EVENT_EMPTY;
        col     = 0;
        pressed = false;
        return true;
    }
    // Format: [7]=press/release, [6:4]=row(0-4), [3:0]=col(0-13).
    pressed = (raw & 0x80U) != 0U;
    row     = static_cast<uint8_t>((raw >> 4U) & 0x07U);
    col     = static_cast<uint8_t>(raw & 0x0FU);
    M5_LIB_LOGV("KEY_EVENT raw:0x%02X pressed:%d row:%u col:%u", raw, pressed, row, col);
    return true;
}

bool UnitTab5Keyboard::read_hid_event(uint8_t& modifier, uint8_t& keycode)
{
    // Read 2 bytes: Modifier(0x30) + Key_code(0x31).
    uint8_t buf[2]{};
    modifier = keycode = 0;
    if (!readRegister(REG_HID_EVENT, buf, sizeof(buf), 0U)) {
        return false;
    }
    modifier = buf[0];
    keycode  = buf[1];
    // Queue empty sentinel: both bytes 0xFF.
    if (modifier == KEY_EVENT_EMPTY && keycode == KEY_EVENT_EMPTY) {
        M5_LIB_LOGV("HID_EVENT: queue empty");
        return true;
    }
    M5_LIB_LOGV("HID_EVENT modifier:0x%02X keycode:0x%02X", modifier, keycode);
    return true;
}

bool UnitTab5Keyboard::read_char_event(uint8_t& modifier, std::string& chars)
{
    // Step 1: Read char payload length from REG_CHAR_EVENT_LENGTH (0x40).
    uint8_t len{};
    modifier = 0;
    chars    = "";
    if (!readRegister8(REG_CHAR_EVENT_LENGTH, len, 0U)) {
        return false;
    }
    // Per firmware author clarification (M5Stack issue reply): the datasheet
    // section 5 note "实际要读取的长度应该是 0x40 寄存器读回来的值 +1" is a
    // documentation error. REG_CHAR_EVENT_LENGTH (0x40) actually returns the
    // TOTAL event byte count (1 modifier + N chars). Read exactly `len` bytes
    // from REG_CHAR_EVENT (0x50). No +1 needed.
    if (len == 0U) {
        // Queue empty.
        modifier = 0;
        chars.clear();
        return true;
    }
    // Physical register window: 0x50..0x59 = 10 bytes total (1 modifier + 9 chars).
    if (len > CHAR_EVENT_MAX_CHARS + 1U) {
        M5_LIB_LOGE("CHAR_EVENT_LENGTH=%u exceeds physical max %u", len, CHAR_EVENT_MAX_CHARS + 1U);
        return false;
    }
    uint8_t buf[CHAR_EVENT_MAX_CHARS + 1U]{};
    if (!readRegister(REG_CHAR_EVENT, buf, len, 0U)) {
        return false;
    }
    modifier             = buf[0];
    const uint8_t n_char = static_cast<uint8_t>(len - 1U);
    chars.assign(reinterpret_cast<const char*>(buf + 1), n_char);
    M5_LIB_LOGV("CHAR_EVENT modifier:0x%02X chars_len:%u", modifier, n_char);
    return true;
}

bool UnitTab5Keyboard::readEventCount(uint8_t& count)
{
    count = 0;
    return readRegister8(REG_EVENT_NUM, count, 0U);
}

// ---------------------------------------------------------------------------
// RGB LED / Brightness
// ---------------------------------------------------------------------------

bool UnitTab5Keyboard::writeRgb(const uint8_t idx, const uint8_t r, const uint8_t g, const uint8_t b)
{
    if (idx >= RGB_LED_COUNT) {
        M5_LIB_LOGE("Invalid RGB LED index: %u (max %u)", idx, RGB_LED_COUNT - 1U);
        return false;
    }
    // Register base addresses per LED.
    // RGB1: 0x60(B)/0x61(G)/0x62(R); RGB2: 0x64(B)/0x65(G)/0x66(R).
    // Note: 0x63 is a gap — do NOT use idx*3 arithmetic.
    const uint8_t base = (idx == 0U) ? REG_RGB1_B : REG_RGB2_B;
    return writeRegister8(base, b) && writeRegister8(static_cast<uint8_t>(base + 1U), g) &&
           writeRegister8(static_cast<uint8_t>(base + 2U), r);
}

bool UnitTab5Keyboard::writeBrightness(const uint8_t pct)
{
    if (pct > 100U) {
        M5_LIB_LOGE("Brightness out of range: %u (max 100)", pct);
        return false;
    }
    return writeRegister8(REG_BRIGHTNESS, pct);
}

bool UnitTab5Keyboard::readBrightness(uint8_t& pct)
{
    pct = 0;
    return readRegister8(REG_BRIGHTNESS, pct, 0U);
}

bool UnitTab5Keyboard::readRgb(const uint8_t idx, uint8_t& r, uint8_t& g, uint8_t& b)
{
    r = 0;
    g = 0;
    b = 0;
    if (idx >= RGB_LED_COUNT) {
        M5_LIB_LOGE("Invalid RGB LED index: %u (max %u)", idx, RGB_LED_COUNT - 1U);
        return false;
    }
    // Register base addresses per LED (mirrors writeRgb layout).
    // RGB1: 0x60(B)/0x61(G)/0x62(R); RGB2: 0x64(B)/0x65(G)/0x66(R).
    // Note: 0x63 is a gap — do NOT use idx*4 arithmetic.
    const uint8_t base = (idx == 0U) ? REG_RGB1_B : REG_RGB2_B;
    return readRegister8(base, b, 0U) && readRegister8(static_cast<uint8_t>(base + 1U), g, 0U) &&
           readRegister8(static_cast<uint8_t>(base + 2U), r, 0U);
}

// ---------------------------------------------------------------------------
// PeriodicMeasurementAdapter lifecycle
// ---------------------------------------------------------------------------
bool UnitTab5Keyboard::start_periodic_measurement()
{
    if (_periodic) {
        return false;
    }
    // Enable INT only when an INT pin is wired; polling mode (irq_pin < 0) needs no INT.
    // INT_CFG is per-mode, so enable only the active mode's bit; writeMode() re-points it
    // when the mode changes. If INT was requested (irq_pin >= 0) but cannot be enabled, fail.
    if (_cfg.irq_pin >= 0) {
        if (!writeInterruptEnable(_cfg.mode == tab5_keyboard::Mode::Normal, _cfg.mode == tab5_keyboard::Mode::HID,
                                  _cfg.mode == tab5_keyboard::Mode::Character)) {
            return false;
        }
    }
    flush();
    _irq_pending = false;
    _latest      = 0;                 // reset the polling-interval timer (Component base member)
    _interval    = _cfg.interval_ms;  // polling interval used by update() when not INT-driven
    _periodic    = true;              // PeriodicMeasurementAdapter does not set this; the hook must.
    return true;
}

bool UnitTab5Keyboard::stop_periodic_measurement()
{
    // PeriodicMeasurementAdapter does not clear this; the hook must. Mark stopped first so
    // the unit is consistently "not periodic" even if disabling the INT fails below.
    _periodic = false;
    // Disable INT but keep buffered events for final drain.
    if (_cfg.irq_pin >= 0) {
        return writeInterruptEnable(false, false, false);
    }
    return true;
}

// ---------------------------------------------------------------------------
// read_event_for_mode
// ---------------------------------------------------------------------------

bool UnitTab5Keyboard::read_event_for_mode(const tab5_keyboard::Mode mode, tab5_keyboard::Event& evt)
{
    using namespace tab5_keyboard;
    switch (mode) {
        case Mode::Normal: {
            uint8_t row{}, col{};
            bool pressed{};
            if (!read_key_event(row, col, pressed)) return false;
            if (row == KEY_EVENT_EMPTY) {
                evt.type = EventType::None;
                return true;
            }
            evt.type        = EventType::Key;
            evt.key.row     = row;
            evt.key.col     = col;
            evt.key.pressed = pressed;
            // Bitwise tracking: update _state for every key event in Normal mode.
            // Modifier keys participate in _state.now (so isSym()/isAa()/isCtrl()/isAlt()
            // work), but the update() synthesizer skips them for software auto-repeat.
            if (row < 5U && col < KEY_COL_COUNT) {
                const uint8_t kidx = toKeyIndex(row, col);
                _state.setKey(kidx, pressed, m5::utility::millis());
            }
            return true;
        }
        case Mode::HID: {
            uint8_t mod{}, kc{};
            if (!read_hid_event(mod, kc)) return false;
            if (mod == KEY_EVENT_EMPTY && kc == KEY_EVENT_EMPTY) {
                evt.type = EventType::None;
                return true;
            }
            evt.type        = EventType::Hid;
            evt.modifier    = mod;
            evt.hid.keycode = kc;
            return true;
        }
        case Mode::Character: {
            uint8_t mod{};
            std::string chars;
            if (!read_char_event(mod, chars)) return false;
            if (chars.empty()) {
                evt.type = EventType::None;
                return true;
            }
            evt.type       = EventType::Character;
            evt.modifier   = mod;
            evt.chr.length = static_cast<uint8_t>(std::min<size_t>(chars.size(), CHAR_EVENT_MAX_CHARS));
            std::memcpy(evt.chr.chars, chars.data(), evt.chr.length);
            evt.chr.chars[evt.chr.length] = '\0';
            return true;
        }
    }
    return false;
}

}  // namespace unit
}  // namespace m5
