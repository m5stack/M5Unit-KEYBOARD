/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitTab5Keyboard (Tab5 built-in keyboard, I2C ExtPort1)

  I2C address: datasheet default 0x6D (range 0x08..0x77 via REG_I2C_ADDRESS).

  Constraints kept in this file:
    * GoogleTest ASSERT_* is FORBIDDEN by project rules — only EXPECT_* is used.
    * C++11 baseline only (no designated initializers etc.).
    * No `k` prefix on constants; const correctness applied where practical.
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_Tab5Keyboard.hpp>
#include <utility/hid_keycode.hpp>
#include <string>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::tab5_keyboard;
using m5::unit::types::elapsed_time_t;

namespace {

// Default I2C address (datasheet: 0x6D, range 0x08..0x77 via REG_I2C_ADDRESS).
constexpr uint8_t DEFAULT_I2C_ADDRESS{0x6D};

}  // namespace

class TestTab5Keyboard : public I2CComponentTestBase<UnitTab5Keyboard> {
protected:
    virtual UnitTab5Keyboard* get_instance() override
    {
        auto ptr = new m5::unit::UnitTab5Keyboard();
        return ptr;
    }

    // Tab5 Keyboard is wired to ExtPort1 (J9), not the Grove port (port_a).
    // The default I2CComponentTestBase::begin() uses port_a_sda/scl (GPIO53/54),
    // which would fail to reach the keyboard. Override to use ExtPort1 pins.
    virtual bool begin() override
    {
        constexpr int8_t TAB5_EXTPORT1_SDA = 0;
        constexpr int8_t TAB5_EXTPORT1_SCL = 1;
        const uint32_t freq                = unit->component_config().clock;
        if (i2cIsInit(0)) {
            Wire.end();
        }
        M5_LOGI("Tab5 Keyboard test: Wire begin SDA:%d SCL:%d FREQ:%u", TAB5_EXTPORT1_SDA, TAB5_EXTPORT1_SCL, freq);
        Wire.begin(TAB5_EXTPORT1_SDA, TAB5_EXTPORT1_SCL, freq);
        return Units.add(*unit, Wire) && Units.begin();
    }

    // Helpers for inspecting UnitTab5Keyboard's private _data buffer.
    // TestTab5Keyboard is the named friend of UnitTab5Keyboard; expose the
    // information the TEST_F-generated derived classes need (friendship does
    // not transit through derivation, hence the indirection).
    bool data_allocated() const
    {
        return unit && unit->_data != nullptr;
    }
    size_t data_capacity() const
    {
        return (unit && unit->_data) ? unit->_data->capacity() : 0;
    }
};

// --------------------------------------------------------------------------
// 1. Construct: instance is created and reports the datasheet default I2C
//    address. Default config_t.irq_pin is 50 (Tab5 ExtPort1 J9 pin 10, ISR
//    mode). Default config_t.mode is Mode::Normal.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, Construct)
{
    SCOPED_TRACE(ustr);

    EXPECT_NE(unit.get(), nullptr);
    EXPECT_EQ(unit->address(), DEFAULT_I2C_ADDRESS);
    // config_t default: irq_pin = 50 (Tab5 ExtPort1 J9 pin 10, ISR mode).
    EXPECT_EQ(unit->config().irq_pin, 50);
    // config_t default: mode = Mode::Normal (matrix coordinate events).
    EXPECT_EQ(unit->config().mode, Mode::Normal);

    // Buffer pre-allocated at default size via component_config()
    EXPECT_TRUE(data_allocated());
    EXPECT_EQ(data_capacity(), tab5_keyboard::DEFAULT_STORED_SIZE);
}

// --------------------------------------------------------------------------
// 2. Begin: the I2CComponentTestBase fixture has already called begin() inside
//    SetUp(). If begin() had failed, SetUp() would have aborted with FAIL().
//    Re-validate the post-begin invariants explicitly.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, Begin)
{
    SCOPED_TRACE(ustr);

    // Hardware-prerequisite expectation: if SetUp() reached this point the
    // unit is online. Probe a basic property to confirm.
    uint8_t ver{};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    // TODO(hardware): on real hardware ver must be non-zero. Until the device
    // is available this may legitimately read 0 — promote to EXPECT_NE once
    // wired.
}

// --------------------------------------------------------------------------
// 3. FirmwareVersion: REG_FIRMWARE_VERSION (0xFE) returns a non-zero byte.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, FirmwareVersion)
{
    SCOPED_TRACE(ustr);

    uint8_t ver{};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    // TODO(hardware-confirm): expect non-zero firmware version on real device.
    EXPECT_NE(ver, 0);
}

// --------------------------------------------------------------------------
// 4. I2CAddressRead: REG_I2C_ADDRESS (0xFF) returns a value in the valid
//    range 0x08..0x77.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, I2CAddressRead)
{
    SCOPED_TRACE(ustr);

    uint8_t addr{};
    const bool ok = unit->readI2CAddress(addr);
    EXPECT_TRUE(ok);
    if (ok) {
        EXPECT_GE(addr, 0x08);
        EXPECT_LE(addr, 0x77);
        EXPECT_EQ(addr, DEFAULT_I2C_ADDRESS);
    }
}

// --------------------------------------------------------------------------
// 5. ChangeI2CAddress: out-of-range values rejected, in-range values applied
//    and round-tripped via readI2CAddress(). Restores default before exit.
//    Style reference: M5Unit-METER test_kmeterISO/kmeterISO_test.cpp.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, ChangeI2CAddress)
{
    SCOPED_TRACE(ustr);

    // Out-of-range: must be rejected.
    EXPECT_FALSE(unit->changeI2CAddress(0x07));
    EXPECT_FALSE(unit->changeI2CAddress(0x78));

    uint8_t addr{};

    // Change to 0x10 (lower bound of valid range).
    EXPECT_TRUE(unit->changeI2CAddress(0x10));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x10);
    EXPECT_EQ(unit->address(), 0x10);

    // Change to 0x77 (upper bound of valid range).
    EXPECT_TRUE(unit->changeI2CAddress(0x77));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, 0x77);
    EXPECT_EQ(unit->address(), 0x77);

    // Restore default (0x6D).
    EXPECT_TRUE(unit->changeI2CAddress(DEFAULT_I2C_ADDRESS));
    EXPECT_TRUE(unit->readI2CAddress(addr));
    EXPECT_EQ(addr, DEFAULT_I2C_ADDRESS);
    EXPECT_EQ(unit->address(), DEFAULT_I2C_ADDRESS);
}

