/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitCardKB/UnitCardKB2/UnitFacesQWERTY/UnitTab5Keyboard
  Serial output is always produced; an on-screen display is drawn when an LCD is present.
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <M5HAL.hpp>
#include <M5Utility.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // wiring::addI2C / addUART / i2cClass / i2cBusHandle / failStop
#include <cctype>
#include <string>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_CARDKB) && !defined(USING_UNIT_CARDKB2) && !defined(USING_UNIT_FACES_QWERTY) && \
    !defined(USING_UNIT_TAB5_KEYBOARD)
// For CardKB
// #define USING_UNIT_CARDKB

// For CardKB2
// #define USING_UNIT_CARDKB2
#if defined(USING_UNIT_CARDKB2)
// Choose one communication mode for CardKB2
// For I2C
// #define USING_I2C_FOR_CARDKB2
// For UART
// #define USING_UART_FOR_CARDKB2
#endif

// For FacesQWERTY
// #define USING_UNIT_FACES_QWERTY

// For Tab5 Keyboard (built into M5Stack Tab5)
// #define USING_UNIT_TAB5_KEYBOARD
#endif
// *************************************************************

namespace {
auto& lcd = M5.Display;
LGFX_Sprite canvas(&lcd);
bool has_lcd{};

const char* special_key_name(const char ch)
{
    switch (ch) {
        case '\b':
            return "BS";
        case '\t':
            return "TAB";
        case '\n':
            return "LF";
        case '\r':
            return "CR";
        case 0x1B:
            return "ESC";
        case 0x7F:
            return "DEL";
        default:
            break;
    }
#if defined(USING_UNIT_CARDKB)
    using namespace m5::unit;
    switch (ch) {
        case UnitCardKB::SCHAR_LEFT:
            return "LEFT";
        case UnitCardKB::SCHAR_UP:
            return "UP";
        case UnitCardKB::SCHAR_DOWN:
            return "DOWN";
        case UnitCardKB::SCHAR_RIGHT:
            return "RIGHT";
        default:
            break;
    }
#elif defined(USING_UNIT_CARDKB2)
    using namespace m5::unit::cardkb2;
    switch (ch) {
        case SCHAR_LEFT:
            return "LEFT";
        case SCHAR_UP:
            return "UP";
        case SCHAR_DOWN:
            return "DOWN";
        case SCHAR_RIGHT:
            return "RIGHT";
        default:
            break;
    }
#elif defined(USING_UNIT_FACES_QWERTY)
    using namespace m5::unit;
    switch (ch) {
        case UnitFacesQWERTY::SCHAR_UP:
            return "UP";
        case UnitFacesQWERTY::SCHAR_INS:
            return "INS";
        case UnitFacesQWERTY::SCHAR_HOME:
            return "HOME";
        case UnitFacesQWERTY::SCHAR_END:
            return "END";
        case UnitFacesQWERTY::SCHAR_PAGE_UP:
            return "PGUP";
        case UnitFacesQWERTY::SCHAR_PAGE_DOWN:
            return "PGDN";
        case UnitFacesQWERTY::SCHAR_LEFT:
            return "LEFT";
        case UnitFacesQWERTY::SCHAR_DOWN:
            return "DOWN";
        case UnitFacesQWERTY::SCHAR_RIGHT:
            return "RIGHT";
        case UnitFacesQWERTY::SCHAR_SPEAKER:
            return "SPK";
        default:
            break;
    }
#endif
    return nullptr;
}
m5::unit::UnitUnified Units;
#if defined(USING_UNIT_CARDKB)
#pragma message "Using UnitCardKB (I2C)"
m5::unit::UnitCardKB unit;
#elif defined(USING_UNIT_CARDKB2)
#if defined(USING_UART_FOR_CARDKB2)
#pragma message "Using UnitCardKB2UART (UART)"
m5::unit::UnitCardKB2UART unit;
#else
#pragma message "Using UnitCardKB2 (I2C)"
m5::unit::UnitCardKB2 unit;
#endif
#elif defined(USING_UNIT_FACES_QWERTY)
#pragma message "Using UnitFacesQWERTY (I2C)"
m5::unit::UnitFacesQWERTY unit;
#elif defined(USING_UNIT_TAB5_KEYBOARD)
#pragma message "Using UnitTab5Keyboard (I2C)"
m5::unit::UnitTab5Keyboard unit;
#else
#error Must choose unit define, USING_UNIT_CARDKB, USING_UNIT_CARDKB2, USING_UNIT_FACES_QWERTY, or USING_UNIT_TAB5_KEYBOARD
#endif

bool small_display{};
#if defined(USING_UNIT_CARDKB) || defined(USING_UNIT_FACES_QWERTY)
bool scan_mode{};
#endif
std::string str{};
#if defined(USING_UNIT_CARDKB2)
#if defined(USING_UART_FOR_CARDKB2)
const char* const mode_label = "UART";
#else
const char* const mode_label = "I2C";
#endif
#else
const char* const mode_label = "Conventional";
#endif

#if !defined(USING_UART_FOR_CARDKB2) && !defined(USING_UNIT_TAB5_KEYBOARD)
// Tab5 Keyboard uses ExtPort1 (G0/G1) — its own setup path, not setup_i2c().
// Guard prevents -Wunused-function in Tab5-only builds.
// wiring::addI2C is board-aware: NessoN1 -> SoftwareI2C (GROVE on port_b), NanoC6/NanoH2 -> Ex_I2C,
// others -> Wire on port_a. Same one-liner on Arduino and ESP-IDF native.
// (Background: skills m5-i2c-three-way-branch / m5-i2c-c6-constraint.)
bool setup_i2c()
{
    return m5::unit::wiring::addI2C(Units, unit) && Units.begin();
}
#endif  // !defined(USING_UART_FOR_CARDKB2) && !defined(USING_UNIT_TAB5_KEYBOARD)

#if defined(USING_UNIT_CARDKB)
bool setup_cardkb()
{
    if (!setup_i2c()) {
        return false;
    }
    M5.Log.printf("Hardware:%02X Firmware:%02X\n", unit.hardwareType(), unit.firmwareVersion());
    return true;
}
#endif  // defined(USING_UNIT_CARDKB)

#if defined(USING_UNIT_CARDKB2) && !defined(USING_UART_FOR_CARDKB2)
bool setup_cardkb2_i2c()
{
    if (!setup_i2c()) {
        return false;
    }
    M5.Log.printf("Firmware:%02X\n", unit.firmwareVersion());
    return true;
}
#endif  // defined(USING_UNIT_CARDKB2) && !defined(USING_UART_FOR_CARDKB2)

#if defined(USING_UART_FOR_CARDKB2)
bool setup_cardkb2_uart()
{
    // UART mode: CardKB2 must be switched to UART mode first (Fn+Sym+2 on the device).
    // NOTE: setExtPower does not fully reset CardKB2 (Sym state may persist). Only works on boards
    // with AXP power management (Core2, CoreS3). Press RST on CardKB2 if Sym LED remains.
    M5.Power.setExtPower(false);
    m5::utility::delay(100);
    M5.Power.setExtPower(true);
    m5::utility::delay(100);
    // wiring::addUART selects the board UART pins (PortC preferred, PortA fallback) and the right
    // HardwareSerial / uart_port on both Arduino and ESP-IDF native.
    if (!m5::unit::wiring::addUART(Units, unit, 115200) || !Units.begin()) {
        return false;
    }
    M5.Log.printf("Firmware:Unknown (UART mode)\n");
    return true;
}
#endif  // defined(USING_UART_FOR_CARDKB2)

#if defined(USING_UNIT_FACES_QWERTY)
bool setup_faces()
{
    // FacesQWERTY connects via M-BUS (internal I2C), not GROVE
    if (!Units.add(unit, M5.In_I2C) || !Units.begin()) {
        return false;
    }
    M5.Log.printf("FacesType:%02X Firmware:%02X\n", unit.facesType(), unit.firmwareVersion());
    return true;
}
#endif  // defined(USING_UNIT_FACES_QWERTY)

#if defined(USING_UNIT_TAB5_KEYBOARD)
// ============================================================================
// Tab5 Keyboard operation mode.
// The mode is cycled at runtime with BtnA: Normal -> HID -> Character -> Normal.
// ============================================================================
struct Tab5ModeEntry {
    m5::unit::tab5_keyboard::Mode mode;
    const char* label;
};
constexpr Tab5ModeEntry TAB5_MODES[] = {
    {m5::unit::tab5_keyboard::Mode::Normal, "Normal"},
    {m5::unit::tab5_keyboard::Mode::HID, "HID"},
    {m5::unit::tab5_keyboard::Mode::Character, "Character"},
};
size_t tab5_mode_index = 0;  // index into TAB5_MODES; advanced by BtnA in loop_tab5_keyboard()

// Tab5 Keyboard connects via ExtPort1 (10-pin internal connector) on M5Stack Tab5.
//
// Tab5 ExtPort1 (J9) signal pins per schematic + M5Tab5-UserDemo BSP:
//   INT = GPIO50 (J9 pin 10), confirmed via TAB5_TCA8418_INT_PIN -- handled by config_t default
//   SDA = GPIO0  (J9 pin 7),  confirmed via hardware
//   SCL = GPIO1  (J9 pin 8),  confirmed via hardware
constexpr int8_t TAB5_KEYBOARD_SDA = 0;
constexpr int8_t TAB5_KEYBOARD_SCL = 1;

bool setup_tab5_keyboard()
{
    // Configure initial mode via config_t before Units.begin().
    // INT pin defaults to 50 (Tab5 ExtPort1 J9 pin 10); override irq_pin only if wiring differs.
    {
        auto cfg = unit.config();
        cfg.mode = TAB5_MODES[tab5_mode_index].mode;  // starts at Normal (index 0)
        // Effective in Normal mode only: enables library-side software auto-repeat.
        // HID and Character modes ignore this flag (repeat is owned by device firmware).
        cfg.software_repeat = true;
        unit.config(cfg);
    }

    M5_LOGI("Tab5 ExtPort1 I2C: SDA:%d SCL:%d", TAB5_KEYBOARD_SDA, TAB5_KEYBOARD_SCL);
#if defined(ARDUINO)
    Wire.end();
    Wire.begin(TAB5_KEYBOARD_SDA, TAB5_KEYBOARD_SCL, unit.component_config().clock);
    if (!Units.add(unit, Wire) || !Units.begin()) {
        return false;
    }
#else
    // ESP-IDF native (5.2+): hardware I2C bus on the fixed ExtPort1 pins (NOT SoftwareI2C).
    auto bus =
        m5::unit::wiring::i2cBusHandle(I2C_NUM_0, static_cast<gpio_num_t>(TAB5_KEYBOARD_SDA),
                                       static_cast<gpio_num_t>(TAB5_KEYBOARD_SCL), unit.component_config().clock);
    if (!Units.add(unit, bus) || !Units.begin()) {
        return false;
    }
#endif

    // begin() applies cfg.mode and (when start_periodic is true) enables the matching INT
    // and starts draining events, so no manual writeInterruptEnable()/startPeriodicMeasurement().
    M5.Log.printf("Firmware:%02X\n", unit.firmwareVersion());
    M5_LOGI("Tab5 keyboard mode: %s", TAB5_MODES[tab5_mode_index].label);
    return true;
}
#endif  // defined(USING_UNIT_TAB5_KEYBOARD)

#if defined(USING_UNIT_CARDKB)
void loop_cardkb(bool& dirty, char& ch)
{
    // Toggle behavior if using M5Unit-KEYBOARD firmware
    if (unit.firmwareVersion() && M5.BtnA.wasClicked()) {
        scan_mode = !scan_mode;
        unit.writeMode(scan_mode ? m5::unit::keyboard::Mode::M5UnitUnified : m5::unit::keyboard::Mode::Conventional);
        M5.Log.printf("======== Change behavior %s mode\n", scan_mode ? "M5UnitUnified" : "Conventional");
        if (has_lcd) {
            canvas.fillScreen(0);
        }
        str   = "";
        dirty = true;
    }

    // Gets the input characters
    if (unit.updated()) {
        while (unit.available()) {
            ch         = unit.getchar();
            auto sname = special_key_name(ch);
            M5.Log.printf("Char:[%02X %s]\n", (uint8_t)ch, sname ? sname : m5::utility::formatString("%c", ch).c_str());
            M5.Speaker.tone(1000, 20);
            if (std::isprint(ch)) {
                str += ch;
            } else if (ch == '\r' || ch == '\n') {
                str += '\n';
            } else if (ch == 0x08 && !str.empty()) {
                str.pop_back();
            }
            dirty = true;
            unit.discard();
        }
    }

    dirty = dirty || (unit.nowBits() != unit.previousBits());

    if (scan_mode) {
        // For Scan mode

        // Any keys state
        auto s = m5::utility::formatString(" Any:%u/%u %u/%u %u/%u %u", unit.isPressed(), unit.wasPressed(),
                                           unit.isReleased(), unit.wasReleased(), unit.isHolding(), unit.wasHold(),
                                           unit.isRepeating());
        if (has_lcd && !small_display) {
            canvas.drawString(s.c_str(), 0, 0);
        }
        // M5_LOGI("%s", s.c_str());

        // Specific key index
        auto kidx = unit.toKeyIndex('g');
        s         = m5::utility::formatString("idxG:%u/%u %u/%u %u/%u %u", unit.isPressed(kidx), unit.wasPressed(kidx),
                                              unit.isReleased(kidx), unit.wasReleased(kidx), unit.isHolding(kidx),
                                              unit.wasHold(kidx), unit.isRepeating(kidx));
        if (has_lcd && !small_display) {
            canvas.drawString(s.c_str(), 0, 16);
        }
        // M5_LOGI("%s", s.c_str());

        // Specific character
        constexpr char sch = '+';
        s = m5::utility::formatString(" Ch+:%u/%u %u/%u %u/%u %u", unit.isPressed(sch), unit.wasPressed(sch),
                                      unit.isReleased(sch), unit.wasReleased(sch), unit.isHolding(sch),
                                      unit.wasHold(sch), unit.isRepeating(sch));
        if (has_lcd && !small_display) {
            canvas.drawString(s.c_str(), 0, 16 * 2);
        }
        // M5_LOGI("%s", s.c_str());

        // Modifier key
        static auto prev_mod = unit.modifierBits();
        auto mod             = unit.modifierBits();
        if (has_lcd && mod != prev_mod) {
            uint16_t left = small_display ? 0 : 19 * 8;

            canvas.setCursor(left, 0);
            canvas.fillRect(left, 0, canvas.width() - left, 16);

            if (unit.isModifier()) {
                if (unit.isShift()) {
                    canvas.print("S ");
                }
                if (unit.isSymbol()) {
                    canvas.print("Sy ");
                }
                if (unit.isFunction()) {
                    canvas.print("Fn ");
                }
                if (unit.isAlt()) {
                    canvas.print("A ");
                }
            }
        }
        prev_mod = mod;

        // Now bits
        if (has_lcd) {
            if (small_display) {
                canvas.setCursor(0, 16 * 4);
                canvas.printf("%016llX", unit.nowBits());
            } else {
                canvas.setCursor(0, 16 * 3);
                canvas.printf(" NOW:%016llX", unit.nowBits());
            }
        }
        // M5_LOGI("NOW:%016llX", unit.nowBits());

#if 1
        // API check
        if (ch) {
            auto kidx = unit.toKeyIndex(ch);
            if (!unit.isPressed(kidx)) {
                M5_LOGE("library error(k) %02X", ch);
            }
            if (!unit.isPressed(ch)) {
                M5_LOGE("library error(ch) %02X", ch);
            }
        }
#endif

    } else if (has_lcd) {
        canvas.drawString(mode_label, 0, 0);
    }
}
#endif  // defined(USING_UNIT_CARDKB)

#if defined(USING_UNIT_CARDKB2) && !defined(USING_UART_FOR_CARDKB2)
void loop_cardkb2_i2c(bool& dirty, char& ch)
{
    // Gets the input characters
    if (unit.updated()) {
        while (unit.available()) {
            ch         = unit.getchar();
            auto sname = special_key_name(ch);
            M5.Log.printf("Char:[%02X %s]\n", (uint8_t)ch, sname ? sname : m5::utility::formatString("%c", ch).c_str());
            M5.Speaker.tone(1000, 20);
            if (std::isprint(ch)) {
                str += ch;
            } else if (ch == '\r' || ch == '\n') {
                str += '\n';
            } else if (ch == 0x08 && !str.empty()) {
                str.pop_back();
            }
            dirty = true;
            unit.discard();
        }
    }

    if (has_lcd) {
        canvas.drawString(mode_label, 0, 0);
    }
}
#endif  // defined(USING_UNIT_CARDKB2) && !defined(USING_UART_FOR_CARDKB2)

#if defined(USING_UART_FOR_CARDKB2)
void loop_cardkb2_uart(bool& dirty, char& ch)
{
    // Gets the input characters
    if (unit.updated()) {
        while (unit.available()) {
            ch         = unit.getchar();
            auto sname = special_key_name(ch);
            M5.Log.printf("Char:[%02X %s]\n", (uint8_t)ch, sname ? sname : m5::utility::formatString("%c", ch).c_str());
            M5.Speaker.tone(1000, 20);
            if (std::isprint(ch)) {
                str += ch;
            } else if (ch == '\r' || ch == '\n') {
                str += '\n';
            } else if (ch == 0x08 && !str.empty()) {
                str.pop_back();
            }
            dirty = true;
            unit.discard();
        }
    }

    dirty = dirty || (unit.nowBits() != unit.previousBits());

    // Any keys state
    auto s =
        m5::utility::formatString(" Any:%u/%u %u/%u %u/%u %u", unit.isPressed(), unit.wasPressed(), unit.isReleased(),
                                  unit.wasReleased(), unit.isHolding(), unit.wasHold(), unit.isRepeating());
    if (has_lcd && !small_display) {
        canvas.drawString(s.c_str(), 0, 0);
    }

    // Specific key index
    auto kidx = unit.toKeyIndex('g');
    s         = m5::utility::formatString("idxG:%u/%u %u/%u %u/%u %u", unit.isPressed(kidx), unit.wasPressed(kidx),
                                          unit.isReleased(kidx), unit.wasReleased(kidx), unit.isHolding(kidx),
                                          unit.wasHold(kidx), unit.isRepeating(kidx));
    if (has_lcd && !small_display) {
        canvas.drawString(s.c_str(), 0, 16);
    }

    // Specific character
    constexpr char sch = '+';
    s = m5::utility::formatString(" Ch+:%u/%u %u/%u %u/%u %u", unit.isPressed(sch), unit.wasPressed(sch),
                                  unit.isReleased(sch), unit.wasReleased(sch), unit.isHolding(sch), unit.wasHold(sch),
                                  unit.isRepeating(sch));
    if (has_lcd && !small_display) {
        canvas.drawString(s.c_str(), 0, 16 * 2);
    }

    // Modifier key
    static auto prev_mod = unit.modifierBits();
    auto mod             = unit.modifierBits();
    if (has_lcd && mod != prev_mod) {
        uint16_t left = small_display ? 0 : 19 * 8;

        canvas.setCursor(left, 0);
        canvas.fillRect(left, 0, canvas.width() - left, 16);

        if (unit.isModifier()) {
            if (unit.isShift()) {
                canvas.print("S ");
            }
            if (unit.isSymbol()) {
                canvas.print("Sy ");
            }
            if (unit.isFunction()) {
                canvas.print("Fn ");
            }
            if (unit.isAlt()) {
                canvas.print("A ");
            }
        }
    }
    prev_mod = mod;

    // Now bits
    if (has_lcd) {
        if (small_display) {
            canvas.setCursor(0, 16 * 4);
            canvas.printf("%016llX", unit.nowBits());
        } else {
            canvas.setCursor(0, 16 * 3);
            canvas.printf(" NOW:%016llX", unit.nowBits());
        }
    }
}
#endif  // defined(USING_UART_FOR_CARDKB2)

#if defined(USING_UNIT_FACES_QWERTY)
void loop_faces(bool& dirty, char& ch)
{
    // Toggle behavior if using M5Unit-KEYBOARD firmware
    if (unit.firmwareVersion() && M5.BtnA.wasClicked()) {
        scan_mode = !scan_mode;
        unit.writeMode(scan_mode ? m5::unit::keyboard::Mode::M5UnitUnified : m5::unit::keyboard::Mode::Conventional);
        M5.Log.printf("======== Change behavior %s mode\n", scan_mode ? "M5UnitUnified" : "Conventional");
        if (has_lcd) {
            canvas.fillScreen(0);
        }
        str   = "";
        dirty = true;
    }

    // Gets the input characters
    if (unit.updated()) {
        while (unit.available()) {
            ch         = unit.getchar();
            auto sname = special_key_name(ch);
            M5.Log.printf("Char:[%02X %s]\n", (uint8_t)ch, sname ? sname : m5::utility::formatString("%c", ch).c_str());
            M5.Speaker.tone(1000, 20);
            if (std::isprint(ch)) {
                str += ch;
            } else if (ch == '\r' || ch == '\n') {
                str += '\n';
            } else if (ch == 0x08 && !str.empty()) {
                str.pop_back();
            }
            dirty = true;
            unit.discard();
        }
    }

    dirty = dirty || (unit.nowBits() != unit.previousBits());

    if (scan_mode) {
        // For Scan mode

        // Any keys state
        auto s = m5::utility::formatString(" Any:%u/%u %u/%u %u/%u %u", unit.isPressed(), unit.wasPressed(),
                                           unit.isReleased(), unit.wasReleased(), unit.isHolding(), unit.wasHold(),
                                           unit.isRepeating());
        if (has_lcd && !small_display) {
            canvas.drawString(s.c_str(), 0, 0);
        }

        // Specific key index
        auto kidx = unit.toKeyIndex('g');
        s         = m5::utility::formatString("idxG:%u/%u %u/%u %u/%u %u", unit.isPressed(kidx), unit.wasPressed(kidx),
                                              unit.isReleased(kidx), unit.wasReleased(kidx), unit.isHolding(kidx),
                                              unit.wasHold(kidx), unit.isRepeating(kidx));
        if (has_lcd && !small_display) {
            canvas.drawString(s.c_str(), 0, 16);
        }

        // Specific character
        constexpr char sch = '+';
        s = m5::utility::formatString(" Ch+:%u/%u %u/%u %u/%u %u", unit.isPressed(sch), unit.wasPressed(sch),
                                      unit.isReleased(sch), unit.wasReleased(sch), unit.isHolding(sch),
                                      unit.wasHold(sch), unit.isRepeating(sch));
        if (has_lcd && !small_display) {
            canvas.drawString(s.c_str(), 0, 16 * 2);
        }

        // Modifier key
        static auto prev_mod = unit.modifierBits();
        auto mod             = unit.modifierBits();
        if (has_lcd && mod != prev_mod) {
            uint16_t left = small_display ? 0 : 19 * 8;

            canvas.setCursor(left, 0);
            canvas.fillRect(left, 0, canvas.width() - left, 16);

            if (unit.isModifier()) {
                if (unit.isShift()) {
                    canvas.print("S ");
                }
                if (unit.isSymbol()) {
                    canvas.print("Sy ");
                }
                if (unit.isFunction()) {
                    canvas.print("Fn ");
                }
                if (unit.isAlt()) {
                    canvas.print("A ");
                }
            }
        }
        prev_mod = mod;

        // Now bits
        if (has_lcd) {
            if (small_display) {
                canvas.setCursor(0, 16 * 4);
                canvas.printf("%016llX", unit.nowBits());
            } else {
                canvas.setCursor(0, 16 * 3);
                canvas.printf(" NOW:%016llX", unit.nowBits());
            }
        }

#if 1
        // API check
        if (ch) {
            auto kidx = unit.toKeyIndex(ch);
            if (!unit.isPressed(kidx)) {
                M5_LOGE("library error(k) %02X", ch);
            }
            if (!unit.isPressed(ch)) {
                M5_LOGE("library error(ch) %02X", ch);
            }
        }
#endif

    } else if (has_lcd) {
        canvas.drawString(mode_label, 0, 0);
    }
}
#endif  // defined(USING_UNIT_FACES_QWERTY)

#if defined(USING_UNIT_TAB5_KEYBOARD)
// Tab5 event log layout: top 16 px is the static MODE label, the rest is a
// scrollable log of incoming events. When the log cursor reaches the bottom
// edge, the log region scrolls up by one line via setScrollRect() + scroll().
// All drawing targets the off-screen `canvas` sprite; pushSprite() runs once
// per loop iteration so the scroll never produces visible flicker.
constexpr int TAB5_LOG_TOP     = 16;
constexpr int TAB5_LINE_HEIGHT = 16;  // matches AsciiFont8x16

void tab5_log_println(const char* line)
{
    int cy = canvas.getCursorY();
    if (cy < TAB5_LOG_TOP) {
        canvas.setCursor(0, TAB5_LOG_TOP);
    } else if (cy + TAB5_LINE_HEIGHT > canvas.height()) {
        canvas.setScrollRect(0, TAB5_LOG_TOP, canvas.width(), canvas.height() - TAB5_LOG_TOP);
        canvas.scroll(0, -TAB5_LINE_HEIGHT);
        canvas.fillRect(0, canvas.height() - TAB5_LINE_HEIGHT, canvas.width(), TAB5_LINE_HEIGHT);
        canvas.setCursor(0, canvas.height() - TAB5_LINE_HEIGHT);
    }
    canvas.printf("%s\n", line);
}

// Redraw the static MODE label in the top 16 px and reset the log cursor.
// Only ever called from inside has_lcd guards.
void draw_tab5_mode_label()
{
    canvas.fillRect(0, 0, canvas.width(), 16);
    const auto s = m5::utility::formatString("MODE: %s", TAB5_MODES[tab5_mode_index].label);
    canvas.drawString(s.c_str(), 0, 0);
    canvas.setCursor(0, TAB5_LOG_TOP);  // log starts below the static MODE label
}

void loop_tab5_keyboard(bool& dirty, char& ch)
{
    (void)ch;  // Tab5 Keyboard reports matrix coordinates / HID codes / chars, not unit::getchar().

    // BtnA cycles Normal -> HID -> Character -> Normal.
    if (M5.BtnA.wasClicked()) {
        tab5_mode_index = (tab5_mode_index + 1) % (sizeof(TAB5_MODES) / sizeof(TAB5_MODES[0]));
        unit.writeMode(TAB5_MODES[tab5_mode_index].mode);  // resets state + re-points INT to new mode
        M5.Log.printf("======== Tab5 mode -> %s\n", TAB5_MODES[tab5_mode_index].label);
        if (has_lcd) {
            canvas.fillScreen(0);  // clear the scrolling log
            draw_tab5_mode_label();
            dirty = true;
        }
    }

    // Draw the MODE label once on first iteration.
    static bool s_mode_label_drawn = false;
    if (has_lcd && !s_mode_label_drawn) {
        s_mode_label_drawn = true;
        draw_tab5_mode_label();
        dirty = true;
    }

    // Lower area: drain all pending events from the unified queue.
    unit.update();
    while (!unit.empty()) {
        const auto evt = unit.oldest();
        std::string line;
        switch (evt.type) {
            case m5::unit::tab5_keyboard::EventType::Key: {
                // (row, col) → ASCII via the bitwise modifier state tracked
                // inside UnitTab5Keyboard. Only meaningful in Normal mode.
                const char key_ch = unit.keyMatrixToChar(evt.key.row, evt.key.col);
                if (key_ch != 0 && std::isprint(static_cast<unsigned char>(key_ch))) {
                    M5_LOGI("[Key]  row=%u col=%u %s%s [%c]", evt.key.row, evt.key.col,
                            evt.key.pressed ? "PRESSED" : "RELEASED", evt.repeat ? " (R)" : "", key_ch);
                } else {
                    M5_LOGI("[Key]  row=%u col=%u %s%s", evt.key.row, evt.key.col,
                            evt.key.pressed ? "PRESSED" : "RELEASED", evt.repeat ? " (R)" : "");
                }
                if (has_lcd) {
                    line = m5::utility::formatString("Key: r=%u c=%u %s%s", evt.key.row, evt.key.col,
                                                     evt.key.pressed ? "DOWN" : "UP", evt.repeat ? " (R)" : "");
                }
                break;
            }
            case m5::unit::tab5_keyboard::EventType::Hid: {
                const char hid_ch = m5::unit::tab5_keyboard::hidUsageToChar(evt.hid.keycode, evt.modifier);
                if (hid_ch != 0 && std::isprint(static_cast<unsigned char>(hid_ch))) {
                    M5_LOGI("[HID]  modifier=0x%02X keycode=0x%02X [%c]", evt.modifier, evt.hid.keycode, hid_ch);
                    if (has_lcd) {
                        line = m5::utility::formatString("HID: mod=0x%02X kc=0x%02X [%c]", evt.modifier,
                                                         evt.hid.keycode, hid_ch);
                    }
                } else {
                    M5_LOGI("[HID]  modifier=0x%02X keycode=0x%02X", evt.modifier, evt.hid.keycode);
                    if (has_lcd) {
                        line = m5::utility::formatString("HID: mod=0x%02X kc=0x%02X", evt.modifier, evt.hid.keycode);
                    }
                }
                break;
            }
            case m5::unit::tab5_keyboard::EventType::Character:
                M5_LOGI("[Char] modifier=0x%02X length=%u chars=\"%s\"", evt.modifier, evt.chr.length, evt.chr.chars);
                if (has_lcd) {
                    line = m5::utility::formatString("Char: mod=0x%02X \"%s\"", evt.modifier, evt.chr.chars);
                }
                break;
            case m5::unit::tab5_keyboard::EventType::None:
            default:
                unit.discard();
                continue;
        }
        if (has_lcd) {
            tab5_log_println(line.c_str());
            dirty = true;
        }
        unit.discard();
    }
}
#endif  // defined(USING_UNIT_TAB5_KEYBOARD)

}  // namespace

