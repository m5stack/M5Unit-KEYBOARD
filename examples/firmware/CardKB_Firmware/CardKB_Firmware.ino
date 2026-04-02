/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Firmware for CardKB that adds key pressed scan mode

  Note that different types of boards are eligible for writing
    CardKB (SKU:U035)        : ATmega328P
    CardKB v1.1 (SKU:U035-B) : ATmega8A

  Required: Adafruit_NeoPixel https://github.com/adafruit/Adafruit_NeoPixel

  ArduinoIDE settings
    - [Tool] - [Board]      SKU:U035 "ATmega328", SKU:U035-B "ATmega8"
    - [Tool] - [Clock]      "Internal 8 MHz"
    - [Tool] - [Programmer] Device to write the firmware you use (Arduino as ISP, USbasp, ... )

  Use command
    - [Skeych] - [Upload Using Programmer]
*/
#include <Adafruit_NeoPixel.h>
constexpr uint8_t PIN{13};
constexpr uint8_t NUMPIXELS{1};
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#include <Wire.h>
#include <stdint.h>

// **** MUST CHOOSE TARGET ******************************
// SKU:U035-B
// #define FIRMWARE_TARGET_CARDKB_V11
// SKU:U035
// #define FIRMWARE_TARGET_CARDKB
// ******************************************************
#if !defined(FIRMWARE_TARGET_CARDKB) && !defined(FIRMWARE_TARGET_CARDKB_V11)
#error Chhose target please!
#endif

#if defined(FIRMWARE_TARGET_CARDKB_V11)
#if defined(ARDUINO_AVR_ATmega8)  // For CardKB v1.1
#pragma message "CardKB v1.1 ATmega8, Are you sure?"
constexpr uint8_t HARDWARE_TYPE{0x11};
#else
#error Invalid board setting
#endif
#endif

#if defined(FIRMWARE_TARGET_CARDKB)
#if defined(ARDUINO_AVR_ATmega328)  // For CardKB
#pragma message "CardKB ATmega328, Are you sure?"
constexpr uint8_t HARDWARE_TYPE{0x01};
#else
#error Invalid board setting
#endif
#endif

#if !defined(F_CPU) || F_CPU != 8000000L
#error "Clock setting must be Internal 8MHz"
#endif

constexpr uint8_t I2C_ADDR{0x5F};
constexpr uint8_t FIRMWARE_VERSION{0x01};

constexpr uint8_t NUMBER_OF_KEYS{48};
//        d0   d1     d2 d3 d4 d5 d6 d7 d8 d9 d10 d11
// A3:    esc  1      2  3  4  5  6  7  8  9  0   del
// A2:    tab  q      w  e  r  t  y  u  i  o  p
// A1:    left up     a  s  d  f  g  h  j  k  l   enter
// A0:    down right  z  x  c  v  b  n  m  ,  .   space
// shift: d12
// sym:   d15
// fn:    d14
uint8_t key_map[NUMBER_OF_KEYS][4 /*normal, shift, sym,fn */] = {
    /* A3 */
    {27, 27, 27, 128},     // esc
    {'1', '1', '!', 129},  // 1
    {'2', '2', '@', 130},  // 2
    {'3', '3', '#', 131},  // 3
    {'4', '4', '$', 132},  // 4
    {'5', '5', '%', 133},  // 5
    {'6', '6', '^', 134},  // 6
    {'7', '7', '&', 135},  // 7
    {'8', '8', '*', 136},  // 8
    {'9', '9', '(', 137},  // 9
    {'0', '0', ')', 138},  // 0
    {8, 127, 8, 139},      // del
    /* A2 */
    {9, 9, 9, 140},         // tab
    {'q', 'Q', '{', 141},   // q
    {'w', 'W', '}', 142},   // w
    {'e', 'E', '[', 143},   // e
    {'r', 'R', ']', 144},   // r
    {'t', 'T', '/', 145},   // t
    {'y', 'Y', '\\', 146},  // y
    {'u', 'U', '|', 147},   // u
    {'i', 'I', '~', 148},   // i
    {'o', 'O', '\'', 149},  // o
    {'p', 'P', '"', 150},   // p
    {0, 0, 0, 0},           // no key
    /* A1 */
    {180, 180, 180, 152},  // LEFT
    {181, 181, 181, 153},  // UP
    {'a', 'A', ';', 154},  // a
    {'s', 'S', ':', 155},  // s
    {'d', 'D', '`', 156},  // d
    {'f', 'F', '+', 157},  // f
    {'g', 'G', '-', 158},  // g
    {'h', 'H', '_', 159},  // h
    {'j', 'J', '=', 160},  // j
    {'k', 'K', '?', 161},  // k
    {'l', 'L', 0, 162},    // l
    {13, 13, 13, 163},     // enter
    /* A0 */
    {182, 182, 182, 164},  // DOWN
    {183, 183, 183, 165},  // RIGHT
    {'z', 'Z', 0, 166},    // z
    {'x', 'X', 0, 167},    // x
    {'c', 'C', 0, 168},    // c
    {'v', 'V', 0, 169},    // v
    {'b', 'B', 0, 170},    // b
    {'n', 'N', 0, 171},    // n
    {'m', 'M', 0, 172},    // m
    {',', ',', '<', 173},  //,
    {'.', '.', '>', 174},  //.
    {' ', ' ', ' ', 175}   // space
};

