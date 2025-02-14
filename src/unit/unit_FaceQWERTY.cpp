/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_FaceQWERTY.cpp
  @brief Face QWERTY Unit for M5UnitUnified
*/
#include "unit_FaceQWERTY.hpp"

using namespace m5::utility::mmh3;
using namespace m5::unit::types;

namespace m5 {
namespace unit {

// class UnitFaceQWERTY
const char UnitFaceQWERTY::name[] = "UnitFaceQWERTY";
const types::uid_t UnitFaceQWERTY::uid{"UnitFaceQWERTY"_mmh3};
const types::uid_t UnitFaceQWERTY::attr{0};

bool UnitFaceQWERTY::begin()
{
    // Waiting for Firmware setup to finish
    m5::utility::delay(600);
    return UnitKeyboard::begin();
}

void UnitFaceQWERTY::update(const bool force)
{
    UnitKeyboard::update(force);
    //  Enter key is returned by 2 bytes of [0x0D, 0X0A] from Firmware
    if (released() == 0x0D) {
        uint8_t discard{};
        readWithTransaction(&discard, 1);  // Discard 0x0A
    }
}

}  // namespace unit
}  // namespace m5
