#pragma once
#ifndef MBK_UNIT_ToF_H
#define MBK_UNIT_ToF_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>


#include <Adafruit_VL53L0X.h>
#include "sensors/units/i2c/UnitI2C.h"

class UnitVL53L0X : public UnitI2C {
  static constexpr uint8_t k_address = 0x29;
  Adafruit_VL53L0X _tof;

  float    _range = 0.0f;

public:

  UnitVL53L0X()           : UnitI2C() {}
  UnitVL53L0X(TwoWire& w) : UnitI2C(w) {}

  bool begin_impl() override {
    if(!_tof.begin(k_address, _wire)){
      _connected= false;
      return false;
    }
    _connected = true;
    _tof.startRangeContinuous();
    return _connected;
  }

  bool sample_impl(uint32_t now_ms) override {
    if (!ensure_connected(now_ms)) {
      return false;
    }
    VL53L0X_RangingMeasurementData_t measure;
    if(!_tof.isRangeComplete()){
        return false;
    }

    uint16_t r = _tof.readRange();
    if(r == 0xffff){
        return false;
    }

    _range = apply_lowpass(float(r), _range);
    return true;
  }



  void write_json(JsonObject& dst) const override
  {   
    Unit::write_json(dst);
    dst["val"].add(_range);

  }
};

#endif  //MBK_UNIT_ToF_H