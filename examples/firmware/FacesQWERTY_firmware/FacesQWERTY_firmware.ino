/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Firmware for FacesQWERTY that adds key pressed scan mode

  ArduinoIDE settings
    - [Tool] - [Board]     "Arduino Pro or Pro mini"
    - [Tool] - Processor]  "Atmega328P (3.3V, 8Mhz)"
    - [Tool] - [Programmer] Device to write the firmware you use (Arduino as ISP, USbasp, ... )

  Use command
    - [Skeych] - [Upload Using Programmer]
*/
#include <Wire.h>
#include <stdint.h>

#if !defined(ARDUINO_AVR_PRO)
#error Board must be Arduino pro or Pro mini
#endif
#if !defined(__AVR_ATmega328P__)
#error mpu must be Atmega328P
#endif
#if !defined(F_CPU) || F_CPU != 8000000L
#error Clock setting must be Internal 8MHz
#endif

constexpr uint8_t I2C_ADDR{0x08};

constexpr uint8_t NUMBER_OF_KEYS{35};

#define Set_Bit(val, bitn) (val |= (1 << (bitn)))
#define Clr_Bit(val, bitn) (val &= ~(1 << (bitn)))
#define Get_Bit(val, bitn) (val & (1 << (bitn)))

// C1 PC2
// C2 PC3
// C3 PC1  except alt and enter

// R1 PB1 QA
// R2 PB0 WSZ
// R3 PD5 EDX
// R4 PD6 RFC
// R5 PD7 TGV
// R6 PD4 YHB
// R7 PD3 UJN
// R8 PD2 IKM
// R9 PD1 OL$
// R10 PD0 P+del

// PB6 Enter
// PB4 Alt
// PB3 Fn
// PB7 sym

// PB5 aA
// PC0 space

// PC0 LEDR(1) LEDL(0)
// PB2 IRQ

#define LED_L_1            \
    do {                   \
        Set_Bit(DDRC, 0);  \
        Set_Bit(PORTC, 0); \
    } while (0)
#define LED_R_1            \
    do {                   \
        Set_Bit(DDRC, 0);  \
        Clr_Bit(PORTC, 0); \
    } while (0)
#define LED_L_0            \
    do {                   \
        Clr_Bit(DDRC, 0);  \
        Clr_Bit(PORTC, 0); \
    } while (0)
#define LED_R_0            \
    do {                   \
        Clr_Bit(DDRC, 0);  \
        Clr_Bit(PORTC, 0); \
    } while (0)

#define IRQ_1 Set_Bit(PORTB, 2)
#define IRQ_0 Clr_Bit(PORTB, 2)
uint8_t ALT = 0, aA = 0, SYM = 0, FN = 0, ESC = 0;
uint8_t twoBytes = 0;
uint8_t KEY = 0, KEY2 = 0, hadPressed = 0;

#define aA_Pressed      (PINB & 0x20) != 0x20
#define SYM_Pressed     (PINB & 0x80) != 0x80
#define ALT_Pressed     (PINB & 0x10) != 0x10
#define FN_Pressed      (PINB & 0x08) != 0x08
#define ENTER_Pressed   (PINB & 0x40) != 0x40
#define aA_unPressed    (PINB & 0x20) == 0x20
#define SYM_unPressed   (PINB & 0x80) == 0x80
#define ALT_unPressed   (PINB & 0x10) == 0x10
#define FN_unPressed    (PINB & 0x08) == 0x08
#define ENTER_unPressed (PINB & 0x40) == 0x40
uint8_t LedMode = 0;  // 0->LR off.1->L on,2->L slow , 3->L fast, 4->R on,5->R slow,6->R fast,7->L R slow 8->LR Fast
uint32_t idle{};

//            d0   d1 d2 d3 d4 d5 d6 d7 d8 d9
// PORTC 1 3: P    O  I  U  Y  T  R  E  W  Q
// PORTC 12 : del  L  K  J  H  G  F  D  S  A
// PORTC  23:      $  M  N  B  V  C  X  Z
// shift: d13
// sym:   d15
// fn:    d11
// alt:   d12
// enter: d14

