# M5Unit - KEYBOARD

## Overview

Library for KEYBOARD using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.


### SKU:U035-B
CardKB v1.1 is a card-size '50 key' QWERTY keyboard. Adpots ATMega8A as the MCU, communication port I2C, and one 'RGB-LED' indicator.

### SKU:A003
QWERTY is a full-featured keyboard panel adapted to FACE_BOTTOM. There are 35 keys in total, and each key can be multiplexed by combination keys to output different characters.


## Related Link
See also examples using conventional methods here.

- [Unit CardKB v1.1 & Datasheet](https://docs.m5stack.com/ja/unit/cardkb_1.1)
- [Faces QWERTY & Datasheet](https://docs.m5stack.com/en/module/faces_keyboard)


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
#if !defined(USING_UNIT_CARDKB) && !defined(USING_UNIT_FACES_QWERTY)
// For CardKB
// #define USING_UNIT_CARDKB
// For FacesQWERTY
// #define USING_UNIT_FACES_QWERTY
#endif
// *************************************************************
```

## New firmware
See also [examples/firmware](examples/firmware)

When this firmware is applied and the Units are used, the operating feel is very different.  
Allows for input while pressing modifier keys and detection of key presses.

### Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-KEYBOARD/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

#### Required
- [Doxygen](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)