// --------------------------------------------------------------------------
// 6. ConfigIrqPin: config_t round-trip via config() getter/setter.
//    Default is 50 (Tab5 ExtPort1 J9 pin 10, ISR mode).
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, ConfigIrqPin)
{
    SCOPED_TRACE(ustr);

    // Default: irq_pin == 50 (Tab5 ExtPort1 J9 pin 10, ISR mode).
    EXPECT_EQ(unit->config().irq_pin, 50);

    // Set to -1 (disable INT, fall back to unconditional polling). C++11
    // baseline, so no designated initializer — assign the field explicitly.
    UnitTab5Keyboard::config_t cfg;
    cfg.irq_pin = -1;
    unit->config(cfg);
    EXPECT_EQ(unit->config().irq_pin, -1);

    // Set to another valid GPIO (24) to confirm arbitrary pins are accepted.
    cfg.irq_pin = 24;
    unit->config(cfg);
    EXPECT_EQ(unit->config().irq_pin, 24);

    // Restore default (50) so subsequent tests are unaffected.
    cfg.irq_pin = 50;
    unit->config(cfg);
    EXPECT_EQ(unit->config().irq_pin, 50);
}

// --------------------------------------------------------------------------
// 7. ConfigMode: config_t.mode round-trip via config() getter/setter.
//    Default is Mode::Normal; begin() applies _cfg.mode via writeMode().
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, ConfigMode)
{
    SCOPED_TRACE(ustr);

    // Default: mode == Mode::Normal.
    EXPECT_EQ(unit->config().mode, Mode::Normal);

    UnitTab5Keyboard::config_t cfg;
    cfg.mode = Mode::HID;
    unit->config(cfg);
    EXPECT_EQ(unit->config().mode, Mode::HID);

    cfg.mode = Mode::Character;
    unit->config(cfg);
    EXPECT_EQ(unit->config().mode, Mode::Character);

    // Restore default (Mode::Normal) so subsequent tests are unaffected.
    cfg.mode = Mode::Normal;
    unit->config(cfg);
    EXPECT_EQ(unit->config().mode, Mode::Normal);
}

// --------------------------------------------------------------------------
// 7b. ConfigModeAppliedOnBegin: cfg.mode set via config() must actually be
//     written to REG_MODE_KEYBOARD by begin() (via writeMode(_cfg.mode)).
//     Verified by reading the mode back through readMode().
// --------------------------------------------------------------------------
// begin() applies cfg.mode to REG_MODE_KEYBOARD (via writeMode) and, when start_periodic is
// true, starts the periodic measurement. begin() is a once-per-lifecycle call (a second begin()
// would call startPeriodicMeasurement() while already running and fail), so the mode is injected
// via get_instance() and verified after the single begin() the fixture performs in SetUp().
struct BeginModeParams {
    Mode mode;
};

class TestTab5KeyboardBeginConfig : public TestTab5Keyboard, public ::testing::WithParamInterface<BeginModeParams> {
protected:
    virtual UnitTab5Keyboard* get_instance() override
    {
        auto ptr = new m5::unit::UnitTab5Keyboard();
        if (ptr) {
            auto cfg = ptr->config();
            cfg.mode = GetParam().mode;
            ptr->config(cfg);
        }
        return ptr;
    }
};

INSTANTIATE_TEST_SUITE_P(ConfigValues, TestTab5KeyboardBeginConfig,
                         ::testing::Values(BeginModeParams{Mode::Normal}, BeginModeParams{Mode::HID},
                                           BeginModeParams{Mode::Character}));

TEST_P(TestTab5KeyboardBeginConfig, BeginAppliesConfig)
{
    SCOPED_TRACE(ustr);

    // cfg.mode must reach REG_MODE_KEYBOARD via writeMode() during begin().
    Mode actual{};
    EXPECT_TRUE(unit->readMode(actual));
    EXPECT_EQ(actual, GetParam().mode);

    // start_periodic defaults to true, so begin() must have started the measurement.
    EXPECT_TRUE(unit->inPeriodic());
}

// --------------------------------------------------------------------------
// 8. ModeSwitch: Normal -> HID -> Character round-trips through readMode().
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, ModeSwitch)
{
    SCOPED_TRACE(ustr);

    const Mode modes[] = {
        Mode::Normal,
        Mode::HID,
        Mode::Character,
    };

    for (const auto target : modes) {
        EXPECT_TRUE(unit->writeMode(target));

        Mode current = Mode::Normal;
        EXPECT_TRUE(unit->readMode(current));
        EXPECT_EQ(static_cast<uint8_t>(current), static_cast<uint8_t>(target));
    }

    // Restore Normal mode as a courtesy for follow-up tests.
    (void)unit->writeMode(Mode::Normal);
}

// --------------------------------------------------------------------------
// 9. InterruptConfig: enabling only the Normal-mode INT bit must be visible
//    in REG_INT_STAT layout (bit 0 only).
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, InterruptConfig)
{
    SCOPED_TRACE(ustr);

    // Enable only "Normal" mode interrupts.
    EXPECT_TRUE(unit->writeInterruptEnable(true, false, false));

    // readInterruptStatus() reports per-mode pending bits. Since no key has
    // been pressed, all three "pending" flags should be false right after
    // configuring INT_CFG.
    // TODO(observe): on real hardware, press a key with each combination and
    // verify the corresponding bit asserts only when its mode is enabled.
    bool normal_pending{true};
    bool hid_pending{true};
    bool char_pending{true};
    EXPECT_TRUE(unit->readInterruptStatus(normal_pending, hid_pending, char_pending));
    // (readInterruptStatus reads REG_INT_STAT; readInterruptEnable reads REG_INT_CFG.)
    EXPECT_FALSE(normal_pending);
    EXPECT_FALSE(hid_pending);
    EXPECT_FALSE(char_pending);

    // Restore default (all enabled, register value 0x07).
    (void)unit->writeInterruptEnable(true, true, true);
}

