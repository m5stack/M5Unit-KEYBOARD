/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitCardKB/UnitFacesQWERTY
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

bool scan_mode{};
}  // namespace

using namespace m5::unit::keyboard;

void setup()
{
    M5.begin();

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
    lcd.fillScreen(TFT_DARKGREEN);

    // If the sound is low by default, adjust with this
    // M5.Speaker.setVolume(255);
}

void loop()
{
    M5.update();
    auto touch = M5.Touch.getDetail();
    Units.update();

    // Toggle behavior if using M5Unit-KEYBOARD firmware
    if (unit.firmwareVersion() && (M5.BtnA.wasClicked() || touch.wasClicked())) {
        scan_mode = !scan_mode;
        unit.writeMode(scan_mode ? Mode::M5UnitUnified : Mode::Conventional);
        M5.Log.printf("======== Change behavior %s mode\n", scan_mode ? "M5UnitUnified" : "Conventional");
    }

    // Gets the input characters
    // Depending on the mode, whether the character is released or pressed, the behavior changes if using
    // M5Unit-KEYBOARD firmware
    char ch{};
    if (unit.updated()) {
        while (unit.available()) {
            ch = unit.getchar();
            M5.Log.printf("Char:[%02X %c]\n", ch, std::isprint(ch) ? ch : ' ');
            M5.Speaker.tone(1000, 20);
            unit.discard();
        }
    }
}
