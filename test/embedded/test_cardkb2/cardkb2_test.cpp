/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitCardKB2 (I2C mode)
  NOTE: CardKB2 must be in I2C mode (Fn+Sym+1) before running this test.
  The device remembers its communication mode across power cycles.
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_CardKB2.hpp>
#include <unit/unit_CardKB2_defs.hpp>
#include <cstring>

using namespace m5::unit::googletest;
using namespace m5::unit;
using m5::unit::types::elapsed_time_t;

class TestCardKB2 : public I2CComponentTestBase<UnitCardKB2> {
protected:
    virtual UnitCardKB2* get_instance() override
    {
        auto ptr = new m5::unit::UnitCardKB2();
        return ptr;
    }
};

TEST_F(TestCardKB2, Basic)
{
    SCOPED_TRACE(ustr);

    // No key pressed initially
    EXPECT_EQ(unit->getchar(), 0);
    EXPECT_FALSE(unit->updated());
}

TEST_F(TestCardKB2, Firmware)
{
    SCOPED_TRACE(ustr);

    // Firmware version should be non-zero on CardKB2
    EXPECT_NE(unit->firmwareVersion(), 0);

    uint8_t ver{};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_EQ(ver, unit->firmwareVersion());
    EXPECT_NE(ver, 0);
}

TEST_F(TestCardKB2, Config)
{
    SCOPED_TRACE(ustr);

    auto cfg = unit->config();
    EXPECT_TRUE(cfg.start_periodic);
    EXPECT_EQ(cfg.interval, 10U);

    // Modify and verify
    cfg.interval = 50;
    unit->config(cfg);
    auto cfg2 = unit->config();
    EXPECT_EQ(cfg2.interval, 50U);

    // Restore
    cfg.interval = 10;
    unit->config(cfg);
}

TEST_F(TestCardKB2, Update)
{
    SCOPED_TRACE(ustr);

    // Update with no key pressed
    unit->update();
    EXPECT_FALSE(unit->updated());
    EXPECT_EQ(unit->getchar(), 0);

    // Force update
    unit->update(true);
    // Still no key pressed
    EXPECT_EQ(unit->getchar(), 0);
}

// Bidirectional: toKeyIndex(ch) and character_to_mode_bits(ch) must be consistent
// for every character across all modifier modes (normal, shift, sym, fn)
TEST_F(TestCardKB2, CharacterToKeyIndexRoundtrip)
{
    SCOPED_TRACE(ustr);

    // Every recognized character must have valid key index AND non-zero mode bits
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

        // Mode-specific consistency:
        // normal (bit0=0x01), shift (bit1=0x02), sym (bit2=0x04), fn (bit3=0x08)
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

    // Verify Fn characters: every Fn char must map to valid key index
    for (int c = 128; c < 256; ++c) {
        auto kidx = unit->toKeyIndex(static_cast<char>(c));
        if (kidx == 0xFF) {
            continue;
        }
        EXPECT_LT(kidx, m5::unit::cardkb2::NUMBER_OF_KEYS)
            << "Fn char 0x" << std::hex << c << " mapped to out-of-range key index " << (int)kidx;
    }
}