// --------------------------------------------------------------------------
// 10. InterruptClear: clearInterrupt() (no arguments — writes 0x00 to
//     REG_INT_STAT in one shot) must complete without error.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, InterruptClear)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->clearInterrupt());
}

// --------------------------------------------------------------------------
// 11. RgbWrite: writing RGB1 and RGB2 in Custom mode must succeed.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, RgbWrite)
{
    SCOPED_TRACE(ustr);

    // Max brightness for visual confirmation
    EXPECT_TRUE(unit->writeBrightness(100));
    EXPECT_TRUE(unit->writeRgbMode(RgbMode::Custom));

    // Phase 1: LED1 only — bright red, 1 second
    EXPECT_TRUE(unit->writeRgb(0, 0xFF, 0x00, 0x00));
    EXPECT_TRUE(unit->writeRgb(1, 0x00, 0x00, 0x00));
    m5::utility::delay(1000);

    // Phase 2: LED2 only — bright green, 1 second
    EXPECT_TRUE(unit->writeRgb(0, 0x00, 0x00, 0x00));
    EXPECT_TRUE(unit->writeRgb(1, 0x00, 0xFF, 0x00));
    m5::utility::delay(1000);

    // Phase 3: Both LEDs — blue, 500 ms (sanity check both wired)
    EXPECT_TRUE(unit->writeRgb(0, 0x00, 0x00, 0xFF));
    EXPECT_TRUE(unit->writeRgb(1, 0x00, 0x00, 0xFF));
    m5::utility::delay(500);

    // Out-of-range LED index must fail without touching the bus.
    EXPECT_FALSE(unit->writeRgb(RGB_LED_COUNT, 0, 0, 0));

    // Restore Bind mode and dim brightness as a courtesy for follow-up tests.
    (void)unit->writeRgb(0, 0, 0, 0);
    (void)unit->writeRgb(1, 0, 0, 0);
    (void)unit->writeRgbMode(RgbMode::Bind);
    (void)unit->writeBrightness(20);
}

// --------------------------------------------------------------------------
// 12. Brightness: writeBrightness() must succeed for a mid-range value.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, Brightness)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeBrightness(50));
}

// --------------------------------------------------------------------------
// 13. PeriodicMeasurementLifecycle: start/stop round-trip via adapter API.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, PeriodicMeasurementLifecycle)
{
    SCOPED_TRACE(ustr);

    // begin() (via SetUp) already started the measurement when start_periodic is true.
    EXPECT_TRUE(unit->inPeriodic());

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_TRUE(unit->startPeriodicMeasurement());
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_TRUE(unit->empty());  // start_periodic_measurement should flush
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
}

// --------------------------------------------------------------------------
// 14. StoredSizeOverride: component_config().stored_size resizes _data on
//     begin().
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, StoredSizeOverride)
{
    SCOPED_TRACE(ustr);

    auto ccfg        = unit->component_config();
    ccfg.stored_size = 32;
    unit->component_config(ccfg);

    // begin() auto-starts periodic; stop first so the re-begin's startPeriodicMeasurement()
    // does not fail on an already-running measurement.
    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_TRUE(unit->begin());
    EXPECT_TRUE(data_allocated());
    EXPECT_EQ(data_capacity(), 32U);
}

// --------------------------------------------------------------------------
// 15. ModeFlush: writeMode() flushes the internal event buffer to mirror
//     the device's queue clear behavior.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, ModeFlush)
{
    SCOPED_TRACE(ustr);

    // Force a drain in current mode (Normal by default)
    EXPECT_TRUE(unit->writeMode(tab5_keyboard::Mode::Normal));
    unit->update(true);

    // Switching mode must empty the internal buffer regardless of contents
    EXPECT_TRUE(unit->writeMode(tab5_keyboard::Mode::HID));
    EXPECT_TRUE(unit->empty());

    EXPECT_TRUE(unit->writeMode(tab5_keyboard::Mode::Character));
    EXPECT_TRUE(unit->empty());

    // Restore Normal
    EXPECT_TRUE(unit->writeMode(tab5_keyboard::Mode::Normal));
}

// --------------------------------------------------------------------------
// 16. EventTypeRouting: per-mode forced update() puts events of correct
//     EventType into the buffer (hardware-dependent: requires key press).
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, EventTypeRouting)
{
    SCOPED_TRACE(ustr);

    struct ModeCase {
        tab5_keyboard::Mode mode;
        tab5_keyboard::EventType expected_type;
    };
    const ModeCase cases[] = {
        {tab5_keyboard::Mode::Normal, tab5_keyboard::EventType::Key},
        {tab5_keyboard::Mode::HID, tab5_keyboard::EventType::Hid},
        {tab5_keyboard::Mode::Character, tab5_keyboard::EventType::Character},
    };

    for (const auto& c : cases) {
        EXPECT_TRUE(unit->writeMode(c.mode));
        unit->update(true);
        // TODO(hardware): without a physical key press the buffer stays empty.
        // When a key is pressed during the test window, type must match.
        if (!unit->empty()) {
            EXPECT_EQ(unit->oldest().type, c.expected_type);
            unit->discard();
        }
    }
}

// --------------------------------------------------------------------------
// 17. RgbModeRoundTrip: writeRgbMode(Bind/Custom) → readRgbMode round-trips.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, RgbModeRoundTrip)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeRgbMode(RgbMode::Custom));
    RgbMode current = RgbMode::Bind;
    EXPECT_TRUE(unit->readRgbMode(current));
    EXPECT_EQ(static_cast<uint8_t>(current), static_cast<uint8_t>(RgbMode::Custom));

    EXPECT_TRUE(unit->writeRgbMode(RgbMode::Bind));
    EXPECT_TRUE(unit->readRgbMode(current));
    EXPECT_EQ(static_cast<uint8_t>(current), static_cast<uint8_t>(RgbMode::Bind));
}

