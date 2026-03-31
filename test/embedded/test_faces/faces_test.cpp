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
#include <cmath>

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
