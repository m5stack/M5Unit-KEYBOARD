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
