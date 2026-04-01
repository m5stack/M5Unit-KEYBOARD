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
bool scan_mode{};
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

#if defined(USING_UART_FOR_CARDKB2)
    // UART mode: CardKB2 must be switched to UART mode first (Fn+Sym+2 on the device)
    // Port C primary, Port A fallback
    auto pin_num_rx = M5.getPin(m5::pin_name_t::port_c_rxd);
    auto pin_num_tx = M5.getPin(m5::pin_name_t::port_c_txd);
    if (pin_num_rx < 0 || pin_num_tx < 0) {
        M5_LOGW("PortC is not available, using PortA");
        Wire.end();
        pin_num_rx = M5.getPin(m5::pin_name_t::port_a_pin1);
        pin_num_tx = M5.getPin(m5::pin_name_t::port_a_pin2);
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
#else
    // I2C (CardKB, CardKB2 I2C, FacesQWERTY)
    auto board = M5.getBoard();
    bool unit_ready{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        auto pb_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pb_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pb_sda, pb_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pb_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pb_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        unit_ready = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        M5_LOGI("Using M5.Ex_I2C");
        unit_ready = Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);
        unit_ready = Units.add(unit, Wire) && Units.begin();
    }

    if (!unit_ready) {
#endif
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
#if defined(USING_UNIT_CARDKB)
    M5.Log.printf("Hardware:%02X Firmware:%02X\n", unit.hardwareType(), unit.firmwareVersion());
#endif
#if defined(USING_UNIT_CARDKB2)
    if (unit.firmwareVersion()) {
        M5.Log.printf("Firmware:%02X\n", unit.firmwareVersion());
    } else {
        M5.Log.printf("Firmware:Unknown (UART mode)\n");
    }
#endif
#if defined(USING_UNIT_FACES_QWERTY)
    M5.Log.printf("FacesType:%02X Firmware:%02X\n", unit.facesType(), unit.firmwareVersion());
#endif

    lcd.setFont(&fonts::AsciiFont8x16);
    lcd.startWrite();
    lcd.fillScreen(0);
}

void loop()
{
    bool dirty{};

    M5.update();
    Units.update();

#if !defined(USING_UNIT_CARDKB2)
    // Toggle behavior if using M5Unit-KEYBOARD firmware
    if (unit.firmwareVersion() && M5.BtnA.wasClicked()) {
        scan_mode = !scan_mode;
        unit.writeMode(scan_mode ? Mode::M5UnitUnified : Mode::Conventional);
        lcd.fillScreen(0);
        str   = "";
        dirty = true;
    }
#endif
    auto prev_str = str.size();

    // Gets the input characters
    // Depending on the mode, whether the character is released or pressed, the behavior changes if using
    // M5Unit-KEYBOARD firmware
    char ch{};
    if (unit.updated()) {
#if defined(USING_UNIT_CARDKB2) && !defined(USING_UART_FOR_CARDKB2)
        // I2C: UnitCardKB2 — single getchar per update
        ch = unit.getchar();
        if (ch) {
            M5.Log.printf("Char:[0x%02X=%d %c]\n", ch, ch, std::isprint(ch) ? ch : ' ');
            if (std::isprint(ch)) {
                str += ch;
            } else if (ch == '\r' || ch == '\n') {
                str += '\n';
            } else if (ch == 0x08 && !str.empty()) {
                str.pop_back();
            }
            dirty = true;
        }
#else
        // UART / CardKB / FacesQWERTY: buffered
        while (unit.available()) {
            ch = unit.getchar();
            M5.Log.printf("Char:[0x%02X=%d %c]\n", ch, ch, std::isprint(ch) ? ch : ' ');
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
#endif
    }

#if !defined(USING_UNIT_CARDKB2) || defined(USING_UART_FOR_CARDKB2)
    dirty = dirty || (unit.nowBits() != unit.previousBits());
#endif

#if !defined(USING_UNIT_CARDKB2)
    if (scan_mode) {
        // For Scan mode

        // Any keys state
        auto s = m5::utility::formatString(" Any:%u/%u %u/%u %u/%u %u", unit.isPressed(), unit.wasPressed(),
                                           unit.isReleased(), unit.wasReleased(), unit.isHolding(), unit.wasHold(),
                                           unit.isRepeating());
        if (!small_display) {
            lcd.drawString(s.c_str(), 0, 0);
        }

        // Specific key index
        auto kidx = unit.toKeyIndex('g');
        s         = m5::utility::formatString("idxG:%u/%u %u/%u %u/%u %u", unit.isPressed(kidx), unit.wasPressed(kidx),
                                              unit.isReleased(kidx), unit.wasReleased(kidx), unit.isHolding(kidx),
                                              unit.wasHold(kidx), unit.isRepeating(kidx));
        if (!small_display) {
            lcd.drawString(s.c_str(), 0, 16);
        }

        // Specific character
        constexpr char sch = '+';
        s = m5::utility::formatString(" Ch+:%u/%u %u/%u %u/%u %u", unit.isPressed(sch), unit.wasPressed(sch),
                                      unit.isReleased(sch), unit.wasReleased(sch), unit.isHolding(sch),
                                      unit.wasHold(sch), unit.isRepeating(sch));
        if (!small_display) {
            lcd.drawString(s.c_str(), 0, 16 * 2);
        }

        // Modifier key
        static auto prev_mod = unit.modifierBits();
        auto mod             = unit.modifierBits();
        if (mod != prev_mod) {
            uint16_t left = small_display ? 0 : 19 * 8;

            lcd.setCursor(left, 0);
            lcd.fillRect(left, 0, lcd.width() - left, 16);

            if (unit.isModifier()) {
                if (unit.isShift()) {
                    lcd.print("S ");
                }
                if (unit.isSymbol()) {
                    lcd.print("Sy ");
                }
                if (unit.isFunction()) {
                    lcd.print("Fn ");
                }
                if (unit.isAlt()) {
                    lcd.print("A ");
                }
            }
        }
        prev_mod = mod;

        // Now bits
        if (small_display) {
            lcd.setCursor(0, 16 * 4);
            lcd.printf("%016llX", unit.nowBits());
        } else {
            lcd.setCursor(0, 16 * 3);
            lcd.printf(" NOW:%016llX", unit.nowBits());
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
#endif
        lcd.drawString(mode_label, 0, 0);
#if !defined(USING_UNIT_CARDKB2)
    }
#endif

    // String
    if (str.size() != prev_str) {
        auto top = small_display ? 16 * 5 : lcd.height() >> 1;
        lcd.fillRect(0, top, lcd.width(), lcd.height() - top);
        lcd.setCursor(0, top);
        lcd.printf("%s", str.c_str());
    }

    // Character
    if (ch) {
        uint32_t scale = ((lcd.height() >> 1) - 16) / 16;
        lcd.setTextSize(scale, scale);
        lcd.setTextDatum(middle_center);

        auto x = (lcd.width() - scale * 8);
        auto y = scale * 16 / 2 + 16;
        lcd.fillRect(x - scale * 8, y - scale * 8, scale * 16, scale * 16);

        if (std::isprint(ch)) {
            lcd.drawString(m5::utility::formatString("%c", ch).c_str(), x, y);

        } else {
            lcd.drawString(m5::utility::formatString("%02X", ch).c_str(), x, y);
        }
        lcd.setTextSize(1, 1);
        lcd.setTextDatum(top_left);
    }

    if (dirty) {
        lcd.display();
        lcd.waitDisplay();
    }
}