// Bit order to key index order
constexpr uint8_t bit_to_kidx[30] = {
    9,  8,  7,  6,  5,  2,  3,  4,  1,  0,   // Q...P
    19, 18, 17, 16, 15, 12, 13, 14, 11, 10,  // A ... del
    32, 28, 27, 26, 25, 22, 23, 24, 21, 31,  // Z ... $
};

constexpr uint8_t key_map[NUMBER_OF_KEYS][5 /* normal, shift, symbol, Fn, Alt */] = {
    //
    {'q', 'Q', '#', '~', 144},
    {'w', 'W', '1', '^', 145},
    {'e', 'E', '2', '&', 146},
    {'r', 'R', '3', '`', 147},
    {'t', 'T', '(', '<', 148},
    {'y', 'Y', ')', '>', 149},
    {'u', 'U', '_', '{', 150},
    {'i', 'I', '-', '}', 151},
    {'o', 'O', '+', '[', 152},
    {'p', 'P', '@', ']', 153},
    //
    {'a', 'A', '*', '|', 154},
    {'s', 'S', '4', '=', 155},
    {'d', 'D', '5', '\\', 156},
    {'f', 'F', '6', '%', 157},
    {'g', 'G', '/', 180, 158},
    {'h', 'H', ':', 181, 159},
    {'j', 'J', ';', 182, 160},
    {'k', 'K', '\'', 183, 161},
    {'l', 'L', '"', 184, 162},
    {8, 8, 127, 8, 163},        // del & backspace
    {255, 255, 255, 185, 164},  // alt
    //
    {'z', 'Z', '7', 186, 165},
    {'x', 'X', '8', 187, 166},
    {'c', 'C', '9', 188, 167},
    {'v', 'V', '?', 189, 168},
    {'b', 'B', '!', 190, 169},
    {'n', 'N', ',', 191, 170},
    {'m', 'M', '.', 192, 171},
    {'$', '$', 255, 193, 172},
    {13, 13, 13, 13, 173},  // enter
    //
    {255, 255, 255, 255, 174},  // aA
    {'0', '0', '0', '0', 175},
    {' ', ' ', ' ', ' ', 176},
    {255, 255, 255, 255, 177},
    {255, 255, 27, 255, 178}};

// For scan mode
constexpr uint8_t FIRMWARE_VERSION{0x01};
constexpr uint8_t FACES_TYPE{0x01};  // 0x01:QWERTY

constexpr uint8_t shift_bit{0x20};
constexpr uint8_t symbol_bit{0x80};
constexpr uint8_t function_bit{0x08};
constexpr uint8_t alt_bit{0x10};
constexpr uint8_t enter_bit{0x40};

// Protocol
constexpr uint8_t CMD_SCAN_REG{0x10};              // R 5 + 1 bytes
constexpr uint8_t CMD_MODE_REG{0x20};              // R/W 1 byte
constexpr uint8_t CMD_FACES_TYPE_REG{0xFC};        // R 1byte
constexpr uint8_t CMD_FIRMWARE_VERSION_REG{0xFE};  // R 1 byte

constexpr uint8_t NUMBER_OF_KEY_STATUS_BYTE{(NUMBER_OF_KEYS + 7) / 8 + 1};
uint8_t key_bits[2][NUMBER_OF_KEY_STATUS_BYTE]{};  // key status + alt key status
uint8_t current{};                                 // write target
uint8_t mode{};                                    // 0:normal 1:shift 2:symbol 3:function 4:alt
uint8_t mode_lock{};                               // 1:locked modifier
uint8_t cmd{};                                     // register command
uint8_t scan_mode{};                               // 0:old mode 1:Scan mode

inline void key_bits_on(const uint8_t cur, const uint8_t idx)
{
    key_bits[cur][idx >> 3] |= (1U << (idx & 0x07));
}

constexpr uint8_t mode_to_modifier_bit_table[5] = {
    0, shift_bit, symbol_bit, function_bit, alt_bit,
};

