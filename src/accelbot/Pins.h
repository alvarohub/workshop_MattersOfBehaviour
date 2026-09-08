/**
 * @file Pins.h
 * @brief Pin conventions for the AccelBot project (M5StampS3, stamps3A variant).
 */
#ifndef ACCELBOT_PINS_H
#define ACCELBOT_PINS_H
#include <Arduino.h>

namespace pins {

// Dynamixel half-duplex bus (Serial0 because USB CDC occupies Serial)
const uint8_t motorsControl{41};   // direction (use 1 on plain StampS3, non-A)
const uint8_t dxlRx{44};
const uint8_t dxlTx{43};

// analog inputs
const uint8_t analogSensor1{5};
const uint8_t analogSensor2{7};
const uint8_t analogSensor3{9};
const uint8_t analogSensor4{11};

// I2C (internal bus shared with M5Unified peripherals: PMU, IMU...)
// SDA = 13, SCL = 15 on the StampS3; Wire is already started by M5.begin()

const uint8_t led{21};             // RGB status LED
const uint8_t ledStrip{5};         // WS2812 rings in series; pad labelled G1 routes to GPIO 5
const uint8_t button{0};           // BtnA

} // namespace pins
#endif // ACCELBOT_PINS_H
