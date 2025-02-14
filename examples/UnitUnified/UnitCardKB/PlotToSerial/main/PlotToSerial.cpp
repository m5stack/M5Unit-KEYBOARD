/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitCardKB
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <M5Utility.h>
#include <cctype>
#include <string>

using namespace m5::unit::cardkb;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitCardKB unit;
bool new_behavior{};
std::string str{};

}  // namespace

void setup()
{
    M5.begin();

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.end();
    Wire.begin(pin_num_sda, pin_num_scl, 100 * 1000U);

    auto cfg = unit.config();
    cfg.mode = Mode::Released;  // Old mode
    unit.config(cfg);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    M5_LOGI("Hardware:%02X Firmware:%02X", unit.hardwareType(), unit.firmwareVersion());

    lcd.setFont(&fonts::AsciiFont8x16);
    lcd.startWrite();
    lcd.clear(0);

    uint32_t scale = std::min(lcd.width(), lcd.height()) / 16;
    lcd.setTextSize(scale, scale);
    lcd.setTextDatum(middle_center);
}

void loop_new_firmware()
{
    using m5::unit::UnitCardKB;

    if (unit.updated()) {
        if (unit.wasPressed() || unit.wasReleased() || unit.isRepeating()) {
            // M5_LOGI("---- %u/%u", unit.availablePressed(), unit.availableReleased());

            // For pressed
            while (unit.availablePressed()) {
                auto key = unit.oldestPressed();
                // M5_LOGI("P:[%02X %c]", key, std::isprint(key) ? key : ' ');
                if (std::isprint(key)) {
                    str += key;
                } else if (key == 0x0D) {
                    str += '\n';
                }
                unit.discardPressed();
            }
            // For released
            while (unit.availableReleased()) {
                auto key = unit.oldestReleased();
                // M5_LOGI("R:[%02X %c]", key, std::isprint(key) ? key : ' ');
                unit.discardReleased();
            }
        }

        // Any keys state
        auto s =
            m5::utility::formatString("A:%u/%u %u/%u %u/%u %u", unit.isPressed(), unit.wasPressed(), unit.isReleased(),
                                      unit.wasReleased(), unit.isHolding(), unit.wasHold(), unit.isRepeating());

        // M5_LOGI("%s", s.c_str());
        lcd.drawString(s.c_str(), 0, 8);

        // Specific key index
        const auto skey = UnitCardKB::KEY_G;
        s = m5::utility::formatString("G:%u/%u %u/%u %u/%u %u", unit.isPressed(skey), unit.wasPressed(skey),
                                      unit.isReleased(skey), unit.wasReleased(skey), unit.isHolding(skey),
                                      unit.wasHold(skey), unit.isRepeating(skey));
        // M5_LOGI("%s", s.c_str());
        lcd.drawString(s.c_str(), 0, 8 + 16);

        // Specific charcter
        const int ch = '+';
        s            = m5::utility::formatString("+:%u/%u %u/%u %u/%u %u", unit.isPressed(ch), unit.wasPressed(ch),
                                                 unit.isReleased(ch), unit.wasReleased(ch), unit.isHolding(ch), unit.wasHold(ch),
                                                 unit.isRepeating(ch));
        // M5_LOGI("%s", s.c_str());
        lcd.drawString(s.c_str(), 0, 8 + 16 * 2);

        // alt key
        lcd.setCursor(16 * 8, 8);
        lcd.fillRect(16 * 8, 8, 8 * 9, 16);
        if (unit.isAlt()) {
            if (unit.isShift()) {
                lcd.print("Sh ");
            }
            if (unit.isSymbol()) {
                lcd.print("Sym");
            }
            if (unit.isFunction()) {
                lcd.print("Fn ");
            }
        }

        // bits
        lcd.setCursor(16 * 8, 8 + 16);
        lcd.printf("%016llX", unit.nowBits());

        // input string
        lcd.setCursor(0, 8 + 16 * 3);
        lcd.printf("%s", str.c_str());
    }
}

void loop_old_firmware()
{
    if (unit.updated()) {
        uint8_t key = unit.released();

        static bool can_print{};

        auto prev = can_print;
        can_print = std::isprint(key);

        M5.Log.printf("Key:%02X <%c>\n", key, can_print ? key : ' ');

        lcd.clear(0);

        if (can_print) {
            lcd.drawString(m5::utility::formatString("%c", key).c_str(), lcd.width() >> 1, lcd.height() >> 1);
        } else {
            lcd.drawString(m5::utility::formatString("%02X", key).c_str(), lcd.width() >> 1, lcd.height() >> 1);
        }
    }
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();

    Units.update();

    if (new_behavior) {
        loop_new_firmware();
    } else {
        loop_old_firmware();
    }

    // Toggle behavior if using unitunified firmware
    if (unit.firmwareVersion() && (M5.BtnA.wasClicked() || touch.wasClicked())) {
        new_behavior = !new_behavior;
        auto ret     = unit.writeMode(new_behavior ? Mode::Scan : Mode::Released);
        M5_LOGI("Change behavior:%u (%u)", new_behavior, ret);

        lcd.clear();
        if (new_behavior) {
            lcd.setTextSize(1, 1);
            lcd.setTextDatum(top_left);
        } else {
            uint32_t scale = std::min(lcd.width(), lcd.height()) / 16;
            lcd.setTextSize(scale, scale);
            lcd.setTextDatum(middle_center);
        }
    }
}