// Make the modifier key bits common as they vary from unit to unit
constexpr uint8_t mode_to_common_modifier_bit_table[5] = {
    0, 0x01, 0x02, 0x04, 0x08,
};

class ModButton {
public:
    enum button_state_t : uint8_t { state_nochange, state_clicked, state_hold, state_decide_click_count };
    bool wasDoubleClicked(void) const
    {
        return _currentState == state_decide_click_count && _clickCount == 2;
    }
    bool isPressed(void) const
    {
        return _press;
    }
    bool wasReleased(void) const
    {
        return _oldPress && !_press;
    }
    void setState(const uint32_t tm, button_state_t state)
    {
        if (_currentState == state_decide_click_count) {
            _clickCount = 0;
        }
        _lastTm          = tm;
        bool flg_timeout = (tm - _lastClicked > 20);  // About 200 ms
        switch (state) {
            case state_nochange:
                if (flg_timeout && !_press && _clickCount) {
                    if (_oldPress == 0 && _currentState == state_nochange) {
                        state = state_decide_click_count;
                    } else {
                        _clickCount = 0;
                    }
                }
                break;
            case state_clicked:
                ++_clickCount;
                _lastClicked = tm;
                break;

            default:
                break;
        }
        _currentState = state;
    }
    void setRawState(const uint32_t tm, const bool press)
    {
        button_state_t state = button_state_t::state_nochange;
        auto oldPress        = _press;
        _oldPress            = oldPress;
        if (_raw_press != press) {
            _raw_press     = press;
            _lastRawChange = tm;
        }
        if (press != (0 != oldPress)) {
            _lastChange = tm;
        }
        if (press) {
            if (!oldPress) {
                _press = 1;
            }
        } else {
            _press = 0;
            if (oldPress == 1) {
                state = button_state_t::state_clicked;
            }
        }
        setState(tm, state);
    }

private:
    uint32_t _lastTm{}, _lastChange{}, _lastRawChange{}, _lastClicked{};
    button_state_t _currentState{state_nochange};  // 0:nochange  1:click  2:hold
    bool _raw_press{};
    uint8_t _press{};  // 0:release  1:click  2:holding
    uint8_t _oldPress{};
    uint8_t _clickCount{};
};
ModButton mod_buttuns[4];  // 0:shift,1:sym,2:Fn,3:Alt

void requestEvent()
{
    if (cmd) {
        if (cmd >= CMD_SCAN_REG && cmd < CMD_SCAN_REG + NUMBER_OF_KEY_STATUS_BYTE) {
            for (int i = cmd - CMD_SCAN_REG; i < NUMBER_OF_KEY_STATUS_BYTE; ++i) {
                // Write previous state  (Firmware writes and buffers for transmission are exclusive)
                Wire.write(key_bits[current ^ 1][i]);
            }
            IRQ_1;
        } else if (cmd == CMD_MODE_REG) {
            Wire.write(scan_mode);
        } else if (cmd == CMD_FIRMWARE_VERSION_REG) {
            Wire.write(FIRMWARE_VERSION);
        } else if (cmd == CMD_FACES_TYPE_REG) {
            Wire.write(FACES_TYPE);
        }
        cmd = 0;
        return;
    }

    // Compatible with conventional behavior
    if (!scan_mode) {
        if (hadPressed == 1) {
            Wire.write(KEY);
            hadPressed = 0;
            IRQ_1;
            return;
        }
        if (twoBytes == 1) {
            Wire.write(KEY2);
            twoBytes = 0;
        }
        return;
    }
}

void receiveEvent(int num)
{
    if (!cmd) {
        cmd = Wire.read();
        if (cmd == CMD_MODE_REG && num == 2) {
            scan_mode = Wire.read() ? 1 : 0;
            cmd = mode = mode_lock = idle = 0;
            LedMode = ALT = aA = SYM = FN = ESC = twoBytes = KEY = KEY2 = hadPressed = 0;
        }
    }
}

