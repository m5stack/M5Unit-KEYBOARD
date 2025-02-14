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

}  // namespace unit
}  // namespace m5
