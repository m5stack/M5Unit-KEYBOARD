/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <gtest/gtest.h>

#include "../../src/utility/bitwise_state.hpp"

using m5::unit::keyboard_bitwise::BitwiseState;

TEST(BitwiseState, Default)
{
    BitwiseState<8> s;
    EXPECT_TRUE(s.now.none());
    EXPECT_TRUE(s.prev.none());
    EXPECT_TRUE(s.holding.none());
    EXPECT_TRUE(s.pressed.none());
    EXPECT_TRUE(s.released.none());
    EXPECT_TRUE(s.was_hold.none());
    EXPECT_TRUE(s.repeating.none());
    EXPECT_EQ(s.press_at[0], 0U);
    EXPECT_EQ(s.last_repeat_at[7], 0U);
    EXPECT_EQ(s.holding_threshold_ms, 800U);
    EXPECT_EQ(s.repeat_initial_ms, 400U);
    EXPECT_EQ(s.repeat_rate_ms, 400U);
}

TEST(BitwiseState, ResetOneShot)
{
    BitwiseState<8> s;
    s.now.set(0);
    s.prev.set(1);
    s.holding.set(2);
    s.pressed.set(3);
    s.released.set(4);
    s.was_hold.set(5);
    s.repeating.set(6);

    s.resetOneShot();

    // Persistent state preserved.
    EXPECT_TRUE(s.now.test(0));
    EXPECT_TRUE(s.prev.test(1));
    EXPECT_TRUE(s.holding.test(2));
    // One-shot state cleared.
    EXPECT_TRUE(s.pressed.none());
    EXPECT_TRUE(s.released.none());
    EXPECT_TRUE(s.was_hold.none());
    EXPECT_TRUE(s.repeating.none());
}

TEST(BitwiseState, ResetAll)
{
    BitwiseState<8> s;
    s.now.set(0);
    s.prev.set(1);
    s.holding.set(2);
    s.pressed.set(3);
    s.press_at[0]       = 123U;
    s.last_repeat_at[7] = 456U;

    s.resetAll();

    EXPECT_TRUE(s.now.none());
    EXPECT_TRUE(s.prev.none());
    EXPECT_TRUE(s.holding.none());
    EXPECT_TRUE(s.pressed.none());
    EXPECT_EQ(s.press_at[0], 0U);
    EXPECT_EQ(s.last_repeat_at[7], 0U);
}

TEST(BitwiseState, SetKeyPress)
{
    BitwiseState<8> s;
    s.setKey(3, true, 1000U);
    EXPECT_TRUE(s.now.test(3));
    EXPECT_EQ(s.press_at[3], 1000U);
    EXPECT_EQ(s.last_repeat_at[3], 0U);
}

TEST(BitwiseState, SetKeyRelease)
{
    BitwiseState<8> s;
    s.setKey(3, true, 1000U);
    s.setKey(3, false, 2000U);
    EXPECT_FALSE(s.now.test(3));
    EXPECT_EQ(s.press_at[3], 1000U);  // press_at NOT cleared on release
}

TEST(BitwiseState, SetKeyPressHeldPreservesPressAt)
{
    BitwiseState<8> s;
    s.setKey(3, true, 1000U);
    s.last_repeat_at[3] = 1500U;  // simulate repeat tick recorded mid-hold
    s.setKey(3, true, 2000U);     // still held — must NOT overwrite press_at / last_repeat_at
    EXPECT_TRUE(s.now.test(3));
    EXPECT_EQ(s.press_at[3], 1000U);
    EXPECT_EQ(s.last_repeat_at[3], 1500U);
}

TEST(BitwiseState, SetKeyOutOfRange)
{
    BitwiseState<8> s;
    s.setKey(8, true, 1000U);    // == N, out of range
    s.setKey(255, true, 1000U);  // far out of range
    EXPECT_TRUE(s.now.none());
}

