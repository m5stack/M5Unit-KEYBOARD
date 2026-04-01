/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitFacesQWERTY
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_FacesQWERTY.hpp>
#include <cstring>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::keyboard;
using m5::unit::types::elapsed_time_t;

class TestFacesQWERTY : public I2CComponentTestBase<UnitFacesQWERTY> {
protected:
    virtual UnitFacesQWERTY* get_instance() override
    {
        auto ptr = new m5::unit::UnitFacesQWERTY();
        return ptr;
    }
};

namespace {

constexpr Mode mode_table[] = {Mode::Conventional, Mode::M5UnitUnified};

}  // namespace

TEST_F(TestFacesQWERTY, Config)
{
    SCOPED_TRACE(ustr);

    auto cfg = unit->config();
    EXPECT_TRUE(cfg.start_periodic);
    EXPECT_EQ(cfg.interval, 10U);
    EXPECT_EQ(cfg.mode, keyboard::Mode::Conventional);
    EXPECT_FALSE(cfg.trigger_irq);

    cfg.interval = 50;
    unit->config(cfg);
    auto cfg2 = unit->config();
    EXPECT_EQ(cfg2.interval, 50U);

    // Restore
    cfg.interval = 10;
    unit->config(cfg);
}

TEST_F(TestFacesQWERTY, BitwiseInitialState)
{
    SCOPED_TRACE(ustr);
    EXPECT_EQ(unit->nowBits(), 0U);
    EXPECT_EQ(unit->pressedBits(), 0U);
    EXPECT_EQ(unit->releasedBits(), 0U);
    EXPECT_EQ(unit->holdingBits(), 0U);
    EXPECT_EQ(unit->repeatingBits(), 0U);
    EXPECT_FALSE(unit->isPressed());
}

TEST_F(TestFacesQWERTY, Periodic)
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

TEST_F(TestFacesQWERTY, M5UnitUnifiedFirmware)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());

    if (!unit->firmwareVersion()) {
        M5_LOGI("FacesQWERTY firmware is conventional");
        return;
    }

    uint8_t ver{};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_EQ(ver, unit->firmwareVersion());
    EXPECT_NE(ver, 0);

    uint8_t ftype{};
    EXPECT_TRUE(unit->readFacesType(ftype));
    EXPECT_EQ(ftype, unit->facesType());
    EXPECT_EQ(ftype, faces::TYPE_QWERTY);

    Mode mode{};
    for (auto&& m : mode_table) {
        EXPECT_TRUE(unit->writeMode(m));

        EXPECT_TRUE(unit->readMode(mode));
        EXPECT_EQ(mode, m);
    }
}

TEST_F(TestFacesQWERTY, CharacterToKeyIndexRoundtrip)
{
    SCOPED_TRACE(ustr);

    for (int c = 1; c < 256; ++c) {
        char ch   = static_cast<char>(c);
        auto kidx = unit->toKeyIndex(ch);
        if (kidx == 0xFF) {
            continue;
        }
        EXPECT_LT(kidx, +UnitFacesQWERTY::NUMBER_OF_KEYS)
            << "toKeyIndex(0x" << std::hex << c << ") returned out-of-range key index " << (int)kidx;

        auto mbits = UnitFacesQWERTY::character_to_mode_bits(ch);
        EXPECT_NE(mbits, 0) << "character_to_mode_bits(0x" << std::hex << c << ") returned 0 but toKeyIndex returned "
                            << (int)kidx;

        if (ch >= 'a' && ch <= 'z') {
            EXPECT_TRUE(mbits & 0x01) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing normal bit";
        }
        if (ch >= 'A' && ch <= 'Z') {
            EXPECT_TRUE(mbits & 0x02) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing shift bit";
        }
        // Sym-only: not in normal/shift AND not in fn
        if (std::strchr("!@#$%^&*(){}[]|\\~`?/<>=+_-;:\"'", ch) && !(mbits & 0x0B)) {
            EXPECT_TRUE(mbits & 0x04) << "char '" << ch << "' mode_bits=" << (int)mbits << " missing sym bit";
        }
    }

    // Verify Fn characters: FacesQWERTY Fn chars are NOT kidx+128,
    // so only check range validity for all recognized >= 0x80 chars
    for (int c = 128; c < 256; ++c) {
        auto kidx = unit->toKeyIndex(static_cast<char>(c));
        if (kidx == 0xFF) {
            continue;
        }
        EXPECT_LT(kidx, +UnitFacesQWERTY::NUMBER_OF_KEYS)
            << "Fn char 0x" << std::hex << c << " mapped to out-of-range key index " << (int)kidx;
    }
}
