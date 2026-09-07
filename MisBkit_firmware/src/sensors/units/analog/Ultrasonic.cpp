/**
 * @file UltrasonicUnit.cpp
 * @author Etienne Montenegro
 * @brief Implementation of UltrasonicUnit.h
 */

#include "sensors/units/analog/UltrasonicUnit.h"
#include <ArduinoLog.h>

UltrasonicUnit::UltrasonicUnit(PortC& port, UnitModel _model) : _port(port), Unit(_model){
}

bool UltrasonicUnit::begin_impl() {
    pinMode(_port.pins[0],OUTPUT);
    pinMode(_port.pins[1],INPUT);
    
    set_enabled(true);
    
    return true;
}

bool UltrasonicUnit::sample_impl(uint32_t now_ms) {

   switch(phase) 
  {
    case 1:
    digitalWrite(_port.pins[0], HIGH);
    phase = 2;
    nextTick = micros() + 10;
    break;

  case 2:
    digitalWrite(_port.pins[0], LOW);
    phase = 3;
    nextTick = micros();
    break;

  case 3:
    // Wait for pin to go high
    if (digitalRead(_port.pins[1]) != HIGH)
    {
      // 1 second
      if (micros() - 1000000 > nextTick)
      {
        phase = 1;
        // timed out
        return false;
      }

      // true bc didn't error
      return true;
    }
    // Pin is high
    // Save time in nextTick

    nextTick = micros();
    phase = 4;

    return true;
    break;
  
  case 4:  
  
    // Wait for pin to go low
    if (digitalRead(_port.pins[1])) return false;

    // Pin is low
    // Get time

    unsigned long time = micros() - nextTick;

    phase = 1;

    float distance = float(time) / 29.0f / 2.0f;

    if (distance > 1900.0f)
    {
        // out of range, clamp
        distance = 1900.0f;
    }
    // filter and store in _filtered (preserve raw port values)
    float filtered = apply_lowpass(map_to_float(distance, 0, 1900), _filtered[0]);
    _filtered[0] = filtered;
    return true;
    break;
  }
   
    return true;
}


void UltrasonicUnit::write_json(JsonObject& dst) const {
    Unit::write_json(dst);           // id, state
    dst["val"].add(_filtered[0]);
}