using namespace m5::unit::keyboard;

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    has_lcd = (lcd.width() > 0);

    if (has_lcd) {
        // The screen shall be in landscape mode
        if (lcd.height() > lcd.width()) {
#if defined(USING_UNIT_TAB5_KEYBOARD)
            lcd.setRotation(3);
#else
            lcd.setRotation(1);
#endif
        }

        small_display = lcd.width() < 240;
        lcd.fillScreen(TFT_LIGHTGRAY);
    }

    bool unit_ready{};
#if defined(USING_UART_FOR_CARDKB2)
    unit_ready = setup_cardkb2_uart();
#elif defined(USING_UNIT_CARDKB2)
    unit_ready = setup_cardkb2_i2c();
#elif defined(USING_UNIT_CARDKB)
    unit_ready = setup_cardkb();
#elif defined(USING_UNIT_FACES_QWERTY)
    unit_ready = setup_faces();
#elif defined(USING_UNIT_TAB5_KEYBOARD)
    unit_ready = setup_tab5_keyboard();
#endif

    if (!unit_ready) {
        M5_LOGE("Failed to begin");
#if defined(USING_UNIT_CARDKB2)
        // CardKB2 remembers its communication mode across power cycles.
        // If using I2C, ensure the device is in I2C mode (Fn+Sym+1).
        // If using UART, ensure the device is in UART mode (Fn+Sym+2).
        M5_LOGE("Check CardKB2 communication mode (Fn+Sym+1:I2C, Fn+Sym+2:UART)");
#endif
        m5::unit::wiring::failStop();
    }
    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    if (has_lcd) {
        canvas.setPsram(false);
        canvas.setColorDepth(1);
        if (!canvas.createSprite(lcd.width(), lcd.height())) {
            M5_LOGE("Failed to create sprite");
            m5::unit::wiring::failStop();
        }
        canvas.setFont(&fonts::AsciiFont8x16);
        canvas.fillScreen(TFT_DARKGREEN);
        canvas.pushSprite(0, 0);
    }
}

