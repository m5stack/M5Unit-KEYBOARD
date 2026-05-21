/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_CardKB2.cpp
  @brief CardKB2 Unit for M5UnitUnified (I2C mode)
*/
#include "unit_CardKB2.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace m5 {
namespace unit {

const char UnitCardKB2::name[] = "UnitCardKB2";
const types::uid_t UnitCardKB2::uid{"UnitCardKB2"_mmh3};
const types::attr_t UnitCardKB2::attr{attribute::AccessI2C};

bool UnitCardKB2::begin()
{
    if (!UnitKeyboard::begin()) {
        M5_LIB_LOGE("============================================================");
        M5_LIB_LOGE(" CardKB2 I2C begin FAILED - device may not be in I2C mode");
        M5_LIB_LOGE(" Switch to I2C mode: press Fn+Sym+1 on the keyboard,");
        M5_LIB_LOGE(" then press the RST button on CardKB2.");
        M5_LIB_LOGE("============================================================");
        return false;
    }

    if (!readFirmwareVersion(_firmware_version)) {
        M5_LIB_LOGE("Failed to read firmware version");
        return false;
    }
    M5_LIB_LOGI("Type:%s Firmware:%02X", "CardKB2", _firmware_version);

    if (_cfg.start_periodic) {
        _periodic = true;
        _interval = _cfg.interval;
        _latest   = 0;
    }
    return true;
}

void UnitCardKB2::update(const bool force)
{
    _updated = false;
    if (!inPeriodic()) {
        return;
    }

    auto at = m5::utility::millis();
    if (!(force || !_latest || at >= _latest + _interval)) {
        return;
    }

    uint8_t raw{};
    if (readWithTransaction(&raw, 1) != m5::hal::error::error_t::OK || !raw) {
        return;
    }
    // Workaround: Filter spurious non-key values (e.g., 0x01 observed on AtomS3).
    // Valid key values: 0x08 (BS), 0x0A (LF), 0x0D (CR), 0x1B (ESC), 0x20-0x7E (printable)
    if (raw < 0x08 || (raw > 0x0D && raw < 0x1B) || (raw > 0x1B && raw < 0x20) || raw > 0x7E) {
        M5_LIB_LOGD("I2C spurious data filtered: 0x%02X", raw);
        return;
    }
    M5_LIB_LOGV("I2C raw:%02X '%c'", raw, (raw >= 0x20 && raw < 0x7F) ? raw : ' ');
    _pressed_key = raw;
    _updated     = true;
    _latest      = at;
}

bool UnitCardKB2::readFirmwareVersion(uint8_t& ver)
{
    ver = 0;
    return readRegister8(static_cast<uint8_t>(0xF1), ver, 0);
}

}  // namespace unit
}  // namespace m5