TEST(BitwiseState, ComputeEdgesPressed)
{
    BitwiseState<8> s;
    s.prev.set(1);
    s.now.set(1);
    s.now.set(3);  // 3 newly pressed
    s.computeEdges();
    EXPECT_FALSE(s.pressed.test(1));  // still held
    EXPECT_TRUE(s.pressed.test(3));
    EXPECT_TRUE(s.released.none());
}

TEST(BitwiseState, ComputeEdgesReleased)
{
    BitwiseState<8> s;
    s.prev.set(2);  // was pressed
    // s.now has bit 2 clear (released)
    s.computeEdges();
    EXPECT_TRUE(s.pressed.none());
    EXPECT_TRUE(s.released.test(2));
}

TEST(BitwiseState, TickHoldRepeatBelowThreshold)
{
    BitwiseState<8> s;
    s.now.set(0);
    s.press_at[0] = 100U;
    s.tickHoldRepeat(500U);  // elapsed = 400, < holding 800
    EXPECT_FALSE(s.holding.test(0));
    EXPECT_FALSE(s.was_hold.test(0));
    EXPECT_TRUE(s.repeating.test(0));  // elapsed 400 >= repeat_initial 400 -> fires
    EXPECT_EQ(s.last_repeat_at[0], 500U);
}

TEST(BitwiseState, TickHoldFiresAtThreshold)
{
    BitwiseState<8> s;
    s.now.set(1);
    s.press_at[1] = 100U;
    s.tickHoldRepeat(900U);  // elapsed = 800, == holding 800
    EXPECT_TRUE(s.holding.test(1));
    EXPECT_TRUE(s.was_hold.test(1));  // rising edge
}

TEST(BitwiseState, TickWasHoldIsOneShot)
{
    BitwiseState<8> s;
    s.now.set(2);
    s.press_at[2] = 100U;
    s.tickHoldRepeat(900U);   // first tick: was_hold rises
    s.was_hold.reset();       // simulate resetOneShot() between updates
    s.tickHoldRepeat(1000U);  // second tick: still holding but no edge
    EXPECT_TRUE(s.holding.test(2));
    EXPECT_FALSE(s.was_hold.test(2));
}

// Verifies `pressed` (= wasPressed externally) stays high for exactly one frame
// across a press → held → released → re-pressed sequence.
TEST(BitwiseState, WasPressedFiresExactlyOneFrame)
{
    BitwiseState<8> s;
    s.holding_threshold_ms = 1000U;  // keep hold/repeat out of the way
    s.repeat_initial_ms    = 1000U;
    s.repeat_rate_ms       = 1000U;

    // Frame 1: rising edge (released → pressed).
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 0U);
    s.computeEdges();
    s.tickHoldRepeat(0U);
    EXPECT_TRUE(s.pressed.test(3));
    EXPECT_TRUE(s.now.test(3));

    // Frame 2: still held, `pressed` must clear.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 10U);
    s.computeEdges();
    s.tickHoldRepeat(10U);
    EXPECT_FALSE(s.pressed.test(3));
    EXPECT_TRUE(s.now.test(3));

    // Frame 3: released, `released` rises but `pressed` stays clear.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, false, 20U);
    s.computeEdges();
    s.tickHoldRepeat(20U);
    EXPECT_FALSE(s.pressed.test(3));
    EXPECT_TRUE(s.released.test(3));

    // Frame 4: re-pressed → `pressed` rises again (a fresh 1-frame).
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 30U);
    s.computeEdges();
    s.tickHoldRepeat(30U);
    EXPECT_TRUE(s.pressed.test(3));

    // Frame 5: still held again, `pressed` clears.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 40U);
    s.computeEdges();
    s.tickHoldRepeat(40U);
    EXPECT_FALSE(s.pressed.test(3));
}

