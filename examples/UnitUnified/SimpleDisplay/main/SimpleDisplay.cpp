/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Display example using M5UnitUnified for UnitCardKB/UnitFacesQWERTY
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <M5Utility.h>
#include <cctype>
#include <string>

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

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
#if defined(USING_UNIT_CARDKB)
m5::unit::UnitCardKB unit;
#elif defined(USING_UNIT_FACES_QWERTY)
m5::unit::UnitFacesQWERTY unit;
#else
#error Must choose unit define, USING_UNIT_CARDKB or USING_UNIT_FACES_QWERTY
#endif

bool small_display{};
bool scan_mode{};
std::string str{};
}  // namespace

using namespace m5::unit::keyboard;

void setup()
{
    M5.begin();

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    small_display = lcd.width() < 240;
    lcd.fillScreen(TFT_LIGHTGRAY);

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
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
    auto touch = M5.Touch.getDetail();
    Units.update();

    // Toggle behavior if using M5Unit-KEYBOARD firmware
    if (unit.firmwareVersion() && (M5.BtnA.wasClicked() || touch.wasClicked())) {
        scan_mode = !scan_mode;
        unit.writeMode(scan_mode ? Mode::M5UnitUnified : Mode::Conventional);
        lcd.fillScreen(0);
        str   = "";
        dirty = true;
    }

    auto prev_str        = str.size();
    static auto prev_mod = unit.modifierBits();

    // Gets the input characters
    // Depending on the mode, whether the character is released or pressed, the behavior changes if using
    // M5Unit-KEYBOARD firmware
    char ch{};
    if (unit.updated()) {
        while (unit.available()) {
            ch = unit.getchar();
            M5.Log.printf("Char:[%02X %c]\n", ch, std::isprint(ch) ? ch : ' ');
            if (std::isprint(ch)) {
                str += ch;
            } else if (ch == 0x0D) {
                str += '\n';
            } else if (ch == 0x08 && !str.empty()) {
                str.pop_back();
            }
            dirty = true;
            unit.discard();
        }
    }

    dirty |= (unit.nowBits() ^ unit.previousBits());

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

        // Specific charcter
        constexpr char sch = '+';
        s = m5::utility::formatString(" Ch+:%u/%u %u/%u %u/%u %u", unit.isPressed(sch), unit.wasPressed(sch),
                                      unit.isReleased(sch), unit.wasReleased(sch), unit.isHolding(sch),
                                      unit.wasHold(sch), unit.isRepeating(sch));
        if (!small_display) {
            lcd.drawString(s.c_str(), 0, 16 * 2);
        }

        // Modifier key
        auto mod = unit.modifierBits();
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
        // For Released mode
        lcd.drawString("Conventional", 0, 0);
    }

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
