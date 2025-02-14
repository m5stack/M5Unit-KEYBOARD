# CardKB firmware

## Overview

In the previous protocol, only the clicked key was acquired, and the handling of the Alt key was also changed by pressing the key after the click (the Alt key could be locked by double-clicking it).

In the new firmware, it is now possible to acquire the press state of all keys.  
This enables Unit-side processing to detect is/was Pressed/Released Holing Repeating.  
In addition, key input while pressing the Alt key has been made possible, resulting in an operation feel similar to that of a normal keyboard.  
As before, the key can be locked by double-clicking the Alt key.


## Protocrol

|REG MAP(0x5F)||0|1|2|3|4|5|6|7|8|9|A|B|C|D|E|F|note|
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|Scan |0x10 R| byte 0| byte 1 | byte 2| byte 3 | byte 4 | byte 5 | byte 6 |||||||||| Scaned key status<br>0..5:key pressed bits<br>6:alt pressed bits|
|Mode |0x20 R/W| Mode | |||||||||||||||Operation mode<br> 0x00:released mode<br> 0x01:scan mode|
|Hardware type|0xF0 R| |||||||||||||type|||Hardware<br>0x01:SKU:U035<br>0x11:SKU:U035-B|
|Firmware version|0xF0 R|||||||||||||||version||Version: firmware version number<br>High nibble:Major<br>Low nibble:Minor|


### Scaned key status
7 bytes array. First 6 bytes is key status, last 1 byte is alt key status.  






## Examples with new firmware
[See also it](../../UnitUnified/UnitCardKB/PlotToSerial)
