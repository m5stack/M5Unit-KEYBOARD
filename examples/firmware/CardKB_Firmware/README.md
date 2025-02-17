# CardKB firmware

## Overview

[In the conventional firmware](https://github.com/m5stack/M5-ProductExampleCodes/blob/master/Unit/CARDKB/firmware_328p/CardKeyBoard/CardKeyBoard.ino), only the released key was retrieved.  
The Alt key qualified the next key after clicking, and double-clicking locked it.

With the new firmware, it is now possible to acquire the press state of all keys.  
This enables Unit-side processing to detect is/was ,Pressed/Released, Holding and Repeating.
(However, the input order of keys pressed at the same time cannot be identified)  
In addition, key input while pressing the Alt key has been made possible, resulting in an operation feel similar to that of a normal keyboard.  
As before, the Alt key can be locked by double-clicking the Alt key.


## Protocrol

|REG MAP(0x5F)||0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|note|
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|Scan |0x10 R| byte 0| byte 1 | byte 2| byte 3 | byte 4 | byte 5 | byte 6 |||||||||| Scaned key status<br>[0...5]:key pressed bits<br>[6]:alt pressed bits|
|Mode |0x20 R/W| Mode | |||||||||||||||Operation mode<br> 0x00:released mode<br> 0x01:scan mode|
|Hardware type|0xF0 R| |||||||||||||type|||Hardware<br>0x01:SKU:U035<br>0x11:SKU:U035-B|
|Firmware version|0xF0 R|||||||||||||||version||Version: firmware version number<br>High nibble:Major<br>Low nibble:Minor|

### Mode

|Mode|Value|Notes|
|---|---|---|
|Released | 0x00| Compatible behavior with conventional firmware|
|Scan| 0x01 |Key press state acquisition mode|


### Scaned key status
#### Key 
|byte|MSB key index | LSB key index|
|---|---|---|
|byte 0 |7 | 0  |
|byte 1 |15| 8  |
|byte 2 |23| 16 |
|byte 3 |31| 24 |
|byte 4 |39| 32 |
|byte 5 |47| 40 |

#### Alt
|byte|Shift|Symbol|Function|
|---|---|---|---|
|byte 6| 0x10| 0x80| 0x40|

#### Key index

|index|Key|
|---|---|
|0|ESC|
|1|1|


//// TODO




## Examples with new firmware
[See also it](../../UnitUnified/UnitCardKB/PlotToSerial)
