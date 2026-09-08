// Copied from MisBkit_firmware/src/Led.h (Etienne Montenegro)
// Adapted: no functional changes.
#ifndef ACCELBOT_LED_H
#define ACCELBOT_LED_H

#include <Arduino.h>

namespace led {

void init();

void setColor(uint8_t r, uint8_t g, uint8_t b, bool force = false);

void blinkColor(uint8_t r, uint8_t g, uint8_t b, int time, int count);
void blinkDigit(uint8_t digit);
void blinkNumber(uint16_t number);
void blinkIP(IPAddress ip);
void error();
void off();
void update();
}  // namespace led

#endif // ACCELBOT_LED_H