// --------------------------------------------------------------------------
// 18. InterruptEnableRoundTrip: writeInterruptEnable → readInterruptEnable
//     reflects same bit pattern for each enable combination.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, InterruptEnableRoundTrip)
{
    SCOPED_TRACE(ustr);

    struct Combo {
        bool normal;
        bool hid;
        bool character;
    };
    const Combo combos[] = {
        {true, true, true}, {true, false, false}, {false, true, false}, {false, false, true}, {false, false, false},
    };
    for (const auto& c : combos) {
        EXPECT_TRUE(unit->writeInterruptEnable(c.normal, c.hid, c.character));
        bool n = !c.normal, h = !c.hid, ch = !c.character;
        EXPECT_TRUE(unit->readInterruptEnable(n, h, ch));
        EXPECT_EQ(n, c.normal);
        EXPECT_EQ(h, c.hid);
        EXPECT_EQ(ch, c.character);
    }
    // Restore default (all enabled per datasheet 0x07).
    EXPECT_TRUE(unit->writeInterruptEnable(true, true, true));
}

// --------------------------------------------------------------------------
// 19. BrightnessRoundTrip: writeBrightness → readBrightness round-trips,
//     out-of-range write rejected.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, BrightnessRoundTrip)
{
    SCOPED_TRACE(ustr);

    const uint8_t cases[] = {0, 1, 20, 50, 99, 100};
    for (const auto pct : cases) {
        EXPECT_TRUE(unit->writeBrightness(pct));
        uint8_t got{};
        EXPECT_TRUE(unit->readBrightness(got));
        EXPECT_EQ(got, pct);
    }
    // Out-of-range rejected.
    EXPECT_FALSE(unit->writeBrightness(101));
    EXPECT_FALSE(unit->writeBrightness(200));
    // Restore datasheet default (20).
    EXPECT_TRUE(unit->writeBrightness(20));
}

// --------------------------------------------------------------------------
// 20. RgbRoundTrip: writeRgb → readRgb round-trips for both LEDs.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, RgbRoundTrip)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeRgbMode(RgbMode::Custom));

    struct RgbCase {
        uint8_t idx;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    const RgbCase cases[] = {
        {0, 0x11, 0x22, 0x33},
        {1, 0x44, 0x55, 0x66},
        {0, 0xFF, 0x00, 0x00},
        {1, 0x00, 0xFF, 0x00},
    };
    for (const auto& c : cases) {
        EXPECT_TRUE(unit->writeRgb(c.idx, c.r, c.g, c.b));
        uint8_t r{}, g{}, b{};
        EXPECT_TRUE(unit->readRgb(c.idx, r, g, b));
        EXPECT_EQ(r, c.r);
        EXPECT_EQ(g, c.g);
        EXPECT_EQ(b, c.b);
    }

    // Out-of-range idx rejected.
    uint8_t r{}, g{}, b{};
    EXPECT_FALSE(unit->readRgb(RGB_LED_COUNT, r, g, b));

    // Restore Bind mode.
    (void)unit->writeRgb(0, 0, 0, 0);
    (void)unit->writeRgb(1, 0, 0, 0);
    (void)unit->writeRgbMode(RgbMode::Bind);
}

// --------------------------------------------------------------------------
// 21. EventCountRead: after flush() in writeMode, readEventCount() == 0
//     regardless of mode.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, EventCountRead)
{
    SCOPED_TRACE(ustr);

    const Mode modes[] = {Mode::Normal, Mode::HID, Mode::Character};
    for (const auto m : modes) {
        EXPECT_TRUE(unit->writeMode(m));
        uint8_t count = 0xFF;
        EXPECT_TRUE(unit->readEventCount(count));
        EXPECT_EQ(count, 0U);
    }
    // Restore Normal.
    EXPECT_TRUE(unit->writeMode(Mode::Normal));
}

// --------------------------------------------------------------------------
// 22. ClearEventQueue: clearEventQueue() empties device + internal buffer.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, ClearEventQueue)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(Mode::Normal));
    EXPECT_TRUE(unit->clearEventQueue());

    uint8_t count = 0xFF;
    EXPECT_TRUE(unit->readEventCount(count));
    EXPECT_EQ(count, 0U);

    EXPECT_TRUE(unit->empty());
}

// --------------------------------------------------------------------------
// 23. HidKeycodeAlphabet: 0x04..0x1D unshifted = a..z, shifted = A..Z.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, HidKeycodeAlphabet)
{
    SCOPED_TRACE(ustr);

    EXPECT_EQ(hidUsageToChar(0x04, 0x00), 'a');
    EXPECT_EQ(hidUsageToChar(0x04, 0x02), 'A');  // L-Shift
    EXPECT_EQ(hidUsageToChar(0x04, 0x20), 'A');  // R-Shift
    EXPECT_EQ(hidUsageToChar(0x1D, 0x00), 'z');
    EXPECT_EQ(hidUsageToChar(0x1D, 0x02), 'Z');
    EXPECT_EQ(hidUsageToChar(0x11, 0x00), 'n');
    EXPECT_EQ(hidUsageToChar(0x11, 0x02), 'N');
}

// --------------------------------------------------------------------------
// 24. HidKeycodeNumber: 0x1E..0x27 unshifted = 1..0, shifted = !@#$%^&*().
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, HidKeycodeNumber)
{
    SCOPED_TRACE(ustr);

    EXPECT_EQ(hidUsageToChar(0x1E, 0x00), '1');
    EXPECT_EQ(hidUsageToChar(0x1E, 0x02), '!');
    EXPECT_EQ(hidUsageToChar(0x21, 0x00), '4');
    EXPECT_EQ(hidUsageToChar(0x21, 0x02), '$');
    EXPECT_EQ(hidUsageToChar(0x27, 0x00), '0');
    EXPECT_EQ(hidUsageToChar(0x27, 0x02), ')');
}

