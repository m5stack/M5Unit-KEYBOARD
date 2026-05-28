# M5Unit - KEYBOARD

## Overview

Library for KEYBOARD using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U035-B
**Unit CardKB v1.1** is a card-sized QWERTY 50-key PCB matrix keyboard that uses ATMega8A as the encoding MCU, with an output interface of I2C. It has one onboard RGB-LED to indicate keyboard status.

### SKU:A003
**Faces QWERTY** is a full-featured keyboard panel compatible with FACE\_BOTTOM, featuring 35 keys. Each key can be reused through key combinations to output different characters. It integrates an MEGA328 processor internally and operates in slave mode via the I2C communication protocol (0x08). The "sym" and "Fn" function keys are used for shift switching, while the "aA" function key is used for case switching. A single click on the corresponding function key activates single-character input with the indicator light on, while a double click makes the indicator light blink, enabling continuous input. Clicking again restores the previous state.

### SKU:U215
**Unit CardKB2** is a card-sized 42-key portable keyboard input unit. Compact and lightweight, it is designed for everyday carry and embedded integration. Powered by the ESP32-C61HF4, it supports 2.4 GHz Wi-Fi 6. The preloaded firmware supports four communication modes — I2C, UART, BLE HID, and ESP-NOW — for flexible connectivity with host devices. An onboard HY2.0-4P connector enables communication in I2C/UART mode, while a USB Type-C port provides power supply and firmware flashing. Additional features include an RGB LED status indicator, a Reset button, a Boot button, and built-in input overvoltage protection, making it well-suited for wireless input, interactive control, and a wide range of portable applications.

### SKU:A164
**Tab5 Keyboard** is a 70-key physical keyboard input expansion module designed specifically for Tab5. It connects directly to the host via Tab5 Ext.Port1, enabling a plug-and-play input experience.
The module integrates an STM32F030C8T6 main control chip, responsible for keyboard matrix scanning and communication processing. The preloaded firmware supports three operating modes: **Normal**, **HID**, and **Character**, adapting to different interaction requirements.
The keyboard adopts a 14 x 5 matrix design, supports multiple simultaneous key presses, and provides a complete set of letters, numbers, symbols, and commonly used function keys such as Aa, Ctrl, Sym, Alt, Tab, Esc, and arrow keys.
The device communicates with Tab5 via I2C and provides an independent interrupt pin for low-latency real-time reporting of key events. Two onboard RGB LEDs can be used for full-color status indication. With a compact structure and simple connection method, it is suitable for text input, command interaction, and various portable applications that require a physical keyboard.


## Related Link
See also examples using conventional methods here.

- [Unit CardKB v1.1 & Datasheet](https://docs.m5stack.com/en/unit/cardkb_1.1)
- [Faces QWERTY & Datasheet](https://docs.m5stack.com/en/module/faces_keyboard)
- [Unit CardKB2 & Datasheet](https://docs.m5stack.com/en/products/sku/U215)
- [Tab5 Keyboard & Datasheet](https://docs.m5stack.com/en/products/sku/A164)


### Required Libraries:
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-KEYBOARD - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

### For ArduinoIDE settings
You must choose a define symbol for the unit you will use.  
(Rewrite source or specify with compile options)

```cpp
// PlotToSerial.ino
// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_CARDKB) && !defined(USING_UNIT_CARDKB2) && !defined(USING_UNIT_FACES_QWERTY) && \
    !defined(USING_UNIT_TAB5_KEYBOARD)
// For UnitCardKB (U035-B)
// #define USING_UNIT_CARDKB
// For UnitCardKB2 (U215)
// #define USING_UNIT_CARDKB2
// For FacesQWERTY (A003)
// #define USING_UNIT_FACES_QWERTY
// For UnitTab5Keyboard (A164) (built into M5Stack Tab5)
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
```

## New firmware (CardKB / FacesQWERTY)
See also [examples/firmware](examples/firmware)

When this firmware is applied to CardKB or FacesQWERTY, the operating feel is very different.

- Bitwise key state tracking — detects all keys simultaneously
- Per-key press, hold, release, and repeat detection
- Individual modifier key state (Shift, Sym, Fn)
- Configurable hold and repeat thresholds

## CardKB2 firmware

The CardKB2 firmware source is maintained at [m5stack/M5Unit-CardKB2-UserDemo](https://github.com/m5stack/M5Unit-CardKB2-UserDemo).

## CardKB2 modifier state sync (UART mode only)

In UART mode, `UnitCardKB2UART` and the CardKB2 firmware track Sym toggle and Caps Lock state independently, with no query API between them. After resetting the host MCU (or restarting the application), also press the RST button on the CardKB2 so both sides start in the same cleared state.

I2C mode is unaffected — the firmware sends translated ASCII directly.

## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-KEYBOARD/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)
