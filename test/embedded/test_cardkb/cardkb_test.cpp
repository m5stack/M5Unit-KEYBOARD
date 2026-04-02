/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitCardKB
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_CardKB.hpp>
#include <cstring>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::keyboard;
using namespace m5::unit::cardkb;
using m5::unit::types::elapsed_time_t;

class TestCardKB : public I2CComponentTestBase<UnitCardKB> {
protected:
    virtual UnitCardKB* get_instance() override
    {
        auto ptr = new m5::unit::UnitCardKB();
        return ptr;
    }
};

namespace {

constexpr Mode mode_table[] = {Mode::Conventional, Mode::M5UnitUnified};

}  // namespace

TEST_F(TestCardKB, Config)
{
    SCOPED_TRACE(ustr);

    auto cfg = unit->config();
    EXPECT_TRUE(cfg.start_periodic);
    EXPECT_EQ(cfg.interval, 10U);
    EXPECT_EQ(cfg.mode, keyboard::Mode::Conventional);

    cfg.interval = 50;
    unit->config(cfg);
    auto cfg2 = unit->config();
    EXPECT_EQ(cfg2.interval, 50U);

    // Restore
    cfg.interval = 10;
    unit->config(cfg);
}

TEST_F(TestCardKB, BitwiseInitialState)
{
    SCOPED_TRACE(ustr);
    EXPECT_EQ(unit->nowBits(), 0U);
    EXPECT_EQ(unit->pressedBits(), 0U);
    EXPECT_EQ(unit->releasedBits(), 0U);
    EXPECT_EQ(unit->holdingBits(), 0U);
    EXPECT_EQ(unit->repeatingBits(), 0U);
    EXPECT_FALSE(unit->isPressed());
}

TEST_F(TestCardKB, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());

    EXPECT_TRUE(unit->startPeriodicMeasurement(1000));
    EXPECT_TRUE(unit->inPeriodic());

    EXPECT_EQ(unit->pressed(), 0U);
    EXPECT_EQ(unit->released(), 0U);
    EXPECT_EQ(unit->getchar(), 0);
}

TEST_F(TestCardKB, M5UnitUnifiedFirmware)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());

    if (!unit->firmwareVersion()) {
        M5_LOGI("CardKB firmware is conventional");
        return;
    }

    uint8_t ver{};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_EQ(ver, unit->firmwareVersion());
    EXPECT_NE(ver, 0);

    uint8_t htype{};
    EXPECT_TRUE(unit->readHardwareType(htype));
    EXPECT_EQ(htype, unit->hardwareType());
    EXPECT_TRUE(htype == TYPE_CARDKB || htype == TYPE_CARDKB_V11);

    Mode mode{};
    for (auto&& m : mode_table) {
        EXPECT_TRUE(unit->writeMode(m));

        EXPECT_TRUE(unit->readMode(mode));
        EXPECT_EQ(mode, m);
    }
}

// Bidirectional conversion: toKeyIndex(ch) must return a valid key index for every
// character that the unit can produce, and the mapping must be consistent.
TEST_F(TestCardKB, CharacterToKeyIndexRoundtrip)
{
    SCOPED_TRACE(ustr);

    // Every recognized character must have valid key index AND non-zero mode bits
    for (int c = 1; c < 256; ++c) {
        char ch   = static_cast<char>(c);
        auto kidx = unit->toKeyIndex(ch);
        if (kidx == 0xFF) {
            continue;
        }
        EXPECT_LT(kidx, +UnitCardKB::NUMBER_OF_KEYS)
            << "toKeyIndex(0x" << std::hex << c << ") returned out-of-range key index " << (int)kidx;

        auto mbits = UnitCardKB::character_to_mode_bits(ch);
        EXPECT_NE(mbits, 0) << "character_to_mode_bits(0x" << std::hex << c << ") returned 0 but toKeyIndex returned "
                            << (int)kidx;

        // Mode-specific consistency: verify each recognized character has the expected mode bit
        // normal=0x01, shift=0x02, sym=0x04, fn=0x08
        if (ch >= 'a' && ch <= 'z') {
            EXPECT_TRUE(mbits & 0x01) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing normal bit";
        }
        if (ch >= 'A' && ch <= 'Z') {
            EXPECT_TRUE(mbits & 0x02) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing shift bit";
        }
        // Symbol-only characters (not in normal/shift) must have sym bit
        if (std::strchr("!@#$%^&*(){}[]|\\~`?/<>=+_-;:\"'", ch) && !(mbits & 0x03)) {
            EXPECT_TRUE(mbits & 0x04) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing sym bit";
        }
    }

    // Verify Fn characters (>= 0x80): toKeyIndex must return the correct key
    // CardKB Fn values are key_index + 128, so roundtrip must hold
    for (uint8_t kidx = 0; kidx < +UnitCardKB::NUMBER_OF_KEYS; ++kidx) {
        uint8_t fn_char = kidx + 128;
        auto result     = unit->toKeyIndex(static_cast<char>(fn_char));
        if (result == 0xFF) {
            continue;
        }
        EXPECT_EQ(result, kidx) << "Fn char 0x" << std::hex << (int)fn_char << " mapped to key " << (int)result
                                << " instead of " << (int)kidx;

        // Fn characters must have function mode bit (0x08)
        auto mbits = UnitCardKB::character_to_mode_bits(static_cast<char>(fn_char));
        EXPECT_EQ(mbits & 0x08, 0x08) << "Fn char 0x" << std::hex << (int)fn_char << " mode_bits=" << (int)mbits
                                      << " missing function bit";
    }
}