// --------------------------------------------------------------------------
// 25. HidKeycodeSymbol: punctuation keys (- = [ ] \ ; ' ` , . /).
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, HidKeycodeSymbol)
{
    SCOPED_TRACE(ustr);

    EXPECT_EQ(hidUsageToChar(0x2D, 0x00), '-');
    EXPECT_EQ(hidUsageToChar(0x2D, 0x02), '_');
    EXPECT_EQ(hidUsageToChar(0x2E, 0x00), '=');
    EXPECT_EQ(hidUsageToChar(0x2E, 0x02), '+');
    EXPECT_EQ(hidUsageToChar(0x33, 0x00), ';');
    EXPECT_EQ(hidUsageToChar(0x33, 0x02), ':');
    EXPECT_EQ(hidUsageToChar(0x34, 0x00), '\'');
    EXPECT_EQ(hidUsageToChar(0x34, 0x02), '"');
    EXPECT_EQ(hidUsageToChar(0x36, 0x00), ',');
    EXPECT_EQ(hidUsageToChar(0x36, 0x02), '<');
    EXPECT_EQ(hidUsageToChar(0x38, 0x00), '/');
    EXPECT_EQ(hidUsageToChar(0x38, 0x02), '?');
}

// --------------------------------------------------------------------------
// 26. HidKeycodeWhitespace: Enter / Backspace / Tab / Space.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, HidKeycodeWhitespace)
{
    SCOPED_TRACE(ustr);

    EXPECT_EQ(hidUsageToChar(0x28, 0x00), '\n');  // Enter
    EXPECT_EQ(hidUsageToChar(0x2A, 0x00), '\b');  // Backspace
    EXPECT_EQ(hidUsageToChar(0x2B, 0x00), '\t');  // Tab
    EXPECT_EQ(hidUsageToChar(0x2C, 0x00), ' ');   // Space
}

// --------------------------------------------------------------------------
// 27. HidKeycodeNonPrintable: F-keys, arrows, modifiers → 0.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, HidKeycodeNonPrintable)
{
    SCOPED_TRACE(ustr);

    EXPECT_EQ(hidUsageToChar(0x29, 0x00), 0);  // Esc
    EXPECT_EQ(hidUsageToChar(0x39, 0x00), 0);  // CapsLock
    EXPECT_EQ(hidUsageToChar(0x3A, 0x00), 0);  // F1
    EXPECT_EQ(hidUsageToChar(0x45, 0x00), 0);  // F12
    EXPECT_EQ(hidUsageToChar(0x4F, 0x00), 0);  // Right Arrow
    EXPECT_EQ(hidUsageToChar(0x50, 0x00), 0);  // Left Arrow
    EXPECT_EQ(hidUsageToChar(0x51, 0x00), 0);  // Down Arrow
    EXPECT_EQ(hidUsageToChar(0x52, 0x00), 0);  // Up Arrow
}

// --------------------------------------------------------------------------
// 28. HidKeycodeOutOfRange: keycode >= 128 → 0.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, HidKeycodeOutOfRange)
{
    SCOPED_TRACE(ustr);

    EXPECT_EQ(hidUsageToChar(0x80, 0x00), 0);
    EXPECT_EQ(hidUsageToChar(0xFF, 0x00), 0);
    EXPECT_EQ(hidUsageToChar(0xFF, 0x02), 0);
}

// --------------------------------------------------------------------------
// 29. HidKeycodeIsPrintable: helper returns true for printable, false otherwise.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, HidKeycodeIsPrintable)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(isPrintableHidKey(0x04, 0x00));   // 'a'
    EXPECT_TRUE(isPrintableHidKey(0x1E, 0x02));   // '!'
    EXPECT_TRUE(isPrintableHidKey(0x2C, 0x00));   // ' ' is printable per isprint
    EXPECT_FALSE(isPrintableHidKey(0x28, 0x00));  // '\n' not printable
    EXPECT_FALSE(isPrintableHidKey(0x2A, 0x00));  // '\b' not printable
    EXPECT_FALSE(isPrintableHidKey(0x3A, 0x00));  // F1 → 0 → not printable
    EXPECT_FALSE(isPrintableHidKey(0xFF, 0x00));  // out of range
}

// --------------------------------------------------------------------------
// ConfigSoftwareRepeat: config_t round-trip for the three software-repeat
// fields. Software repeat is Normal-mode only; this test only verifies the
// getter/setter round-trip (no hardware timing dependency).
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, ConfigSoftwareRepeat)
{
    SCOPED_TRACE(ustr);

    // Defaults: software_repeat = false, repeat_initial_ms = 400, repeat_rate_ms = 80,
    // interval_ms = 50, start_periodic = true.
    EXPECT_FALSE(unit->config().software_repeat);
    EXPECT_EQ(unit->config().repeat_initial_ms, 400U);
    EXPECT_EQ(unit->config().repeat_rate_ms, 80U);
    EXPECT_EQ(unit->config().interval_ms, 50U);
    EXPECT_TRUE(unit->config().start_periodic);

    // Enable + override timing (including the polling interval).
    UnitTab5Keyboard::config_t cfg;
    cfg.software_repeat   = true;
    cfg.repeat_initial_ms = 250;
    cfg.repeat_rate_ms    = 33;
    cfg.interval_ms       = 20;
    unit->config(cfg);
    EXPECT_TRUE(unit->config().software_repeat);
    EXPECT_EQ(unit->config().repeat_initial_ms, 250U);
    EXPECT_EQ(unit->config().repeat_rate_ms, 33U);
    EXPECT_EQ(unit->config().interval_ms, 20U);

    // Restore defaults so subsequent tests are unaffected.
    cfg.software_repeat   = false;
    cfg.repeat_initial_ms = 400;
    cfg.repeat_rate_ms    = 80;
    cfg.interval_ms       = 50;
    unit->config(cfg);
    EXPECT_FALSE(unit->config().software_repeat);
    EXPECT_EQ(unit->config().repeat_initial_ms, 400U);
    EXPECT_EQ(unit->config().repeat_rate_ms, 80U);
    EXPECT_EQ(unit->config().interval_ms, 50U);
}

