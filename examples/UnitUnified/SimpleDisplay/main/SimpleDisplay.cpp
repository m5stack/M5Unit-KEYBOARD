/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Display example using M5UnitUnified for UnitCardKB/UnitCardKB2/UnitFacesQWERTY
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <M5HAL.hpp>
#include <M5Utility.h>
#include <cctype>
#include <string>

// *************************************************************
// Choose one define symbol to match the unit you are using
// *************************************************************
#if !defined(USING_UNIT_CARDKB) && !defined(USING_UNIT_CARDKB2) && !defined(USING_UNIT_FACES_QWERTY)
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
#endif
// *************************************************************

namespace {
auto& lcd = M5.Display;
LGFX_Sprite canvas(&lcd);

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
#else
#error Must choose unit define, USING_UNIT_CARDKB, USING_UNIT_CARDKB2, or USING_UNIT_FACES_QWERTY
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

#if !defined(USING_UART_FOR_CARDKB2)
// NessoN1: Arduino Wire (I2C_NUM_0) cannot be used for GROVE port.
//   Wire is used by M5Unified In_I2C for internal devices (IOExpander etc.).
//   Wire1 exists but is reserved for HatPort — cannot be used for GROVE.
//   Reconfiguring Wire to GROVE pins breaks In_I2C, causing ESP_ERR_INVALID_STATE in M5.update().
//   Solution: Use SoftwareI2C via M5HAL (bit-banging) for the GROVE port.
// NanoC6: Wire.begin() on GROVE pins conflicts with m5::I2C_Class registered by Ex_I2C.setPort()
//   on the same I2C_NUM_0, causing sporadic NACK errors.
//   Solution: Use M5.Ex_I2C (m5::I2C_Class) directly instead of Arduino Wire.
bool setup_i2c()
{
    auto board = M5.getBoard();
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a (which maps to Wire pins 8/10)
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        return Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
        return Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);
        return Units.add(unit, Wire) && Units.begin();
    }
}
#endif  // !defined(USING_UART_FOR_CARDKB2)

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
    // UART mode: CardKB2 must be switched to UART mode first (Fn+Sym+2 on the device)
    // Port C primary, Port A fallback (NessoN1: Port B fallback — Port A is Wire pins)
    auto board      = M5.getBoard();
    auto pin_num_rx = M5.getPin(m5::pin_name_t::port_c_rxd);
    auto pin_num_tx = M5.getPin(m5::pin_name_t::port_c_txd);
    if (pin_num_rx < 0 || pin_num_tx < 0) {
        if (board == m5::board_t::board_ArduinoNessoN1) {
            M5_LOGW("PortC is not available, using PortB");
            pin_num_rx = M5.getPin(m5::pin_name_t::port_b_in);
            pin_num_tx = M5.getPin(m5::pin_name_t::port_b_out);
        } else {
            M5_LOGW("PortC is not available, using PortA");
            Wire.end();
            pin_num_rx = M5.getPin(m5::pin_name_t::port_a_pin1);
            pin_num_tx = M5.getPin(m5::pin_name_t::port_a_pin2);
        }
    }
    M5_LOGI("getPin: RX:%d TX:%d", pin_num_rx, pin_num_tx);

    // NOTE: setExtPower does not fully reset CardKB2 (Sym state may persist).
    // Only works on boards with AXP power management (Core2, CoreS3).
    // Press RST button on CardKB2 if Sym LED remains after mode switch.
    M5.Power.setExtPower(false);
    m5::utility::delay(100);
    M5.Power.setExtPower(true);
    m5::utility::delay(100);
#if defined(CONFIG_IDF_TARGET_ESP32C6)
    auto& serial = Serial1;
#elif SOC_UART_NUM > 2
    auto& serial = Serial2;
#elif SOC_UART_NUM > 1
    auto& serial = Serial1;
