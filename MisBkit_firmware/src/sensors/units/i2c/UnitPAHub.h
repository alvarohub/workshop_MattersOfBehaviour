#pragma once
#ifndef MBK_UNIT_PAHUB_H
#define MBK_UNIT_PAHUB_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include <TCA9548.h>
#include <array>
#include <memory>
#include "sensors/units/Unit.h"
#include "sensors/units/i2c/UnitI2C.h"

class UnitPAHUB : public UnitI2C {
  static constexpr uint8_t k_address = 0x70;
  uint8_t _address = 0;
  PCA9548  _sensor{k_address, _wire};

  static constexpr uint8_t k_max_units = 8;
  std::array<std::unique_ptr<Unit>, k_max_units> _owned_units{};
  bool _active_units[k_max_units]{false};
  uint8_t _owned_unit_count{0};


  float _distance = 0.0f;
  uint8_t channels = 0;

public:

  UnitPAHUB()           : UnitI2C() {_address = k_address;}
  UnitPAHUB(TwoWire& w) : UnitI2C(w) {_address = k_address;}
  UnitPAHUB(TwoWire& w, uint8_t addr) : UnitI2C(w) {
    _sensor = PCA9548{addr, _wire};
    _address = addr;
  }

  bool begin_impl() override {
    _logger.setPrefix(prefix_print);

    if(!_sensor.begin()){
      _logger.errorln("Failed to initialize");
      return false;
    }
    channels = _sensor.channelCount();

    scan();
    _connected = true;
    return _connected;
  }
  Unit* create_unit(UnitModel model, int index) {

  std::unique_ptr<Unit> unit;
  TwoWire& bus = *_wire;
  switch (model) {
    case UnitModel::sensor_tof4m:
      unit = std::make_unique<UnitVL53L1X>(bus);
      break;
    case UnitModel::sensor_tvoc:
      unit = std::make_unique<UnitSGP30>(bus);
      break;
    case UnitModel::sensor_dlight:
      unit = std::make_unique<UnitDLight>(bus);
      break;
    case UnitModel::sensor_accel:
      unit = std::make_unique<MPU6886>(bus);
      break;
    case UnitModel::sensor_ultrasonic_i2c:
      unit = std::make_unique<UltrasonicI2C>(bus);
      break;
    case UnitModel::hmi_8angle:
      unit = std::make_unique<Unit8Angle>(bus);
      break;
    default:
      _logger.warningln("create_unit() unsupported model %d", static_cast<int>(model));
      return nullptr;
  }

  if (!unit) {
    return nullptr;
  }

  unit->set_model(model);
  _owned_units[index] = std::move(unit);
  _owned_unit_count++;
  return _owned_units[index].get();
}
void set_enabled(bool val) override {
  Unit::set_enabled(val);
  for (uint8_t i = 0; i < k_max_units; ++i) {
   if(_active_units[i]){
    if(_sensor.selectChannel(i)){
       _owned_units[i]->set_enabled(val);
    }else {
      _logger.errorln("Failed to select channel %i");
    }
   

   }

  } 
}  
void set_alpha(float a) override{
  Unit::set_alpha(a);
    for (uint8_t i = 0; i < k_max_units; ++i) {
      if(_active_units[i]){
        if( _sensor.selectChannel(i)){
          _owned_units[i]->set_alpha(a);
        }else{
          _logger.errorln("failed to select channel %d when setting alpha ", i);
        }

      }
        } 
}
void scan(){


    for(uint8_t i = 0 ; i < _sensor.channelCount(); i++){
      Serial.println(i);
      _sensor.selectChannel(i);
      bool b = false;
      
      for (uint8_t address = 0x01; address < 0x7f; address++) {
        if(address == _address)continue;
        b = _sensor.isConnected(address,i);
        if(b){
          UnitModel m = model_from_i2c_address(address);
          _logger.infoln("Found unit %s  at address %d on channel %d", model_to_str(m),address, i);
          Unit* new_u = create_unit(m, i);
          if(new_u){
            if(!new_u->begin()){
              _logger.error("Failed to begin unit on channel %d", i);
            }
            new_u->set_id(i);
            new_u->set_enabled(true);
            break;
          }else{
            _owned_units[i].reset();
            if (_owned_unit_count > 0) {
              --_owned_unit_count;
            }
          }
        }
      } // end address loop
      if(!b){
        _sensor.disableChannel(i);      
      }
       _active_units[i] = b;
    }
      
    
    if(_owned_unit_count == 0){
      _logger.warningln("No units found");
    }
  }

  bool sample_impl(uint32_t now_ms) override {

    if (!ensure_connected(now_ms)) {
      Serial.println("Hub not connected");
      return false;
    }

    if(_owned_unit_count == 0){
      //Serial.println("No units to tick");
      return false;
    }

    if(!_sensor.isConnected()){
      _connected = false;
      return false;
    }

    for(uint8_t i = 0 ; i < channels; i++){
      if(_active_units[i]){
        if(_owned_units[i]){
          _sensor.selectChannel(i);
          bool s = _owned_units[i]->tick(now_ms);
        }
      }
    }
    
    // _logger.traceln("distance : %F",_distance);
    return true;
  }



  void write_json(JsonObject& dst) const override
  {
    Unit::write_json(dst);   
    for(uint8_t i = 0 ; i < channels; i++){
      if(_active_units[i]){
        if(_owned_units[i]){
          JsonObject obj = dst["val"].add<JsonObject>();
          _owned_units[i]->write_json(obj);
        }
      }
    }

  }

  void write_config_to_json(JsonObject & obj) override{
    Unit::write_config_to_json(obj);
    JsonArray units= obj["units"].to<JsonArray>();
    for(uint8_t i = 0 ; i < channels; i++){
      if(_active_units[i]){
        if(_owned_units[i]){
          JsonObject u = units.add<JsonObject>();
          _owned_units[i]->write_config_to_json(u);
        }
      }
    }
  }

  static void prefix_print(Print* _logOutput, int logLevel){
      _logOutput->printf("[Unit - PAHUB] ");
    }
};

#endif  // MBK_UNIT_PAHUB_H