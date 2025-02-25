/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
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
#include <cmath>
#include <random>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::keyboard;
using namespace m5::unit::cardkb;
using m5::unit::types::elapsed_time_t;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<100000U>());

class TestCardKB : public ComponentTestBase<UnitCardKB, bool> {
protected:
    virtual UnitCardKB* get_instance() override
    {
        auto ptr = new m5::unit::UnitCardKB();
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestCardKB, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestCardKB, ::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestCardKB, ::testing::Values(false));

namespace {

constexpr Mode mode_table[] = {Mode::Conventional, Mode::M5UnitUnified};

}  // namespace

TEST_P(TestCardKB, Periodic)
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

TEST_P(TestCardKB, M5UnitUnifiedFirmware)
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
