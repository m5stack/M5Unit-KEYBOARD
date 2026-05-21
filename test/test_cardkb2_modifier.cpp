/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <gtest/gtest.h>

#include "../../src/utility/cardkb2_modifier_state.hpp"

using m5::unit::cardkb2::CardKB2ModifierState;

TEST(CardKB2ModifierState, SymTapToggles)
{
    CardKB2ModifierState m;
    m.onSymEdge(true, 0U);
    EXPECT_TRUE(m.symActive());
    m.onSymEdge(false, 80U);
    m.poll(300U);
    EXPECT_TRUE(m.symActive());
    m.onSymEdge(true, 1000U);
    m.onSymEdge(false, 1080U);
    m.poll(1300U);
    EXPECT_FALSE(m.symActive());
}

TEST(CardKB2ModifierState, SymLongPressDoesNotToggle)
{
    CardKB2ModifierState m;
    m.onSymEdge(true, 0U);
    m.poll(400U);
    m.onSymEdge(false, 600U);
    m.poll(900U);
    EXPECT_FALSE(m.symActive());
}

TEST(CardKB2ModifierState, SymMomentaryWhileHeld)
{
    CardKB2ModifierState m;
    m.onSymEdge(true, 0U);
    EXPECT_TRUE(m.symActive());
    m.onSymEdge(false, 100U);
    EXPECT_FALSE(m.symActive());
}

TEST(CardKB2ModifierState, CapsSingleClickIsOneShot)
{
    CardKB2ModifierState m;
    m.onAaEdge(true, 0U);
    m.onAaEdge(false, 60U);
    m.poll(300U);
    EXPECT_TRUE(m.capsActive());
    m.consumeCapsOneShot();
    EXPECT_FALSE(m.capsActive());
}

TEST(CardKB2ModifierState, CapsDoubleClickLocks)
{
    CardKB2ModifierState m;
    m.onAaEdge(true, 0U);
    m.onAaEdge(false, 40U);
    m.onAaEdge(true, 90U);
    m.onAaEdge(false, 130U);
    m.poll(400U);
    EXPECT_TRUE(m.capsActive());
    m.consumeCapsOneShot();
    EXPECT_TRUE(m.capsActive());
    m.onAaEdge(true, 1000U);
    m.onAaEdge(false, 1050U);
    m.poll(1300U);
    EXPECT_FALSE(m.capsActive());
}

TEST(CardKB2ModifierState, CapsHoldWhileHeld)
{
    CardKB2ModifierState m;
    m.onAaEdge(true, 0U);
    EXPECT_TRUE(m.capsActive());
    m.poll(450U);
    m.onAaEdge(false, 500U);
    EXPECT_FALSE(m.capsActive());
}

TEST(CardKB2ModifierState, FnMomentary)
{
    CardKB2ModifierState m;
    EXPECT_FALSE(m.fnActive());
    m.onFnEdge(true);
    EXPECT_TRUE(m.fnActive());
    m.onFnEdge(false);
    EXPECT_FALSE(m.fnActive());
}