constexpr uint8_t shift_bit{0x10};
constexpr uint8_t symbol_bit{0x80};
constexpr uint8_t function_bit{0x40};

// Protocol
constexpr uint8_t CMD_SCAN{0x10};                  // R 7 bytes
constexpr uint8_t CMD_MODE{0x20};                  // R/W 1 byte
constexpr uint8_t CMD_HARDWARE_TYPE_REG{0xFD};     // R 1 byte
constexpr uint8_t CMD_FIRMWARE_VERSION_REG{0xFE};  // R 1 byte

constexpr uint8_t NUMBER_OF_KEY_STATUS_BYTE{(NUMBER_OF_KEYS + 7) / 8 + 1};
uint8_t key_bits[2][NUMBER_OF_KEY_STATUS_BYTE]{};  // key status + alt key status
uint8_t current{};                                 // write target
uint8_t pressed{}, released{};
uint8_t mode{}, released_mode{};  // 0:normal 1:shift 2:sym 3:fn
uint8_t mode_lock{};
uint8_t cmd{};
uint8_t scan_mode{};  // 0:Nearly compatible with the old 1: scan mode
uint32_t idle{};
uint32_t led_table[4]{};

constexpr uint8_t mode_to_modifier_bit_table[4] = {
    0,
    shift_bit,
    symbol_bit,
    function_bit,
};

// Make the modifier key bits common as they vary from unit to unit
constexpr uint8_t mode_to_common_modifier_bit_table[4] = {
    0,
    0x01,
    0x02,
    0x04,
};

inline void key_bits_on(const uint8_t cur, const uint8_t idx)
{
    key_bits[cur][idx >> 3] |= (1U << (idx & 0x07));
}

inline bool is_key_bits(const uint8_t cur, const uint8_t idx)
{
    return key_bits[cur][idx >> 3] & (1U << (idx & 0x07));
}

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
ModButton mod_buttuns[3];  // 0:shift,1:sym,2:Fn

void flush(const uint32_t clr, uint32_t times = 3, const uint32_t delayTime = 20)
{
    while (times--) {
        pixels.setPixelColor(0, clr);
        pixels.show();
        delay(delayTime);
        pixels.setPixelColor(0, 0);
        pixels.show();
        delay(delayTime);
    }
}

void requestEvent()
{
    if (cmd) {
        if (cmd >= CMD_SCAN && cmd < CMD_SCAN + NUMBER_OF_KEY_STATUS_BYTE) {
            for (int i = cmd - CMD_SCAN; i < NUMBER_OF_KEY_STATUS_BYTE; ++i) {
                // Write previous state  (Firmware writes and buffers for transmission are exclusive)
                Wire.write(key_bits[current ^ 1][i]);
            }

        } else if (cmd == CMD_MODE) {
            Wire.write(scan_mode);
        } else if (cmd == CMD_FIRMWARE_VERSION_REG) {
            Wire.write(FIRMWARE_VERSION);
        } else if (cmd == CMD_HARDWARE_TYPE_REG) {
            Wire.write(HARDWARE_TYPE);
        }
        cmd = 0;
        return;
    }

    // Compatible with conventional behavior
    if (!scan_mode && released) {
        Wire.write(key_map[released - 1][released_mode]);
        if (!mode_lock) {
            mode = released_mode = 0;
        }
        released = 0;
    }
}

void receiveEvent(int num)
{
    if (!cmd) {
        cmd = Wire.read();
        if (cmd == CMD_MODE && num == 2) {
            scan_mode = Wire.read() ? 1 : 0;
            cmd       = 0;
            pressed = released = mode = mode_lock = released_mode = idle = 0;
            // flush(pixels.Color(0, scan_mode ? 3 : 0, scan_mode ? 0 : 3, 4));  // Green:to new Blue: to old
        }
    }
}

