#pragma once
#ifndef MBK_UNIT_DLIGHT_H
#define MBK_UNIT_DLIGHT_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>

#include <hp_BH1750.h> 
#include "sensors/units/i2c/UnitI2C.h"

class UnitDLight : public UnitI2C {
  static constexpr uint8_t k_address = 0x23;
  hp_BH1750 _sensor;

  float    _lux = 0.0f;

 public:

    UnitDLight()           : UnitI2C() {}
    UnitDLight(TwoWire& w) : UnitI2C(w) {}

    bool begin_impl() override {
      _logger.setPrefix(&UnitDLight::prefix_print);
      if(!_sensor.begin(k_address, _wire)){
        _connected= false;
        return false;
      }
      
      _sensor.calibrateTiming();
      _sensor.setQuality(BH1750_QUALITY_LOW);
      _sensor.start();
      _connected = true;
      return _connected;
    }

    bool sample_impl(uint32_t now_ms) override {
      if (!ensure_connected(now_ms)) {
        _logger.traceln("Device not reachable");
        return false;
      }

      if(!_sensor.hasValue()){
          return false;
      }

      float sample = _sensor.getLux();
      _lux = apply_lowpass(map_to_float(sample,0.11f, 1215.0f), _lux);
      if(_lux > 1.0) _lux = 1.0;
      _sensor.start();
      // _logger.traceln("Lux: %F, mapped: %F",sample, _mapped);
      return true;
    }



    void write_json(JsonObject& dst) const override {
      Unit::write_json(dst);
      dst["val"].add(_lux);
    }

    static void prefix_print(Print* _logOutput, int logLevel){
      _logOutput->printf("[Unit - DLight] ");
    }

};

#endif  //MBK_UNIT_DLIGHT_H