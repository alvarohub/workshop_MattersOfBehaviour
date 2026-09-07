#pragma once
#ifndef MBK_UNIT_ULTRASONIC_I2C_H
#define MBK_UNIT_ULTRASONIC_I2C_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>

#include "sensors/units/i2c/UnitI2C.h"
// implementation based on https://github.com/m5stack/M5Unit-Sonic/tree/master

class UltrasonicI2C : public UnitI2C
{
    //  Hardware
    static constexpr uint8_t k_address = 0x57;

    //  Cached measurements
    float    _distance   = 0.0f;

    //  Non-blocking state
    bool     _triggered  = false;
    uint32_t _trigger_ms = 0;

public:

    UltrasonicI2C()           : UnitI2C(){}
    UltrasonicI2C(TwoWire& w) : UnitI2C(w) {}

    //  Lifecycle

    bool begin_impl() override
    {   
         _logger.setPrefix(&UltrasonicI2C::prefix_print);

        _wire->beginTransmission(k_address);
        uint8_t s = _wire->endTransmission();
       
        _logger.infoln("begin status %d", s);
        if(s == 0){
            _connected = true;
        }else{
            _connected = false;
        }
        return _connected;
    }

    bool sample_impl(uint32_t now_ms) override
    {   
        if (!ensure_connected(now_ms))
        {
            _triggered = false;
            return false;
        }

        if (!_triggered)
        {
            // Phase 1: fire the measurement command and record the time.
            _wire->beginTransmission(k_address);
            _wire->write(0x01);
            _wire->endTransmission();
            //We cant check for error on transmission here. the sensor dos not reply on this command
            _trigger_ms = now_ms;
            _triggered  = true;
            return false;
        }

        if ((now_ms - _trigger_ms) < 120)
        {
            // Sensor not ready yet — come back next tick.
            return false;
        }

        // Phase 2: 120 ms have elapsed, read the result.
        
        if (!_wire->requestFrom(k_address, (uint8_t)3))
        {
            _triggered = false;
            note_disconnect(now_ms);
            return false;
        }
        uint32_t data = _wire->read();
        data <<= 8;
        data |= _wire->read();
        data <<= 8;
        data |= _wire->read();
        _triggered = false;
        float sample = float(data) / 1000.0f;
        sample = constrain(sample, 20.0, 1000.0);
        _distance = apply_lowpass(map_to_float(sample, 20, 1000), _distance);
        _logger.traceln("Distance: %F  %F", sample, _distance);
        return true;
    }



    //  Serialisation

    void write_json(JsonObject& dst) const override
    {   Unit::write_json(dst);
        dst["val"].add(_distance);
    }

    static void prefix_print(Print* _logOutput, int logLevel){
      _logOutput->printf("[Unit - UltrasonicI2C] ");
  }
};

#endif  //MBK_UNIT_ULTRASONIC_I2C_H