// old
uint8_t GetInput()
{
    if (aA_Pressed) {
        FN  = 0;
        SYM = 0;
        ALT = 0;
        while (aA_Pressed) delay(1);  // release key
        if (aA == 0)                  // had not pressed
        {
            delay(200);  // detect doblue click
            if (aA_Pressed) {
                while (aA_Pressed) delay(1);
                aA      = 2;
                LedMode = 2;
            } else {
                aA      = 1;
                LedMode = 1;
            }
        } else  // had pressed
        {
            delay(200);
            if (aA_Pressed) {
                while (aA_Pressed) delay(1);
                if (aA == 2) {
                    aA = 0;
                } else {
                    LedMode = 2;
                    aA      = 2;
                }
            } else {
                aA = 0;
            }
        }
    }
    if (ALT_Pressed) {
        FN      = 0;
        SYM     = 0;
        aA      = 0;
        LedMode = 3;
        ALT     = 1;
    } else {
        ALT = 0;
    }

    if (FN_Pressed) {
        aA  = 0;
        SYM = 0;
        ALT = 0;
        while (FN_Pressed) delay(1);  // release key
        if (FN == 0)                  // had not pressed
        {
            delay(200);  // detect doblue click
            if (FN_Pressed) {
                while (FN_Pressed) delay(1);
                FN      = 2;
                LedMode = 5;
            } else {
                FN      = 1;
                LedMode = 4;
            }
        } else  // had pressed
        {
            delay(200);
            if (FN_Pressed) {
                while (FN_Pressed) delay(1);
                if (FN == 2) {
                    FN = 0;
                } else {
                    LedMode = 5;
                    FN      = 2;
                }
            } else {
                FN = 0;
            }
        }
    }
    if (SYM_Pressed) {
        aA  = 0;
        FN  = 0;
        ALT = 0;
        while (SYM_Pressed) delay(1);  // release key
        if (SYM == 0)                  // had not pressed
        {
            delay(200);  // detect doblue click
            if (SYM_Pressed) {
                while (SYM_Pressed) delay(1);
                SYM     = 2;
                LedMode = 6;
            } else {
                SYM     = 1;
                LedMode = 7;
            }
        } else  // had pressed
        {
            delay(200);
            if (SYM_Pressed) {
                while (SYM_Pressed) delay(1);
                if (SYM == 2) {
                    SYM = 0;
                } else {
                    LedMode = 6;
                    SYM     = 2;
                }
            } else {
                SYM = 0;
            }
        }
    }

    if (ENTER_Pressed) {
        while (ENTER_Pressed) delay(1);  // ENTER
        return 29;
    }

    // normal key scan
    Set_Bit(PORTC, 1);
    Set_Bit(PORTC, 3);
    Clr_Bit(PORTC, 2);
    delay(2);
    switch (PIND) {
        case 0xfe:
            while (PIND != 0xff) delay(1);
            return 9;
            break;
        case 0xfd:
            while (PIND != 0xff) delay(1);
            return 8;
            break;
        case 0xfb:
            while (PIND != 0xff) delay(1);
            return 7;
            break;
        case 0xf7:
            while (PIND != 0xff) delay(1);
            return 6;
            break;
        case 0xef:
            while (PIND != 0xff) delay(1);
            return 5;
            break;
        case 0xdf:
            while (PIND != 0xff) delay(1);
            return 2;
            break;
        case 0xbf:
            while (PIND != 0xff) delay(1);
            return 3;
            break;
        case 0x7f:
            while (PIND != 0xff) delay(1);
            return 4;
            break;
    }
    switch (PINB & 0x03) {
        case 0x01:
            while ((PINB & 0x03) != 0x03) delay(1);
            return 0;
            break;
        case 0x02:
            while ((PINB & 0x03) != 0x03) delay(1);
            return 1;
            break;
    }
    Set_Bit(PORTC, 1);
    Set_Bit(PORTC, 2);
    Clr_Bit(PORTC, 3);
    delay(2);
    switch (PIND) {
        case 0xfe:
            while (PIND != 0xff) delay(1);
            return 19;
            break;
        case 0xfd:
            while (PIND != 0xff) delay(1);
            return 18;
            break;
        case 0xfb:
            while (PIND != 0xff) delay(1);
            return 17;
            break;
        case 0xf7:
            while (PIND != 0xff) delay(1);
            return 16;
            break;
        case 0xef:
            while (PIND != 0xff) delay(1);
            return 15;
            break;
        case 0xdf:
            while (PIND != 0xff) delay(1);
            return 12;
            break;
        case 0xbf:
            while (PIND != 0xff) delay(1);
            return 13;
            break;
        case 0x7f:
            while (PIND != 0xff) delay(1);
            return 14;
            break;
    }
    switch (PINB & 0x03) {
        case 0x01:
            while ((PINB & 0x03) != 0x03) delay(1);
            return 10;
            break;
        case 0x02:
            while ((PINB & 0x03) != 0x03) delay(1);
            return 11;
            break;
    }
    Set_Bit(PORTC, 2);
    Set_Bit(PORTC, 3);
    Clr_Bit(PORTC, 1);
    delay(2);
    switch (PIND) {
        case 0xfe:
            while (PIND != 0xff) delay(1);
            return 32;
            break;
        case 0xfd:
            while (PIND != 0xff) delay(1);
            return 28;
            break;
        case 0xfb:
            while (PIND != 0xff) delay(1);
            return 27;
            break;
        case 0xf7:
            while (PIND != 0xff) delay(1);
            return 26;
            break;
        case 0xef:
            while (PIND != 0xff) delay(1);
            return 25;
            break;
        case 0xdf:
            while (PIND != 0xff) delay(1);
            return 22;
            break;
        case 0xbf:
            while (PIND != 0xff) delay(1);
            return 23;
            break;
        case 0x7f:
            while (PIND != 0xff) delay(1);
            return 24;
            break;
    }
    switch (PINB & 0x03) {
        case 0x01:
            while ((PINB & 0x03) != 0x03) delay(1);
            return 31;
            break;
        case 0x02:
            while ((PINB & 0x03) != 0x03) delay(1);
            return 21;
            break;
    }

    return 255;
}