// --------------------------------------------------------------------------
// EventRepeatFlagDefault: Event.repeat defaults to false; hardware-originated
// events read back via update() must also leave it false. Time-based repeat
// synthesis is hardware-timer dependent and is intentionally NOT covered here.
// TODO(hardware): add a timed test that holds a key and asserts at least one
//                 repeat event with evt.repeat == true is observed.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, EventRepeatFlagDefault)
{
    SCOPED_TRACE(ustr);

    Event e;
    EXPECT_FALSE(e.repeat);
}

TEST_F(TestTab5Keyboard, IsModifierKey)
{
    SCOPED_TRACE(ustr);

    // Sym (3, 0), Aa (3, 1), Ctrl (4, 0), Alt (4, 1) — hardware-confirmed.
    EXPECT_TRUE(isModifierKey(3, 0));
    EXPECT_TRUE(isModifierKey(3, 1));
    EXPECT_TRUE(isModifierKey(4, 0));
    EXPECT_TRUE(isModifierKey(4, 1));

    // Adjacent positions must NOT be considered modifiers.
    EXPECT_FALSE(isModifierKey(0, 0));
    EXPECT_FALSE(isModifierKey(2, 0));
    EXPECT_FALSE(isModifierKey(2, 1));
    EXPECT_FALSE(isModifierKey(3, 2));
    EXPECT_FALSE(isModifierKey(4, 2));
    EXPECT_FALSE(isModifierKey(3, 13));
    EXPECT_FALSE(isModifierKey(4, 13));
}

TEST_F(TestTab5Keyboard, EventModifierFlags)
{
    SCOPED_TRACE(ustr);

    // No modifier => all helpers false
    {
        Event e;
        e.modifier = 0x00;
        EXPECT_FALSE(e.isCtrl());
        EXPECT_FALSE(e.isShift());
        EXPECT_FALSE(e.isAlt());
    }
    // HID LCtrl (0x01) — also matches Character-mode Ctrl encoding
    {
        Event e;
        e.modifier = 0x01;
        EXPECT_TRUE(e.isCtrl());
        EXPECT_FALSE(e.isShift());
        EXPECT_FALSE(e.isAlt());
    }
    // HID LShift (0x02) — Tab5 Sym/Aa map to this in HID mode
    {
        Event e;
        e.modifier = 0x02;
        EXPECT_FALSE(e.isCtrl());
        EXPECT_TRUE(e.isShift());
        EXPECT_FALSE(e.isAlt());
    }
    // HID LAlt (0x04) — also matches Character-mode Alt encoding
    {
        Event e;
        e.modifier = 0x04;
        EXPECT_FALSE(e.isCtrl());
        EXPECT_FALSE(e.isShift());
        EXPECT_TRUE(e.isAlt());
    }
    // HID RCtrl (0x10)
    {
        Event e;
        e.modifier = 0x10;
        EXPECT_TRUE(e.isCtrl());
        EXPECT_FALSE(e.isShift());
        EXPECT_FALSE(e.isAlt());
    }
    // HID RShift (0x20)
    {
        Event e;
        e.modifier = 0x20;
        EXPECT_FALSE(e.isCtrl());
        EXPECT_TRUE(e.isShift());
        EXPECT_FALSE(e.isAlt());
    }
    // HID RAlt (0x40)
    {
        Event e;
        e.modifier = 0x40;
        EXPECT_FALSE(e.isCtrl());
        EXPECT_FALSE(e.isShift());
        EXPECT_TRUE(e.isAlt());
    }
    // Combined: LCtrl + LShift + LAlt
    {
        Event e;
        e.modifier = 0x07;
        EXPECT_TRUE(e.isCtrl());
        EXPECT_TRUE(e.isShift());
        EXPECT_TRUE(e.isAlt());
    }
    // GUI bits (0x08, 0x80) are ignored — Tab5 has no GUI key
    {
        Event e;
        e.modifier = 0x88;
        EXPECT_FALSE(e.isCtrl());
        EXPECT_FALSE(e.isShift());
        EXPECT_FALSE(e.isAlt());
    }
}

// --------------------------------------------------------------------------
// BitwiseDefaultEmpty: after begin() (and the implicit writeMode(Normal) it
// performs), every bitset accessor must report an empty set.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, BitwiseDefaultEmpty)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->nowBits().none());
    EXPECT_TRUE(unit->previousBits().none());
    EXPECT_TRUE(unit->pressedBits().none());
    EXPECT_TRUE(unit->releasedBits().none());
    EXPECT_TRUE(unit->holdingBits().none());
    EXPECT_TRUE(unit->wasHoldBits().none());
    EXPECT_TRUE(unit->repeatingBits().none());
}

// --------------------------------------------------------------------------
// ToKeyIndex: (row, col) -> flat key index for the four modifier keys and a
// couple of corners, plus matching KIDX_* compile-time constants.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, ToKeyIndex)
{
    SCOPED_TRACE(ustr);

    // Modifier kidx constants.
    EXPECT_EQ(KIDX_SYM, 42U);
    EXPECT_EQ(KIDX_AA, 43U);
    EXPECT_EQ(KIDX_CTRL, 56U);
    EXPECT_EQ(KIDX_ALT, 57U);

    // toKeyIndex() must agree with the constants.
    EXPECT_EQ(toKeyIndex(3, 0), KIDX_SYM);
    EXPECT_EQ(toKeyIndex(3, 1), KIDX_AA);
    EXPECT_EQ(toKeyIndex(4, 0), KIDX_CTRL);
    EXPECT_EQ(toKeyIndex(4, 1), KIDX_ALT);

    // Matrix corners.
    EXPECT_EQ(toKeyIndex(0, 0), 0U);
    EXPECT_EQ(toKeyIndex(0, 13), 13U);
    EXPECT_EQ(toKeyIndex(4, 13), 69U);
    EXPECT_EQ(KEY_COUNT, 70U);
}

