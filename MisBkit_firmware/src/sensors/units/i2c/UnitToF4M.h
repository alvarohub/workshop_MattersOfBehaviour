#pragma once
#ifndef MBK_UNIT_ToF4M_H
#define MBK_UNIT_ToF4M_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include <VL53L1X.h>

#include "sensors/units/i2c/UnitI2C.h"

class UnitVL53L1X : public UnitI2C {
  static constexpr uint8_t k_address = 0x29;
 static constexpr u16_t k_max_distance = 500;
  VL53L1X _sensor;
  float _distance = 0.0f;

public:

  UnitVL53L1X()           : UnitI2C() {}
  UnitVL53L1X(TwoWire& w) : UnitI2C(w) {}

  bool begin_impl() override {
    _logger.setPrefix(prefix_print);
    _sensor.setAddress(k_address);
    _sensor.setBus(_wire);
    if(!_sensor.init()){
      _connected= false;
      return false;
    }
    _sensor.stopContinuous();
    _connected = true;
    _sensor.setDistanceMode(VL53L1X::Short);
    _sensor.setMeasurementTimingBudget(20000);

  // Start continuous readings at a rate of one measurement every 50 ms (the
  // inter-measurement period). This period should be at least as long as the
  // timing budget.
    _sensor.startContinuous(20);
    return _connected;
  }

  bool sample_impl(uint32_t now_ms) override {
    
    if(_sensor.last_status ==2){
      _connected = false;
    }
    if(_sensor.ranging_data.range_status == VL53L1X::RangeStatus::HardwareFail){
      _connected = false;
    }
          
          
    if (!ensure_connected(now_ms)) {
      _logger.traceln("Not connected");
      return false;
    }
    
    if(!_sensor.dataReady()){
        return false;
    }

    uint16_t d = _sensor.read(false);
    
    if(d == 0) {
        return false;
    }
    
    float sample = float(d);
    if (_sensor.ranging_data.range_status == VL53L1X::RangeStatus::OutOfBoundsFail ||
        _sensor.ranging_data.range_status == VL53L1X::RangeStatus::WrapTargetFail ){
          sample = k_max_distance;
        }
    _distance = apply_lowpass(map_to_float(sample, 0.0F, static_cast<float>(k_max_distance)), _distance);

    //_logger.traceln("distance : %F",_distance);
    return true;
  }



  void write_json(JsonObject& dst) const override
  {
    Unit::write_json(dst);
    dst["val"].add(_distance);
  }

  static void prefix_print(Print* _logOutput, int logLevel){
      _logOutput->printf("[Unit - ToF4M] ");
    }
};

#endif  //MBK_UNIT_ToF4M_H