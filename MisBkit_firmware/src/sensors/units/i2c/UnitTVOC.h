#pragma once
#ifndef MBK_UNIT_TVOC_H
#define MBK_UNIT_TVOC_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>

#include <Adafruit_SGP30.h>
#include "sensors/units/i2c/UnitI2C.h"

class UnitSGP30 : public UnitI2C
{
  //  Hardware
  static constexpr uint8_t k_address = 0x58;

  Adafruit_SGP30 _sensor;

  //  Cached measurements

  float _tvoc = 0.0f;
  float _eco2 = 0.0f;

public:

  UnitSGP30()           : UnitI2C() {}
  UnitSGP30(TwoWire& w) : UnitI2C(w) {}

  //  Lifecycle

  bool begin_impl() override
  {
    _logger.setPrefix(&UnitSGP30::prefix_print);
    if(!_sensor.begin( _wire)){
      _connected= false;
      return false;
    }

    _connected = true;
    return _connected;
  }

  bool sample_impl(uint32_t now_ms) override
  {
    if (!ensure_connected(now_ms)) {
      _logger.traceln("Not connected");
      return false;
    }

    if (! _sensor.IAQmeasure()) {
      _logger.traceln("Failed to measure");
      _connected = false;
    return false;
  }
    _tvoc = apply_lowpass(map_to_float(float(_sensor.TVOC),0, 2000), _tvoc);
    _eco2 = apply_lowpass(map_to_float(float(_sensor.eCO2),400, 3000), _eco2);
    if(_tvoc > 1.0)_tvoc =1.0;
    if(_eco2 > 1.0)_eco2 =1.0;
    //_logger.traceln("tvoc: %F eco2: %F", _tvoc, _eco2);
    return true;
  }



  //  Serialisation

  void write_json(JsonObject& dst) const override
  {
    Unit::write_json(dst);
    dst["val"].add(_tvoc);
    dst["val"].add(_eco2);

  }

  static void prefix_print(Print* _logOutput, int logLevel){
      _logOutput->printf("[Unit - TVOC] ");
  }
};

#endif  //MBK_UNIT_TVOC_H

uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}