// --------------------------------------------------------------------------
// BitwiseAnyKeyHelpers: any-key state queries must all return false when no
// key has been pressed yet.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, BitwiseAnyKeyHelpers)
{
    SCOPED_TRACE(ustr);

    EXPECT_FALSE(unit->isPressed());
    EXPECT_FALSE(unit->wasPressed());
    EXPECT_FALSE(unit->wasReleased());
    EXPECT_FALSE(unit->isHolding());
    EXPECT_FALSE(unit->wasHold());
    EXPECT_FALSE(unit->isRepeating());

    // Modifier shortcuts: all false until the corresponding key is pressed.
    EXPECT_FALSE(unit->isSym());
    EXPECT_FALSE(unit->isAa());
    EXPECT_FALSE(unit->isCtrl());
    EXPECT_FALSE(unit->isAlt());

    // TODO(hardware-timer): add timed tests that hold a non-modifier key and
    // verify wasPressed() → isHolding() / wasHold() / isRepeating() transitions
    // against config.holding_threshold_ms / repeat_initial_ms / repeat_rate_ms.
}

// --------------------------------------------------------------------------
// KidxOutOfRange: per-key queries must return false (no UB) when kidx is
// outside [0, KEY_COUNT).
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, KidxOutOfRange)
{
    SCOPED_TRACE(ustr);

    const uint8_t out_of_range[] = {KEY_COUNT, static_cast<uint8_t>(KEY_COUNT + 1U), 100U, 200U, 255U};
    for (const auto kidx : out_of_range) {
        EXPECT_FALSE(unit->isPressed(kidx));
        EXPECT_FALSE(unit->wasPressed(kidx));
        EXPECT_FALSE(unit->wasReleased(kidx));
        EXPECT_FALSE(unit->isHolding(kidx));
        EXPECT_FALSE(unit->wasHold(kidx));
        EXPECT_FALSE(unit->isRepeating(kidx));
    }
}