#else
#error "Not enough Serial"
#endif
    serial.begin(115200, SERIAL_8N1, pin_num_rx, pin_num_tx);
    if (!Units.add(unit, serial) || !Units.begin()) {
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

#if defined(USING_UNIT_CARDKB)
void loop_cardkb(bool& dirty, char& ch)
{
    // Toggle behavior if using M5Unit-KEYBOARD firmware
    if (unit.firmwareVersion() && M5.BtnA.wasClicked()) {
        scan_mode = !scan_mode;
        unit.writeMode(scan_mode ? m5::unit::keyboard::Mode::M5UnitUnified : m5::unit::keyboard::Mode::Conventional);
        M5.Log.printf("======== Change behavior %s mode\n", scan_mode ? "M5UnitUnified" : "Conventional");
        canvas.fillScreen(0);
        str   = "";
        dirty = true;
    }

    // Gets the input characters
    if (unit.updated()) {
        while (unit.available()) {
            ch         = unit.getchar();
            auto sname = special_key_name(ch);
            M5.Log.printf("Char:[%02X %s]\n", (uint8_t)ch, sname ? sname : m5::utility::formatString("%c", ch).c_str());
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
        if (!small_display) {
            canvas.drawString(s.c_str(), 0, 0);
        }

        // Specific key index
        auto kidx = unit.toKeyIndex('g');
        s         = m5::utility::formatString("idxG:%u/%u %u/%u %u/%u %u", unit.isPressed(kidx), unit.wasPressed(kidx),
                                              unit.isReleased(kidx), unit.wasReleased(kidx), unit.isHolding(kidx),
                                              unit.wasHold(kidx), unit.isRepeating(kidx));
        if (!small_display) {
            canvas.drawString(s.c_str(), 0, 16);
        }

        // Specific character
        constexpr char sch = '+';
        s = m5::utility::formatString(" Ch+:%u/%u %u/%u %u/%u %u", unit.isPressed(sch), unit.wasPressed(sch),
                                      unit.isReleased(sch), unit.wasReleased(sch), unit.isHolding(sch),
                                      unit.wasHold(sch), unit.isRepeating(sch));
        if (!small_display) {
            canvas.drawString(s.c_str(), 0, 16 * 2);
        }

        // Modifier key
        static auto prev_mod = unit.modifierBits();
        auto mod             = unit.modifierBits();
        if (mod != prev_mod) {
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
        if (small_display) {
            canvas.setCursor(0, 16 * 4);
            canvas.printf("%016llX", unit.nowBits());
        } else {
            canvas.setCursor(0, 16 * 3);
            canvas.printf(" NOW:%016llX", unit.nowBits());
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

    } else {
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

    canvas.drawString(mode_label, 0, 0);
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
    if (!small_display) {
        canvas.drawString(s.c_str(), 0, 0);
    }

    // Specific key index
    auto kidx = unit.toKeyIndex('g');
    s         = m5::utility::formatString("idxG:%u/%u %u/%u %u/%u %u", unit.isPressed(kidx), unit.wasPressed(kidx),
                                          unit.isReleased(kidx), unit.wasReleased(kidx), unit.isHolding(kidx),
                                          unit.wasHold(kidx), unit.isRepeating(kidx));
    if (!small_display) {
        canvas.drawString(s.c_str(), 0, 16);
    }

    // Specific character
    constexpr char sch = '+';
    s = m5::utility::formatString(" Ch+:%u/%u %u/%u %u/%u %u", unit.isPressed(sch), unit.wasPressed(sch),
                                  unit.isReleased(sch), unit.wasReleased(sch), unit.isHolding(sch), unit.wasHold(sch),
                                  unit.isRepeating(sch));
    if (!small_display) {
        canvas.drawString(s.c_str(), 0, 16 * 2);
    }

    // Modifier key
    static auto prev_mod = unit.modifierBits();
    auto mod             = unit.modifierBits();
    if (mod != prev_mod) {
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
    if (small_display) {
        canvas.setCursor(0, 16 * 4);
        canvas.printf("%016llX", unit.nowBits());
    } else {
        canvas.setCursor(0, 16 * 3);
        canvas.printf(" NOW:%016llX", unit.nowBits());
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
        canvas.fillScreen(0);
        str   = "";
        dirty = true;
    }

    // Gets the input characters
    if (unit.updated()) {
        while (unit.available()) {
            ch         = unit.getchar();
            auto sname = special_key_name(ch);
            M5.Log.printf("Char:[%02X %s]\n", (uint8_t)ch, sname ? sname : m5::utility::formatString("%c", ch).c_str());
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
        if (!small_display) {
            canvas.drawString(s.c_str(), 0, 0);
        }

        // Specific key index
        auto kidx = unit.toKeyIndex('g');
        s         = m5::utility::formatString("idxG:%u/%u %u/%u %u/%u %u", unit.isPressed(kidx), unit.wasPressed(kidx),
                                              unit.isReleased(kidx), unit.wasReleased(kidx), unit.isHolding(kidx),
                                              unit.wasHold(kidx), unit.isRepeating(kidx));
        if (!small_display) {
            canvas.drawString(s.c_str(), 0, 16);
        }

        // Specific character
        constexpr char sch = '+';
        s = m5::utility::formatString(" Ch+:%u/%u %u/%u %u/%u %u", unit.isPressed(sch), unit.wasPressed(sch),
                                      unit.isReleased(sch), unit.wasReleased(sch), unit.isHolding(sch),
                                      unit.wasHold(sch), unit.isRepeating(sch));
        if (!small_display) {
            canvas.drawString(s.c_str(), 0, 16 * 2);
        }

        // Modifier key
        static auto prev_mod = unit.modifierBits();
        auto mod             = unit.modifierBits();
        if (mod != prev_mod) {
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
        if (small_display) {
            canvas.setCursor(0, 16 * 4);
            canvas.printf("%016llX", unit.nowBits());
        } else {
            canvas.setCursor(0, 16 * 3);
            canvas.printf(" NOW:%016llX", unit.nowBits());
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

    } else {
        canvas.drawString(mode_label, 0, 0);
    }
}
#endif  // defined(USING_UNIT_FACES_QWERTY)

}  // namespace

using namespace m5::unit::keyboard;

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    small_display = lcd.width() < 240;
    lcd.fillScreen(TFT_LIGHTGRAY);

    bool unit_ready{};
#if defined(USING_UART_FOR_CARDKB2)
    unit_ready = setup_cardkb2_uart();
#elif defined(USING_UNIT_CARDKB2)
    unit_ready = setup_cardkb2_i2c();
#elif defined(USING_UNIT_CARDKB)
    unit_ready = setup_cardkb();
#elif defined(USING_UNIT_FACES_QWERTY)
    unit_ready = setup_faces();
#endif

    if (!unit_ready) {
        M5_LOGE("Failed to begin");
#if defined(USING_UNIT_CARDKB2)
        // CardKB2 remembers its communication mode across power cycles.
        // If using I2C, ensure the device is in I2C mode (Fn+Sym+1).
        // If using UART, ensure the device is in UART mode (Fn+Sym+2).
        M5_LOGE("Check CardKB2 communication mode (Fn+Sym+1:I2C, Fn+Sym+2:UART)");
#endif
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    canvas.setPsram(false);
    canvas.setColorDepth(1);
    canvas.createSprite(lcd.width(), lcd.height());
    canvas.setFont(&fonts::AsciiFont8x16);
    canvas.fillScreen(TFT_DARKGREEN);
    canvas.pushSprite(0, 0);
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
#endif

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
