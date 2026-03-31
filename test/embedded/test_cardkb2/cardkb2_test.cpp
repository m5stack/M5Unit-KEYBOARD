/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitCardKB2
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_CardKB2.hpp>
#include <cmath>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::keyboard;
using m5::unit::types::elapsed_time_t;

class TestCardKB2 : public I2CComponentTestBase<UnitCardKB2> {
protected:
    virtual UnitCardKB2* get_instance() override
    {
        auto ptr = new m5::unit::UnitCardKB2();
        return ptr;
    }
};

TEST_F(TestCardKB2, Periodic)
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

TEST_F(TestCardKB2, M5UnitUnifiedFirmware)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->inPeriodic());

    if (!unit->firmwareVersion()) {
        M5_LOGI("CardKB2 firmware is conventional");
        return;
    }

    uint8_t ver{};
    EXPECT_TRUE(unit->readFirmwareVersion(ver));
    EXPECT_EQ(ver, unit->firmwareVersion());
    EXPECT_NE(ver, 0);

    // CardKB2 does not support mode switching (I2C=Conventional, UART=M5UnitUnified)
    Mode mode{};
    EXPECT_FALSE(unit->writeMode(Mode::M5UnitUnified));
    EXPECT_FALSE(unit->readMode(mode));
}
