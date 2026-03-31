/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitCardKB/UnitCardKB2/UnitFacesQWERTY
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
#pragma message "Using UnitCardKB2 (UART)"
#else
#pragma message "Using UnitCardKB2 (I2C)"
#endif
m5::unit::UnitCardKB2 unit;
#elif defined(USING_UNIT_FACES_QWERTY)
#pragma message "Using UnitFacesQWERTY (I2C)"
m5::unit::UnitFacesQWERTY unit;
#else
#error Must choose unit define, USING_UNIT_CARDKB, USING_UNIT_CARDKB2, or USING_UNIT_FACES_QWERTY
#endif

bool scan_mode{};
}  // namespace

using namespace m5::unit::keyboard;

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    bool unit_ready{};
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

    M5.Power.setExtPower(false);  // Turn off port power to reset the sym status.
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
    unit_ready = Units.add(unit, serial) && Units.begin();
#else
    // I2C mode
    auto board = M5.getBoard();
    if (board == m5::board_t::board_ArduinoNessoN1) {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
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
#endif

    if (!unit_ready) {
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
#elif defined(USING_UNIT_CARDKB2)
    M5.Log.printf("Firmware:%02X\n", unit.firmwareVersion());
#elif defined(USING_UNIT_FACES_QWERTY)
    M5.Log.printf("FacesType:%02X Firmware:%02X\n", unit.facesType(), unit.firmwareVersion());
#endif
    lcd.fillScreen(TFT_DARKGREEN);

    // If the sound is low by default, adjust with this
    // M5.Speaker.setVolume(255);
}

void loop()
{
    M5.update();
    Units.update();

    // Toggle behavior if using M5Unit-KEYBOARD firmware
    if (unit.firmwareVersion() && M5.BtnA.wasClicked()) {
        scan_mode = !scan_mode;
        unit.writeMode(scan_mode ? Mode::M5UnitUnified : Mode::Conventional);
        M5.Log.printf("======== Change behavior %s mode\n", scan_mode ? "M5UnitUnified" : "Conventional");
    }

    // Gets the input characters
    // Depending on the mode, whether the character is released or pressed, the behavior changes if using
    // M5Unit-KEYBOARD firmware
    if (unit.updated()) {
        while (unit.available()) {
            char ch = unit.getchar();
            M5.Log.printf("Char:[%02X %c]\n", ch, std::isprint(ch) ? ch : ' ');
            M5.Speaker.tone(1000, 20);
            unit.discard();
        }
    }
}
