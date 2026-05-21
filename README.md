# M5Unit - KEYBOARD

## Overview

Library for KEYBOARD using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U035-B
CardKB v1.1 is a card-size '50 key' QWERTY keyboard. Adopts ATMega8A as the MCU, communication port I2C, and one 'RGB-LED' indicator.

### SKU:A003
QWERTY is a full-featured keyboard panel adapted to FACE_BOTTOM. There are 35 keys in total, and each key can be multiplexed by combination keys to output different characters.

### SKU:U215
Unit CardKB2 is a card-sized 42-key portable keyboard input unit. Its compact and lightweight form factor makes it ideal for on-the-go use and embedded integration. 

### SKU:A164
Tab5 Keyboard. (TODO: short product description to be added later.)


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
// PlotToSerial.ino, SimpleDisplay.ino
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
// Choose Tab5 Keyboard operation mode (default: Normal)
// #define USING_TAB5_KEYBOARD_NORMAL
// #define USING_TAB5_KEYBOARD_HID
// #define USING_TAB5_KEYBOARD_CHARACTER
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
