/**
 * @file Unit.cpp
 * @author Etienne Montenegro
 * @brief Implementation of the Unit lifecycle base class.
 */

#include <Arduino.h>
#include <ArduinoLog.h>
#include "sensors/units/Unit.h"

Unit::Unit(UnitModel _model){
  _config.model=_model;
  set_channel_names();
}

void Unit::set_channel_names(){
  
    uint8_t ch_count = 0;
    const char* const* ch_names = get_channel_names(_config.model, _config.channel_count);
    if (ch_count > 0 && ch_names != nullptr) {
      _config.channel_count = ch_count;
      _config.channels = ch_names;
    }
}
bool Unit::begin() {
    _logger.begin(LOG_LEVEL_TRACE,&Serial);
    _initialized = begin_impl();

  if (!_initialized) {
    _logger.errorln("Failed to initialize Unit model=%s on port %d", model_to_str(_config.model), _config.id);
    return false;
  }
  _logger.infoln("Success connecting to device");
  return true;
}

void Unit::write_config_to_json(JsonObject& obj){
  _config.write_to_json(obj);
};

 void Unit::write_json(JsonObject& dst) const{
    dst["id"] = _config.id;
    dst["name"] = model_to_str(_config.model);
  };


bool Unit::tick(uint32_t now_ms) {
  
  if (!_config.enabled) {
      // _logger.traceln("id=%d model=%s Not enabled", _id, model_to_str(model));
    return false;
  }

  if ((now_ms - _last_sample_ms) < _sample_period_ms) {
    return false;
  }

  const bool ok = sample_impl(now_ms);
  _last_sample_ms = now_ms;

  if (ok) {
    return true;
  }

  return false;
}


void Unit::set_enabled(bool val) {
  if(! _initialized){
    _logger.warningln("Unit %d not initialized, cannot enable",_config.id);
    _config.enabled = false;
    return;
  }
  _logger.traceln("Unit::set_enabled() id=%d enabled=%d", _config.id, val);
  _config.enabled = val;

}

float Unit::map_to_float(float val, float min_range, float max_range, float new_min, float new_max){
  return (val - min_range) * ((new_max - new_min) / (max_range - min_range)) + new_min;
}

float Unit::apply_lowpass(float current, float previous){
  float a = get_alpha();
  return (a * current) + ((1.0f - a) * previous);
}

void Unit::to_json(JsonObject entry, float v, const char* name) {
  entry["name"] = name;
  entry["val"] = v;
}

void Unit::set_config(JsonObject obj){

  serializeJson(obj,Serial);
  Serial.println();
  _config.enabled = obj["io"].as<bool>();
  _config.alpha = obj["alpha"];
  // set_config_imp(obj);
}

