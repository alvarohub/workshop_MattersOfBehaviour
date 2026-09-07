#pragma once
#ifndef MBK_UNIT_8ANGLE_H
#define MBK_UNIT_8ANGLE_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>

#include "sensors/units/i2c/UnitI2C.h"





class Unit8Angle : public UnitI2C {
    static constexpr uint8_t k_address = 0x43;
        enum: long 
    {
      Off = 0x000000,
      Red = 0xff0000,
      Green = 0x00ff00,
      Blue = 0x0000ff,
      Cyan = 0x00ffff,
      Magenta = 0xff00ff,
      Yellow = 0xffff00,
      White = 0x00ffffff
    };

    
    int _angles[8]{0};
    float _filtered_angles[8]{0};
    int _sw = false;



    bool isConnected()
    {
    _wire->beginTransmission(k_address);
    return _wire->endTransmission() == 0;
    }

// Use the standard LED RGB colours defined in M5Stack8Angle.h
// 32-bit rgbColour = 0x00rrggbb
bool setLed(int channel, long rgbColour, int brightness) 
{
  if ((unsigned int)channel > 8)
    return false;
  byte data[4];
  data[0] = rgbColour >> 16;  // red
  data[1] = rgbColour >> 8;   // green
  data[2] = rgbColour;        // blue
  data[3] = brightness;       // 0..100
  uint8_t reg = 0x30 + channel * 4;
  if(writeBytes(k_address , reg, 4, data) == WireStatus::SUCCESS){
    return true;
  }
  return false;

}

bool setAllLeds(long rgbColour, int brightness)
{
  for (int channel = 0; channel < 8; ++channel) {
    if (!setLed(channel, rgbColour, brightness))
      return false;
  }
  return true;
}

// Read potentiometer as 12-bit value, 0..4095
bool getAnalogInput12(int channel, int* value)
{
  if ((unsigned int)channel > 7)
    return false;
  byte data[2];
  WireStatus w = readBytes(k_address, channel * 2, 2, data);
 _logger.traceln("%d", w);
  if (w != WireStatus::SUCCESS)
    return false;
  *value = (data[1] << 8) | data[0];
  return true;
}

// Read potentiometer as 8-bit value, 0..255
bool getAnalogInput8(int channel, int* value)
{
  if ((unsigned int)channel > 7)
    return false;
  byte b;
  WireStatus w = readBytes(k_address, 0x10 + channel, 1, &b);
  if (w != WireStatus::SUCCESS)
    return false;
  *value = b;
  return true;
}

// Slider switch, 0=off, 1=on
bool getSwitchPosition(int* position)
{
  byte b;
  WireStatus w = readBytes(k_address,0x20, 1, &b);
  if (w != WireStatus::SUCCESS)
    return false;
  *position = b & 1;
  return true;
}

// Gets a potentiometer value as rotary switch position 
// with a given number of steps
// returns position 0 .. steps - 1
bool getRotaryPosition(int channel, int steps, int* position)
{
  if (steps > 4096)
    steps = 4096;
  int value;
  if (!getAnalogInput12(channel, &value))
    return false;
  *position = (value * steps) >> 12;
  return true;
}

bool getFwVersion(int* version)
{
  byte b;
  WireStatus w = readBytes(k_address, 0xfe, 1, &b);
  if (w != WireStatus::SUCCESS) {
    *version = -1;
    return false;
  }
  *version = b;
  return true;
}


 public:
    Unit8Angle()           : UnitI2C(){}
    Unit8Angle(TwoWire& w) : UnitI2C(w) {}

    bool begin_impl() override {
        _logger.setPrefix(&Unit8Angle::prefix_print);
        _connected =  isConnected();
        return _connected;
    }


    bool sample_impl(uint32_t now_ms) override {
        if (!ensure_connected(now_ms)) {
            return false;
        }
        
        if(!isConnected()){
            _connected = false;
            return false;
        }

        for (uint8_t i = 0; i < 8; i++) {
            getAnalogInput8(i, &_angles[i]);
           _filtered_angles[i] = apply_lowpass(map_to_float(_angles[i], 0, 255,1.0,0.0), _filtered_angles[i]);
        }
        getSwitchPosition(&_sw);
        return true;
    }


    void write_json(JsonObject& dst) const override {
         Unit::write_json(dst);
        for (size_t i = 0; i < 8; i++)
        {

            dst["val"].add(_filtered_angles[i]);
        }
        dst["val"].add(_sw);
        
       
    }
            static void prefix_print(Print* _logOutput, int logLevel){
      _logOutput->printf("[Unit - Angle8] ");
  }
};

#endif  //MBK_UNIT_8ANGLE_H