/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitCardKB2UART (UART mode)
  NOTE: CardKB2 must be in UART mode (Fn+Sym+2) before running this test.
  The device remembers its communication mode across power cycles.
  Connect CardKB2 to Port C (or Port A if Port C is unavailable).
  Press RST button on CardKB2 after mode switch.
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_CardKB2UART.hpp>
#include <unit/unit_CardKB2_defs.hpp>
#include <cstring>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::keyboard;

class TestCardKB2UART : public UARTComponentTestBase<UnitCardKB2UART> {
protected:
    virtual UnitCardKB2UART* get_instance() override
    {
        return new UnitCardKB2UART();
    }
    virtual HardwareSerial* init_serial() override
    {
        auto pin_num_rx = M5.getPin(m5::pin_name_t::port_c_rxd);
        auto pin_num_tx = M5.getPin(m5::pin_name_t::port_c_txd);
        if (pin_num_rx < 0 || pin_num_tx < 0) {
            M5_LOGW("PortC is not available, using PortA");
            Wire.end();
            pin_num_rx = M5.getPin(m5::pin_name_t::port_a_pin1);
            pin_num_tx = M5.getPin(m5::pin_name_t::port_a_pin2);
        }
        M5_LOGI("UART RX:%d TX:%d", pin_num_rx, pin_num_tx);

#if defined(CONFIG_IDF_TARGET_ESP32C6)
        auto& s = Serial1;
#elif SOC_UART_NUM > 2
        auto& s = Serial2;
#elif SOC_UART_NUM > 1
        auto& s = Serial1;
#else
#error "Not enough Serial"
#endif
        s.begin(115200, SERIAL_8N1, pin_num_rx, pin_num_tx);
        return &s;
    }
};

TEST_F(TestCardKB2UART, Basic)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_EQ(unit->getchar(), 0);
}

TEST_F(TestCardKB2UART, FirmwareVersion)
{
    SCOPED_TRACE(ustr);
    // UART mode cannot read firmware version
    uint8_t ver{};
    EXPECT_FALSE(unit->readFirmwareVersion(ver));
    EXPECT_EQ(ver, 0);
    EXPECT_EQ(unit->firmwareVersion(), 0);
}

TEST_F(TestCardKB2UART, ModeNotSupported)
{
    SCOPED_TRACE(ustr);
    Mode mode{};
    EXPECT_FALSE(unit->writeMode(Mode::M5UnitUnified));
    EXPECT_FALSE(unit->readMode(mode));
}

TEST_F(TestCardKB2UART, BitwiseInitialState)
{
    SCOPED_TRACE(ustr);
    EXPECT_EQ(unit->nowBits(), 0U);
    EXPECT_EQ(unit->pressedBits(), 0U);
    EXPECT_EQ(unit->releasedBits(), 0U);
    EXPECT_EQ(unit->holdingBits(), 0U);
    EXPECT_EQ(unit->repeatingBits(), 0U);
    EXPECT_FALSE(unit->isPressed());
}

TEST_F(TestCardKB2UART, Config)
{
    SCOPED_TRACE(ustr);

    auto cfg = unit->config();
    EXPECT_TRUE(cfg.start_periodic);
    EXPECT_EQ(cfg.interval, 10U);

    cfg.interval = 50;
    unit->config(cfg);
    auto cfg2 = unit->config();
    EXPECT_EQ(cfg2.interval, 50U);

    // Restore
    cfg.interval = 10;
    unit->config(cfg);
}

TEST_F(TestCardKB2UART, Update)
{
    SCOPED_TRACE(ustr);

    // Update with no key pressed
    Units.update();
    EXPECT_FALSE(unit->updated());
    EXPECT_EQ(unit->getchar(), 0);
    EXPECT_EQ(unit->nowBits(), 0U);
}

// Bidirectional: toKeyIndex(ch) and character_to_mode_bits(ch) must be consistent
// for every character across all modifier modes (normal, shift, sym, fn)
TEST_F(TestCardKB2UART, CharacterToKeyIndexRoundtrip)
{
    SCOPED_TRACE(ustr);

    for (int c = 1; c < 256; ++c) {
        char ch   = static_cast<char>(c);
        auto kidx = unit->toKeyIndex(ch);
        if (kidx == 0xFF) {
            continue;
        }
        EXPECT_LT(kidx, m5::unit::cardkb2::NUMBER_OF_KEYS)
            << "toKeyIndex(0x" << std::hex << c << ") returned out-of-range key index " << (int)kidx;

        auto mbits = m5::unit::cardkb2::character_to_mode_bits(ch);
        EXPECT_NE(mbits, 0) << "character_to_mode_bits(0x" << std::hex << c << ") returned 0 but toKeyIndex returned "
                            << (int)kidx;

        if (ch >= 'a' && ch <= 'z') {
            EXPECT_TRUE(mbits & 0x01) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing normal bit";
        }
        if (ch >= 'A' && ch <= 'Z') {
            EXPECT_TRUE(mbits & 0x02) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing shift bit";
        }
        if (std::strchr("!@#$%^&*(){}[]|\\~`?/<>=+_-;:\"'", ch) && !(mbits & 0x03)) {
            EXPECT_TRUE(mbits & 0x04) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing sym bit";
        }
    }

    for (int c = 128; c < 256; ++c) {
        auto kidx = unit->toKeyIndex(static_cast<char>(c));
        if (kidx == 0xFF) {
            continue;
        }
        EXPECT_LT(kidx, m5::unit::cardkb2::NUMBER_OF_KEYS)
            << "Fn char 0x" << std::hex << c << " mapped to out-of-range key index " << (int)kidx;
    }
}