void get_key_status()
{
    // on:set, off:clr
    static constexpr uint8_t port_table[] = {
        0x0A,  // set1, clr2, set3
        0x06,  // set1, set2, clr3
        0x0C,  // clr1, set2, set3
    };

    uint16_t bits{};
    uint16_t pd{}, pb{};

    // Scan and store key pressed bits
    for (uint_fast8_t i = 0; i < 3; ++i) {
        // Final states are Clr1 Set23
        PORTC = port_table[i] | (PORTC & 0xF1);
        delay(2);

        pd   = PIND;
        pb   = PINB;
        bits = ~((pb << 8) | pd) & 0x03FF;  // Use inverted 10 bits
        for (uint_fast8_t bidx = 0; bidx < 10; ++bidx) {
            if (bits & (1U << bidx)) {
                key_bits_on(current, bit_to_kidx[10 * i + bidx]);
            }
        }
    }
    if (~pb & enter_bit) {
        key_bits_on(current, 29);
    }
}

void setup()
{
    DDRB  = 0x04;
    PORTB = 0xfb;
    IRQ_1;

    pinMode(A3, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A0, OUTPUT);

    for (int i = 0; i < 5; i++) {
        LED_L_1;
        delay(i * 30);
        LED_R_1;
        delay(i * 30);
    }
    LED_R_0;
    DDRD  = 0x00;
    PORTD = 0xff;

    LedMode = 1;

    Wire.begin(I2C_ADDR);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);

    // For Alts
    PORTC = 0x0C | (PORTC & 0xF1);
    delay(2);
}

/*
  For new mode
  Lighting left  : shift lock
  Lighting right : symbol lock
  Flushing right : function lock
  Flushing left  : alt lock
 */