// Verifies `released` (= wasReleased externally) stays high for exactly one
// frame after a held key is released, and does not re-fire while idle.
TEST(BitwiseState, WasReleasedFiresExactlyOneFrame)
{
    BitwiseState<8> s;
    s.holding_threshold_ms = 1000U;
    s.repeat_initial_ms    = 1000U;
    s.repeat_rate_ms       = 1000U;

    // Frame 1: press.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 0U);
    s.computeEdges();
    s.tickHoldRepeat(0U);
    EXPECT_FALSE(s.released.test(3));

    // Frame 2: release → `released` rises.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, false, 10U);
    s.computeEdges();
    s.tickHoldRepeat(10U);
    EXPECT_TRUE(s.released.test(3));
    EXPECT_FALSE(s.now.test(3));

    // Frame 3: still released, `released` must clear.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, false, 20U);
    s.computeEdges();
    s.tickHoldRepeat(20U);
    EXPECT_FALSE(s.released.test(3));

    // Frame 4: still released — no spurious re-fire.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, false, 30U);
    s.computeEdges();
    s.tickHoldRepeat(30U);
    EXPECT_FALSE(s.released.test(3));
}

// Verifies `repeating` is a per-tick 1-frame flag: high only on frames where
// the repeat interval elapses, never on consecutive frames within one interval.
TEST(BitwiseState, RepeatingClearsBetweenIntervals)
{
    BitwiseState<8> s;
    s.holding_threshold_ms = 1000U;  // keep was_hold out of the test
    s.repeat_initial_ms    = 100U;
    s.repeat_rate_ms       = 100U;

    // Frame 1: press at t=0. Below repeat_initial.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 0U);
    s.computeEdges();
    s.tickHoldRepeat(0U);
    EXPECT_FALSE(s.repeating.test(3));

    // Frame 2: cross repeat_initial — first repeat fires.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 100U);
    s.computeEdges();
    s.tickHoldRepeat(100U);
    EXPECT_TRUE(s.repeating.test(3));

    // Frame 3: only +50 ms — still inside the rate window → cleared, not high.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 150U);
    s.computeEdges();
    s.tickHoldRepeat(150U);
    EXPECT_FALSE(s.repeating.test(3));

    // Frame 4: +100 ms since last repeat — next repeat fires.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 200U);
    s.computeEdges();
    s.tickHoldRepeat(200U);
    EXPECT_TRUE(s.repeating.test(3));

    // Frame 5: +50 ms again — clears.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 250U);
    s.computeEdges();
    s.tickHoldRepeat(250U);
    EXPECT_FALSE(s.repeating.test(3));
}

// Verifies `now` (= isPressed externally) is persistent across frames while the
// key remains pressed. Sanity counterpart to the 1-frame edge tests.
TEST(BitwiseState, IsPressedPersistsWhileHeld)
{
    BitwiseState<8> s;
    s.holding_threshold_ms = 10000U;  // never trigger
    s.repeat_initial_ms    = 10000U;
    s.repeat_rate_ms       = 10000U;

    for (uint32_t t = 0; t <= 500U; t += 100U) {
        s.commitPrev();
        s.resetOneShot();
        s.setKey(5, true, t);
        s.computeEdges();
        s.tickHoldRepeat(t);
        EXPECT_TRUE(s.now.test(5));
    }
    // Then release.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(5, false, 600U);
    s.computeEdges();
    s.tickHoldRepeat(600U);
    EXPECT_FALSE(s.now.test(5));
}

// Verifies `holding` (= isHolding externally) latches at threshold and stays
// high while the key remains pressed, then clears on release.
TEST(BitwiseState, IsHoldingPersistsAfterThreshold)
{
    BitwiseState<8> s;
    s.holding_threshold_ms = 100U;
    s.repeat_initial_ms    = 10000U;  // suppress repeat
    s.repeat_rate_ms       = 10000U;

    // Frame 1: press at t=0.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(2, true, 0U);
    s.computeEdges();
    s.tickHoldRepeat(0U);
    EXPECT_FALSE(s.holding.test(2));

    // Frames 2-5: still below threshold then cross at t=100. Holding latches and persists.
    const uint32_t times[]        = {50U, 100U, 150U, 200U};
    const bool expected_holding[] = {false, true, true, true};
    for (size_t i = 0; i < 4; ++i) {
        s.commitPrev();
        s.resetOneShot();
        s.setKey(2, true, times[i]);
        s.computeEdges();
        s.tickHoldRepeat(times[i]);
        EXPECT_EQ(s.holding.test(2), expected_holding[i]);
    }

    // Release → holding clears.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(2, false, 250U);
    s.computeEdges();
    s.tickHoldRepeat(250U);
    EXPECT_FALSE(s.holding.test(2));
}

