/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Key matrix visualizer example using M5UnitUnified for UnitTab5Keyboard.

  Renders the 5 x 14 Tab5 keyboard matrix on the Tab5 LCD: each cell is
  filled when the corresponding key is currently pressed and reverts to an
  outline when released.
*/
// This example targets UnitTab5Keyboard (Normal mode) only — no build-time
// define is required. `M5UnitUnifiedKEYBOARD.h` always exposes the Tab5 unit.
#include "main/KeyMatrixVisualizer.cpp"