void loop()
{
    static bool initial{true};
    bool dirty{initial};
    initial = false;
    char ch{};

    M5.update();
    Units.update();

    auto prev_str = str.size();

#if defined(USING_UART_FOR_CARDKB2)
    loop_cardkb2_uart(dirty, ch);
#elif defined(USING_UNIT_CARDKB2)
    loop_cardkb2_i2c(dirty, ch);
#elif defined(USING_UNIT_CARDKB)
    loop_cardkb(dirty, ch);
#elif defined(USING_UNIT_FACES_QWERTY)
    loop_faces(dirty, ch);
#elif defined(USING_UNIT_TAB5_KEYBOARD)
    loop_tab5_keyboard(dirty, ch);
#endif

    if (has_lcd) {
        // String
        if (str.size() != prev_str) {
            auto top = small_display ? 16 * 5 : canvas.height() >> 1;
            canvas.fillRect(0, top, canvas.width(), canvas.height() - top);
            canvas.setCursor(0, top);
            canvas.printf("%s", str.c_str());
        }

        // Character
        if (ch) {
            uint32_t scale = ((canvas.height() >> 1) - 16) / 16;
            canvas.setTextSize(scale, scale);
            canvas.setTextDatum(middle_center);

            auto x = (canvas.width() - scale * 8);
            auto y = scale * 16 / 2 + 16;
            canvas.fillRect(x - scale * 8, y - scale * 8, scale * 16, scale * 16);

            if (std::isprint(ch)) {
                canvas.drawString(m5::utility::formatString("%c", ch).c_str(), x, y);

            } else {
                canvas.drawString(m5::utility::formatString("%02X", ch).c_str(), x, y);
            }
            canvas.setTextSize(1, 1);
            canvas.setTextDatum(top_left);
        }

        if (dirty) {
            canvas.pushSprite(0, 0);
        }
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core targets (e.g. CardKB on C3/C6/H2): feed IDLE every 2 s without per-iteration yields.
// Dual-core SoCs (S3/Tab5/Core 系) compile this out; IDLE runs on the other core.
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS   = 2000;
    constexpr TickType_t FEED_SLEEP_TICKS = pdMS_TO_TICKS(5);
    static uint32_t s_next_feed_ms        = 0;
    const uint32_t now_ms                 = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    if (now_ms >= s_next_feed_ms) {
        s_next_feed_ms = now_ms + FEED_INTERVAL_MS;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
