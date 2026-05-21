/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitCardKB/UnitCardKB2/UnitFacesQWERTY/UnitTab5Keyboard
*/
// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_CARDKB) && !defined(USING_UNIT_CARDKB2) && !defined(USING_UNIT_FACES_QWERTY) && \
    !defined(USING_UNIT_TAB5_KEYBOARD)
// For CardKB
// #define USING_UNIT_CARDKB
// For CardKB2
// #define USING_UNIT_CARDKB2
// For FacesQWERTY
// #define USING_UNIT_FACES_QWERTY
// For Tab5 Keyboard (built into M5Stack Tab5)
// #define USING_UNIT_TAB5_KEYBOARD
// Tab5 Keyboard operation mode is cycled at runtime with BtnA (Normal/HID/Character).
// *************************************************************
#if defined(USING_UNIT_CARDKB2)
// Choose one communication mode for CardKB2
// For I2C
// #define USING_I2C_FOR_CARDKB2
// For UART
// #define USING_UART_FOR_CARDKB2
#endif
#endif
#include "main/PlotToSerial.cpp"
