# CardKB firmware

## Overview

[In the conventional firmware](https://github.com/m5stack/M5-ProductExampleCodes/blob/master/Unit/CARDKB/firmware_328p/CardKeyBoard/CardKeyBoard.ino), only the released key was retrieved.  
The modifier key qualified the next key after clicking, and double-clicking locked it.

With the new firmware, it is now possible to acquire the pressed state of all keys.  
This enables Software-side processing to detect is/was, Pressed, Released, Holding and Repeating.  
(However, the input order of keys pressed at the same time cannot be identified)  
In addition, key input while pressing the modifier key has been made possible, resulting in an operation feel similar to that of a normal keyboard.  
As before, the modifier key can be locked by double-clicking.

## Required library
- Wire
- [Adafruit\_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)

## Protocrol

|REG MAP(0x5F)||0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|note|
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|Scan |0x10 R| byte0<br>key| byte1<br>key | byte2<br>key| byte3<br>key | byte4<br>key | byte5<br>key | byte6<br>modifier |||||||||| Scanned pressed key status|
|Mode |0x20 R/W| Mode | |||||||||||||||Operation mode<br>0x00:Conventional mode<br> 0x01:M5UnitUnified mode|
|Hardware type|0xF0 R| |||||||||||||type|||Hardware<br>0x01:SKU:U035<br>0x11:SKU:U035-B|
|Firmware version|0xF0 R|||||||||||||||version||Version: firmware version number|

### Mode

|Value|Mode|Notes|
|---|---|---|
|0x00|Conventional |Compatible behavior with conventional firmware|
|0x01|M5UnitUnified|Key pressed status acquisition mode|

### Hardware type

|Value|Type|
|---|---|
|0x01|SKU:U035 CardKB|
|0x11|SKU:U035-B CardKB v1.1|

### Scanned pressed key bits
#### Modifier
|byte|Shift|Symbol|Function|
|---|---|---|---|
|byte 6| 0x01| 0x02| 0x04|

#### Key
Up to 48 bits valid as a key

|byte|MSB key index | LSB key index|
|---|---|---|
|byte 0 |7 | 0  |
|byte 1 |15| 8  |
|byte 2 |23| 16 |
|byte 3 |31| 24 |
|byte 4 |39| 32 |
|byte 5 |47| 40 |


#### Key index

|index|Key|
|---|---|
| 0|ESC|
| 1|1|
| 2|2|
| 3|3|
| 4|4|
| 5|5|
| 6|6|
| 7|7|
| 8|8|
| 9|9|
|10|0|
|11|BS|
|12|TAB|
|13|Q|
|14|W|
|15|E|
|16|R|
|17|T|
|18|Y|
|19|U|
|20|I|
|21|O|
|22|P|
|23|NO\_KEY|
|24|LEFT|
|25|UP|
|26|A|
|27|S|
|28|D|
|29|F|
|30|G|
|31|H|
|32|J|
|33|K|
|34|L|
|35|ENTER|
|36|DOWN|
|37|RIGHT|
|38|Z|
|39|X|
|40|C|
|41|V|
|42|B|
|43|N|
|44|M|
|45|COMMA|
|46|PERIOD|
|47|SPACE|

## Examples with this firmware
- [PlotTSerial](../../UnitUnified/PlotToSerial)
- [SimpleDisplay](../../UnitUnified/SimpleDisplay)


## How to write
- In Japanese
  - [https://shikarunochi.matrix.jp/?p=2910](https://shikarunochi.matrix.jp/?p=2910)  
    This article for CardKB (ATmega328), If you write to CardKB v 1.1 then board setting is ATmega8.