bool get_key_status()
{
    uint16_t bits{};
    uint16_t pd{}, pb{};

    // Scan and store key pressed bits
    for (int a = 0; a < 4; ++a) {
        // Final states are A3:H A2:H A1:H A0:L
        digitalWrite(A3, (a == 0) ? LOW : HIGH);
        digitalWrite(A2, (a == 1) ? LOW : HIGH);
        digitalWrite(A1, (a == 2) ? LOW : HIGH);
        digitalWrite(A0, (a == 3) ? LOW : HIGH);
        delay(2);

        pd   = PIND;
        pb   = PINB;
        bits = ~((pb << 8) | pd) & 0x0FFF;  // Use inverted 12 bits
        for (uint_fast8_t bidx = 0; bidx < 12; ++bidx) {
            if (bits & (1U << bidx)) {
                key_bits_on(current, 12 * a + bidx);
                if (!pressed) {
                    released = 0;
                    pressed  = 12 * a + bidx + 1;
                }
            }
        }
    }

    // Released the first pressed key? (for old mode)
    if (!released && pressed && !is_key_bits(current, (pressed - 1))) {
        released      = pressed;
        released_mode = mode;
        pressed       = 0;
    }
    return pressed != 0;
}

void setup()
{
    pinMode(A3, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A0, OUTPUT);
    digitalWrite(A0, HIGH);
    digitalWrite(A1, LOW);
    digitalWrite(A2, LOW);
    digitalWrite(A3, LOW);
    DDRB  = 0x00;
    PORTB = 0xff;
    DDRD  = 0x00;
    PORTD = 0xff;

    // Make color table
    led_table[0] = pixels.Color(0, 0, 0);  // Black
    led_table[1] = pixels.Color(3, 0, 0);  // Red
    led_table[2] = pixels.Color(0, 3, 0);  // Green
    led_table[3] = pixels.Color(0, 0, 3);  // Blue

    pixels.begin();

    Wire.begin(I2C_ADDR);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);

    digitalWrite(A3, HIGH);
    digitalWrite(A2, HIGH);
    digitalWrite(A1, HIGH);
    digitalWrite(A0, LOW);
    delay(2);

    for (int i = 1; i < 4; i++) {
        flush(led_table[i]);
    }
    pixels.setPixelColor(0, 0);
    pixels.show();
}

void loop()
{
    memset(key_bits[current], 0, sizeof(key_bits[0]));

    // Detect modifier keys
    // Must be A3:H A2:H A1:H A0:L
    uint8_t mod = PINB;
    uint8_t tmp_mod_bits{};
    mod = ~mod & (shift_bit | symbol_bit | function_bit);

    // Simultaneous Alt presses take precedence over Shift/Sym/Fun in that order
    for (int_fast8_t i = 2; i >= 0; --i) {
        uint8_t m    = i + 1;
        uint8_t mbit = mode_to_modifier_bit_table[m];
        tmp_mod_bits |= (mod & mbit) ? mode_to_common_modifier_bit_table[m] : 0;
        mod_buttuns[i].setRawState(idle, mod & mbit);
        if (mod_buttuns[i].wasReleased()) {
            mode      = scan_mode ? 0 : ((mode == m) ? 0 : m);  // cancel or change mode (old)
            mode_lock = 0;
        } else if (mod_buttuns[i].wasDoubleClicked()) {
            mode      = m;
            mode_lock = 1;  // mode lock
        }
    }
    key_bits[current][NUMBER_OF_KEY_STATUS_BYTE - 1] = mode ? mode_to_common_modifier_bit_table[mode] : tmp_mod_bits;

    // LED for mode
    // Lighting is mode locked (old, new)
    // Flashing is apply alt to next clicked key (old)
    if (scan_mode) {
        pixels.setPixelColor(0, led_table[mode_lock ? mode : 0]);
    } else {
        pixels.setPixelColor(0, led_table[(!mode_lock && (idle / 6) % 2 == 1) ? 0 : mode]);
    }

    // Scan key state ( delay(2) * 4 in get_key_status)
    if (get_key_status()) {
        // Pressed any key?
        pixels.setPixelColor(0, pixels.Color(2, 2, 2));
    }
    pixels.show();

    // If there is a released key and ALT is applied, clear the mode (old)
    if (!scan_mode && released && !mode_lock) {
        mode = 0;
    }

    // Swap scan buffer for writing
    current ^= 1;

    ++idle;
    delay(1);  // About 9ms delay + process time per 1 loop
}
