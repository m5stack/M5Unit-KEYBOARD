# FacesQWERTY firmware

## Overview

[In the conventional firmware](https://github.com/m5stack/FACES-Firmware/blob/master/KeyBoard.ino)
The modifier key qualified the next key after clicking, and double-clicking locked it.

With the new firmware, it is now possible to acquire the pressed state of all keys.  
This enables Software-side processing to detect is/was, Pressed, Released, Holding and Repeating.  
(However, the input order of keys pressed at the same time cannot be identified)  
In addition, key input while pressing the modifier key has been made possible, resulting in an operation feel similar to that of a normal keyboard.  
As before, the modifier key can be locked by double-clicking.

## Required library
- Wire

## Protocrol

|REG MAP(0x08)||0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|note|
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|Scan |0x10 R| byte0<br>key| byte1<br>key | byte2<br>key| byte3<br>key | byte4<br>key | byte5<br>modifier ||||||||||| Scanned pressed key status|
|Mode |0x20 R/W| Mode | |||||||||||||||Operation mode<br>0x00:Conventional mode<br> 0x01:M5UnitUnified mode|
|Faces type|0xF0 R| ||||||||||||type||||Faces type<br> 0x01:FacesQWERTY|
|Firmware version|0xF0 R|||||||||||||||version||Version: firmware version number|

### IRQ
Interrupt occurs under the following conditions (GPIO 5)

|Mode|Condition|
|---|---|
|Conventional|A key was released|
|M5UnitUnified|Any key status was changed|

### Mode

|Value|Mode|Notes|
|---|---|---|
|0x00|Conventional |Compatible behavior with conventional firmware|
|0x01|M5UnitUnified|Key pressed status acquisition mode|

### Faces type

|Value|Type|
|---|---|
|0x01|FacesQWERTY|

(Other Faces will be added in the future)

### Scanned pressed key bits
#### Modifier
|byte|Shift|Symbol|Function|Alt|
|---|---|---|---|---|
|byte 5| 0x01| 0x02| 0x04|0x08|

#### Key
Up to 35 bits valid as a key

|byte|MSB key index | LSB key index|
|---|---|---|
|byte 0 |7 | 0  |
|byte 1 |15| 8  |
|byte 2 |23| 16 |
|byte 3 |31| 24 |
|byte 4 |39| 32 |


#### Key index

|index|Key|
|---|---|
| 0|Q|
| 1|W|
| 2|E|
| 3|R|
| 4|T|
| 5|Y|
| 6|U|
| 7|I|
| 8|O|
| 9|P|
|10|A|
|11|S|
|12|D|
|13|F|
|14|G|
|15|H|
|16|J|
|17|K|
|18|L|
|19|BS|
|20|NO\_KEY|
|21|Z|
|22|X|
|23|C|
|24|V|
|25|B|
|26|N|
|27|M|
|28|DOLLAR|
|29|ENTER|
|30|NO\_KEY|
|31|0|
|32|SPACE|
|33|NO\_KEY|
|34|NO\_KEY|

## Examples with this firmware
- [PlotTSerial](../../UnitUnified/PlotToSerial)
- [SimpleDisplay](../../UnitUnified/SimpleDisplay)


## How to write
- In Japanese
  - [https://ht-deko.com/arduino/m5stack\_faces.html](https://ht-deko.com/arduino/m5stack_faces.html#05)