// Full update-cycle equivalent of the above: verifies was_hold stays high for
// exactly one frame when the realistic order (commitPrev → resetOneShot →
// setKey → computeEdges → tickHoldRepeat) is followed across many frames.
TEST(BitwiseState, WasHoldFiresExactlyOneFrame)
{
    BitwiseState<8> s;
    s.holding_threshold_ms = 100U;
    s.repeat_initial_ms    = 1000U;  // keep repeat out of the way
    s.repeat_rate_ms       = 1000U;

    // Frame 1: press at t=0. Below threshold.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 0U);
    s.computeEdges();
    s.tickHoldRepeat(0U);
    EXPECT_FALSE(s.was_hold.test(3));
    EXPECT_FALSE(s.holding.test(3));

    // Frame 2: still pressed, still below threshold.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 50U);
    s.computeEdges();
    s.tickHoldRepeat(50U);
    EXPECT_FALSE(s.was_hold.test(3));
    EXPECT_FALSE(s.holding.test(3));

    // Frame 3: crosses threshold. was_hold rises for this frame only.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 100U);
    s.computeEdges();
    s.tickHoldRepeat(100U);
    EXPECT_TRUE(s.was_hold.test(3));
    EXPECT_TRUE(s.holding.test(3));

    // Frame 4: was_hold must clear, holding persists.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 150U);
    s.computeEdges();
    s.tickHoldRepeat(150U);
    EXPECT_FALSE(s.was_hold.test(3));
    EXPECT_TRUE(s.holding.test(3));

    // Frame 5: still no rising edge.
    s.commitPrev();
    s.resetOneShot();
    s.setKey(3, true, 200U);
    s.computeEdges();
    s.tickHoldRepeat(200U);
    EXPECT_FALSE(s.was_hold.test(3));
    EXPECT_TRUE(s.holding.test(3));
}

TEST(BitwiseState, TickRepeatRateInterval)
{
    BitwiseState<8> s;
    s.repeat_rate_ms = 80U;  // Tab5 style
    s.now.set(3);
    s.press_at[3] = 100U;
    s.tickHoldRepeat(500U);  // elapsed 400 >= initial 400 -> fires, last_repeat = 500
    s.repeating.reset();
    s.tickHoldRepeat(550U);  // 550 - 500 = 50, < rate 80 -> no fire
    EXPECT_FALSE(s.repeating.test(3));
    s.tickHoldRepeat(600U);  // 600 - 500 = 100, >= rate 80 -> fires
    EXPECT_TRUE(s.repeating.test(3));
    EXPECT_EQ(s.last_repeat_at[3], 600U);
}

TEST(BitwiseState, TickKeyReleaseClearsHold)
{
    BitwiseState<8> s;
    s.now.set(4);
    s.press_at[4] = 100U;
    s.holding.set(4);
    s.last_repeat_at[4] = 500U;
    s.now.reset(4);  // released
    s.tickHoldRepeat(1000U);
    EXPECT_FALSE(s.holding.test(4));
    EXPECT_EQ(s.last_repeat_at[4], 0U);
}

TEST(BitwiseState, CommitPrev)
{
    BitwiseState<8> s;
    s.now.set(0);
    s.now.set(5);
    s.commitPrev();
    EXPECT_TRUE(s.prev.test(0));
    EXPECT_TRUE(s.prev.test(5));
    EXPECT_TRUE(s.now.test(0));  // now unchanged
    EXPECT_TRUE(s.now.test(5));
}