void lightning_led(const uint8_t mode)
{
    switch (mode) {
        case 1:  // Shift
            LED_L_1;
            break;
        case 2:  // Symbol
            LED_R_1;
            break;
        case 3:  // Function
            if ((idle / 10) % 2 == 1) {
                LED_R_0;
            } else {
                LED_R_1;
            }
            break;
        case 4:  // Alt
            if ((idle / 10) % 2 == 1) {
                LED_L_0;
            } else {
                LED_L_1;
            }
            break;
        default:
            LED_L_0;
            LED_R_0;
            break;
    }
}

void loop()
{
    if (scan_mode) {
        memset(key_bits[current], 0, sizeof(key_bits[0]));

        // Detect modifier keys
        // Must be PORTC 0x0C
        uint8_t mod = PINB;
        uint8_t tmp_mod_bits{};
        mod = ~mod & (shift_bit | symbol_bit | function_bit | alt_bit);

        // Simultaneous modifier keys presses take precedence over Shift/Sym/Fun/Alt in that order
        for (int_fast8_t i = 3; i >= 0; --i) {
            uint8_t m    = i + 1;
            uint8_t mbit = mode_to_modifier_bit_table[m];
            tmp_mod_bits |= (mod & mbit) ? mode_to_common_modifier_bit_table[m] : 0;
            mod_buttuns[i].setRawState(idle, mod & mbit);
            if (mod_buttuns[i].wasReleased()) {
                mode      = 0;
                mode_lock = 0;
            } else if (mod_buttuns[i].wasDoubleClicked()) {
                mode      = m;
                mode_lock = 1;  // mode lock
            }
        }
        key_bits[current][NUMBER_OF_KEY_STATUS_BYTE - 1] =
            mode ? mode_to_common_modifier_bit_table[mode] : tmp_mod_bits;

        get_key_status();  // delay 2 * 3

        if (memcmp(key_bits[current], key_bits[current ^ 1], sizeof(key_bits[0]))) {
            IRQ_0;
        }

        if (mode_lock) {
            lightning_led(mode);
        } else {
            LED_L_0;
            LED_R_0;
        }

        current ^= 1;
        ++idle;
        delay(3);  // About 9ms delay + process time per 1 loop
        return;
    }

    idle++;
    unsigned char kk = GetInput();
    if (aA == 0 && ALT == 0 && FN == 0 && SYM == 0) LedMode = 0;
    switch (LedMode) {
        case 0:
            LED_L_0;
            break;
        case 1:
            LED_L_1;
            break;
        case 2:
            if ((idle / 20) % 2 == 1) {
                LED_L_0;
            } else {
                LED_L_1;
            }
            break;
        case 3:
            if ((idle / 10) % 2 == 1) {
                LED_L_0;
            } else {
                LED_L_1;
            }
            break;
        case 4:
            LED_R_1;
            break;
        case 5:
            if ((idle / 20) % 2 == 1) {
                LED_R_0;
            } else {
                LED_R_1;
            }
            break;
        case 6:
            if ((idle / 10) % 2 == 1) {
                LED_R_0;
            } else {
                LED_R_1;
            }
            break;
        case 7:
            if ((idle / 30) % 2 == 1) {
                LED_L_1;
            } else {
                LED_R_1;
            }
            break;
        case 8:
            if ((idle / 20) % 2 == 1) {
                LED_L_1;
            } else {
                LED_R_1;
            }
            break;
    }
    if (kk == 29) {
        KEY        = 0x0d;
        twoBytes   = 1;
        KEY2       = 0x0a;  // enter
        hadPressed = 1;
        IRQ_0;
    } else if (kk < 36) {
        if (ALT > 0) {
            KEY = key_map[kk][4];
        } else {
            if (SYM > 0) {
                if (SYM == 1) SYM = 0;
                KEY = key_map[kk][2];
            } else {
                if (FN > 0) {
                    if (FN == 1) FN = 0;
                    KEY = key_map[kk][3];
                } else {
                    if (aA > 0) {
                        if (aA == 1) aA = 0;
                        KEY = key_map[kk][1];
                    } else
                        KEY = key_map[kk][0];
                }
            }
        }
        hadPressed = 1;
        IRQ_0;
    }
}
