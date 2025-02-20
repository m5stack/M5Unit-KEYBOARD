/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_Keyboard.cpp
  @brief Base class for units
*/
#include "unit_Keyboard.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::keyboard;
using namespace m5::unit::keyboard::command;

namespace m5 {
namespace unit {

// class UnitKeyboard
const char UnitKeyboard::name[] = "UnitKeyboard";
const types::uid_t UnitKeyboard::uid{"UnitKeyboard"_mmh3};
const types::uid_t UnitKeyboard::attr{0};

bool UnitKeyboard::begin()
{
    uint8_t discard{};
    return readWithTransaction(&discard, 1) == m5::hal::error::error_t::OK;
}

void UnitKeyboard::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            _updated = (readWithTransaction(&_released_key, 1) == m5::hal::error::error_t::OK) && (_released_key != 0);
            if (_updated) {
                _latest = at;
            }
        }
    }
}

// class UnitKeyboardBitwise
const char UnitKeyboardBitwise::name[] = "UnitKeyboardBitwise";
const types::uid_t UnitKeyboardBitwise::uid{"UnitKeyboardBitwise"_mmh3};
const types::uid_t UnitKeyboardBitwise::attr{0};

void UnitKeyboardBitwise::update(const bool force)
{
    if (_mode == Mode::Conventional) {
        UnitKeyboard::update(force);
        if (_updated) {
            _inputs->push_back(UnitKeyboard::released());
        }
    }
}

bool UnitKeyboardBitwise::readFirmwareVersion(uint8_t& ver)
{
    ver = 0;
    return readRegister8(CMD_FIRMWARE_VERSION_REG, ver, 0);
}

bool UnitKeyboardBitwise::readMode(Mode& mode)
{
    mode = Mode::Conventional;
    uint8_t v{};
    if (readRegister8(CMD_MODE_REG, v, 0)) {
        mode = static_cast<Mode>(v);
        return true;
    }
    return false;
}

bool UnitKeyboardBitwise::writeMode(const Mode mode)
{
    if (firmwareVersion() && writeRegister8(CMD_MODE_REG, m5::stl::to_underlying(mode))) {
        _mode = mode;

        _inputs->clear();
        _now = _prev = _wasPressed = _wasReleased = _wasHold = _holding = _repeating = 0;
        std::fill(_repeat_start_at.begin(), _repeat_start_at.end(), 0);
        std::fill(_hold_start_at.begin(), _hold_start_at.end(), 0);
        return true;
    }
    return false;
}

}  // namespace unit
}  // namespace m5