// --------------------------------------------------------------------------
// KeyMatrixToHidBase: representative base-layer (no Sym held) mappings for
// letters, digits, whitespace and shifted symbols.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, KeyMatrixToHidBase)
{
    SCOPED_TRACE(ustr);

    // (0, 1) '1' → keycode 0x1E, no modifier
    {
        const HidMapping m = keyMatrixToHidBase(0, 1);
        EXPECT_EQ(m.keycode, 0x1E);
        EXPECT_EQ(m.modifier, 0x00);
    }
    // (2, 1) 'q' → keycode 0x14, no modifier
    {
        const HidMapping m = keyMatrixToHidBase(2, 1);
        EXPECT_EQ(m.keycode, 0x14);
        EXPECT_EQ(m.modifier, 0x00);
    }
    // (3, 2) 'a' → keycode 0x04, no modifier
    {
        const HidMapping m = keyMatrixToHidBase(3, 2);
        EXPECT_EQ(m.keycode, 0x04);
        EXPECT_EQ(m.modifier, 0x00);
    }
    // (4, 4) 'c' → keycode 0x06, no modifier
    {
        const HidMapping m = keyMatrixToHidBase(4, 4);
        EXPECT_EQ(m.keycode, 0x06);
        EXPECT_EQ(m.modifier, 0x00);
    }
    // (4, 13) Space → keycode 0x2C, no modifier
    {
        const HidMapping m = keyMatrixToHidBase(4, 13);
        EXPECT_EQ(m.keycode, 0x2C);
        EXPECT_EQ(m.modifier, 0x00);
    }
    // (1, 0) '`' → keycode 0x35, no modifier
    {
        const HidMapping m = keyMatrixToHidBase(1, 0);
        EXPECT_EQ(m.keycode, 0x35);
        EXPECT_EQ(m.modifier, 0x00);
    }
    // (0, 12) '+' → keycode 0x2E with forced Shift (0x02)
    {
        const HidMapping m = keyMatrixToHidBase(0, 12);
        EXPECT_EQ(m.keycode, 0x2E);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (3, 12) '_' → keycode 0x2D with forced Shift (0x02)
    {
        const HidMapping m = keyMatrixToHidBase(3, 12);
        EXPECT_EQ(m.keycode, 0x2D);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (3, 13) Enter → keycode 0x28, no modifier
    {
        const HidMapping m = keyMatrixToHidBase(3, 13);
        EXPECT_EQ(m.keycode, 0x28);
        EXPECT_EQ(m.modifier, 0x00);
    }
}

// --------------------------------------------------------------------------
// KeyMatrixToHidSym: 12 Sym-variant keys whose mapping differs from base.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, KeyMatrixToHidSym)
{
    SCOPED_TRACE(ustr);

    // (1, 0) ` → ~ : keycode 0x35, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(1, 0);
        EXPECT_EQ(m.keycode, 0x35);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (1, 1) ! → ? : keycode 0x38, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(1, 1);
        EXPECT_EQ(m.keycode, 0x38);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (1, 8) * → / : keycode 0x38, modifier 0x00 (shift dropped)
    {
        const HidMapping m = keyMatrixToHidSym(1, 8);
        EXPECT_EQ(m.keycode, 0x38);
        EXPECT_EQ(m.modifier, 0x00);
    }
    // (1, 9) ( → < : keycode 0x36, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(1, 9);
        EXPECT_EQ(m.keycode, 0x36);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (1, 10) ) → > : keycode 0x37, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(1, 10);
        EXPECT_EQ(m.keycode, 0x37);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (1, 11) [ → { : keycode 0x2F, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(1, 11);
        EXPECT_EQ(m.keycode, 0x2F);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (1, 12) ] → } : keycode 0x30, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(1, 12);
        EXPECT_EQ(m.keycode, 0x30);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (1, 13) \ → | : keycode 0x31, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(1, 13);
        EXPECT_EQ(m.keycode, 0x31);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (2, 11) ; → : : keycode 0x33, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(2, 11);
        EXPECT_EQ(m.keycode, 0x33);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (2, 12) ' → " : keycode 0x34, modifier 0x02
    {
        const HidMapping m = keyMatrixToHidSym(2, 12);
        EXPECT_EQ(m.keycode, 0x34);
        EXPECT_EQ(m.modifier, 0x02);
    }
    // (3, 12) _ → = : keycode 0x2E, modifier 0x00 (shift dropped)
    {
        const HidMapping m = keyMatrixToHidSym(3, 12);
        EXPECT_EQ(m.keycode, 0x2E);
        EXPECT_EQ(m.modifier, 0x00);
    }
    // (4, 9) . → , : keycode 0x36, modifier 0x00
    {
        const HidMapping m = keyMatrixToHidSym(4, 9);
        EXPECT_EQ(m.keycode, 0x36);
        EXPECT_EQ(m.modifier, 0x00);
    }
}

// --------------------------------------------------------------------------
// KeyMatrixToHidModifierKey: the four modifier slots (Sym/Aa/Ctrl/Alt) must
// resolve to {0, 0} in both base and Sym tables.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, KeyMatrixToHidModifierKey)
{
    SCOPED_TRACE(ustr);

    const uint8_t modifier_positions[][2] = {
        {3, 0},  // Sym
        {3, 1},  // Aa
        {4, 0},  // Ctrl
        {4, 1},  // Alt
    };
    for (const auto& rc : modifier_positions) {
        const HidMapping b = keyMatrixToHidBase(rc[0], rc[1]);
        EXPECT_EQ(b.keycode, 0x00);
        EXPECT_EQ(b.modifier, 0x00);

        const HidMapping s = keyMatrixToHidSym(rc[0], rc[1]);
        EXPECT_EQ(s.keycode, 0x00);
        EXPECT_EQ(s.modifier, 0x00);
    }
}

// --------------------------------------------------------------------------
// KeyMatrixToHidOutOfRange: invalid (row, col) coordinates must return
// {0, 0} from both lookup tables.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, KeyMatrixToHidOutOfRange)
{
    SCOPED_TRACE(ustr);

    const uint8_t bad_positions[][2] = {
        {5, 0},      // row out of range
        {0, 14},     // col out of range
        {255, 255},  // both out of range
    };
    for (const auto& rc : bad_positions) {
        const HidMapping b = keyMatrixToHidBase(rc[0], rc[1]);
        EXPECT_EQ(b.keycode, 0x00);
        EXPECT_EQ(b.modifier, 0x00);

        const HidMapping s = keyMatrixToHidSym(rc[0], rc[1]);
        EXPECT_EQ(s.keycode, 0x00);
        EXPECT_EQ(s.modifier, 0x00);
    }
}

// --------------------------------------------------------------------------
// KeyMatrixToChar: UnitTab5Keyboard::keyMatrixToChar() with the bitwise
// modifier state empty (Sym/Aa not held) returns the base-layer ASCII char
// for representative keys. Out-of-range and modifier slots return 0.
// --------------------------------------------------------------------------
TEST_F(TestTab5Keyboard, KeyMatrixToChar)
{
    SCOPED_TRACE(ustr);

    // No key has been pressed yet, so _now is empty → isSym()/isAa() == false.
    EXPECT_FALSE(unit->isSym());
    EXPECT_FALSE(unit->isAa());

    // Letters / digits / whitespace.
    EXPECT_EQ(unit->keyMatrixToChar(3, 2), 'a');
    EXPECT_EQ(unit->keyMatrixToChar(0, 1), '1');
    EXPECT_EQ(unit->keyMatrixToChar(4, 13), ' ');
    EXPECT_EQ(unit->keyMatrixToChar(1, 0), '`');
    EXPECT_EQ(unit->keyMatrixToChar(0, 12), '+');
    EXPECT_EQ(unit->keyMatrixToChar(3, 12), '_');
    EXPECT_EQ(unit->keyMatrixToChar(2, 1), 'q');
    EXPECT_EQ(unit->keyMatrixToChar(4, 4), 'c');
    EXPECT_EQ(unit->keyMatrixToChar(3, 13), '\n');  // Enter → '\n'

    // Flat-kidx overload agrees with the (row, col) overload.
    EXPECT_EQ(unit->keyMatrixToChar(toKeyIndex(3, 2)), 'a');
    EXPECT_EQ(unit->keyMatrixToChar(toKeyIndex(0, 1)), '1');

    // Modifier slots → 0.
    EXPECT_EQ(unit->keyMatrixToChar(3, 0), 0);  // Sym
    EXPECT_EQ(unit->keyMatrixToChar(3, 1), 0);  // Aa
    EXPECT_EQ(unit->keyMatrixToChar(4, 0), 0);  // Ctrl
    EXPECT_EQ(unit->keyMatrixToChar(4, 1), 0);  // Alt

    // Out-of-range → 0.
    EXPECT_EQ(unit->keyMatrixToChar(5, 0), 0);
    EXPECT_EQ(unit->keyMatrixToChar(0, 14), 0);
    EXPECT_EQ(unit->keyMatrixToChar(static_cast<uint8_t>(255), static_cast<uint8_t>(255)), 0);
    EXPECT_EQ(unit->keyMatrixToChar(KEY_COUNT), 0);
    EXPECT_EQ(unit->keyMatrixToChar(static_cast<uint8_t>(255)), 0);

    // Non-printable HID keycodes (arrows, Esc, Del, Tab, BS) map to 0 because
    // hidUsageToChar() has no translation for them in the US-ANSI table.
    EXPECT_EQ(unit->keyMatrixToChar(0, 0), 0);   // Esc (0x29)
    EXPECT_EQ(unit->keyMatrixToChar(0, 13), 0);  // Del (0x4C)
    EXPECT_EQ(unit->keyMatrixToChar(3, 11), 0);  // ↑   (0x52)
    EXPECT_EQ(unit->keyMatrixToChar(4, 10), 0);  // ←   (0x50)
    // Tab and Backspace ARE mapped (to '\t' and '\b') even though they are
    // non-printable per std::isprint. keyMatrixToChar() only returns 0 when
    // hidUsageToChar() has no entry at all.
    EXPECT_EQ(unit->keyMatrixToChar(2, 0), '\t');
    EXPECT_EQ(unit->keyMatrixToChar(2, 13), '\b');
}
