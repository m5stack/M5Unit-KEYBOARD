/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <gtest/gtest.h>

#include "../../src/utility/button_event_detector.hpp"

using m5::unit::keyboard_bitwise::ButtonEvent;
using m5::unit::keyboard_bitwise::ButtonEventDetector;

TEST(ButtonEventDetector, RawEdges)
{
    ButtonEventDetector d;  // defaults: short=200, long=400
    EXPECT_EQ(d.edge(true, 0U), ButtonEvent::PressDown);
    // Hardware-repeat PRESS while already down is ignored.
    EXPECT_EQ(d.edge(true, 50U), ButtonEvent::None);
    EXPECT_EQ(d.edge(false, 100U), ButtonEvent::PressUp);
}

TEST(ButtonEventDetector, SingleClickDelayedAfterShortWindow)
{
    ButtonEventDetector d;
    EXPECT_EQ(d.edge(true, 0U), ButtonEvent::PressDown);
    EXPECT_EQ(d.edge(false, 80U), ButtonEvent::PressUp);  // tap (released < long)
    // Within the short window: no click yet (could still become a double).
    EXPECT_EQ(d.poll(150U), ButtonEvent::None);
    // After short window elapses with no second press: SingleClick.
    EXPECT_EQ(d.poll(300U), ButtonEvent::SingleClick);
    // Resolved; no repeat events afterwards.
    EXPECT_EQ(d.poll(400U), ButtonEvent::None);
}

TEST(ButtonEventDetector, LongPressEmitsStartThenUpNoClick)
{
    ButtonEventDetector d;
    EXPECT_EQ(d.edge(true, 0U), ButtonEvent::PressDown);
    EXPECT_EQ(d.poll(399U), ButtonEvent::None);
    EXPECT_EQ(d.poll(400U), ButtonEvent::LongPressStart);  // held >= long
    EXPECT_EQ(d.edge(false, 600U), ButtonEvent::LongPressUp);
    // No SingleClick after a long press.
    EXPECT_EQ(d.poll(900U), ButtonEvent::None);
}

TEST(ButtonEventDetector, DoubleClickWithinShortWindow)
{
    ButtonEventDetector d;
    EXPECT_EQ(d.edge(true, 0U), ButtonEvent::PressDown);
    EXPECT_EQ(d.edge(false, 60U), ButtonEvent::PressUp);
    // Second press arrives within the short window -> repeat.
    EXPECT_EQ(d.edge(true, 120U), ButtonEvent::PressDown);
    EXPECT_EQ(d.edge(false, 180U), ButtonEvent::PressUp);
    // After short window with no third press: DoubleClick.
    EXPECT_EQ(d.poll(400U), ButtonEvent::DoubleClick);
    EXPECT_EQ(d.poll(500U), ButtonEvent::None);
}
