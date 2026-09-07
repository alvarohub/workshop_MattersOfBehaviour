/**
 * @file Unit.h
 * @author Etienne Montenegro
 * @brief Base lifecycle class for all Grove port units.
 *
 * Unit owns the tick/begin lifecycle, enable flag, sample period, and fault
 * tracking. Concrete units inherit Unit and implement begin_impl(),
 * sample_impl(), and write_json() to provide hardware-specific behavior and
 * serialization.
 *
 * PortC/PortA/PortB are plain GPIO resource classes — they do not inherit
 * Unit. Units hold a reference to a port resource and own the sampling logic,
 * GPIO direction choices, and JSON payload shape.
 *
 * Adding a new unit type:
 *   1. Create a class that inherits Unit.
 *   2. Implement begin_impl() to configure the port resource GPIO.
 *   3. Implement sample_impl() to read/drive pins and update cached state.
 *   4. Implement write_json() to emit the unit's data payload.
 *   5. Add the unit model to UnitModel and dispatch in SensorManager.
 */

#ifndef GROVE_UNIT_H
#define GROVE_UNIT_H

#include <stdint.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include "sensors/units/UnitModel.h"
#include "Configuration.h"

#define UNIT_MAX_CHANNELS 16
struct unitConfigration{

};

class Unit {
 public:
  Unit() = default;
  Unit(UnitModel _model);
  virtual ~Unit() = default;

  bool begin();
  void teardown();

  /**
   * @brief Lifecycle scheduler entry point.
   *
   * Enforces the configured sample period, then calls sample_impl().
   * Derived classes perform non-blocking work in sample_impl().
   *
   * @param now_ms Current time from millis()
   * @return true  New sample acquired
   * @return false No sample or sampling failed
   */
  bool tick(uint32_t now_ms);

  bool is_enabled() const { return _config.enabled; }

  virtual void set_enabled(bool val);

  void set_id(uint8_t id){_config.id = id;}
  void set_model(UnitModel m){
    _config.model = m;
    set_channel_names();
  }
  UnitModel get_model(){return _config.model;}
  UnitConfig get_config(){return _config;}
  float map_to_float(float val, float min_range, float max_range, float new_min = -0, float new_max = 1.0);
  float get_alpha() const {
    float a = 1.0 - _config.alpha;
    if(a == 0){
      a = 0.001;
    }else if( a == 1 ){
      a = 0.999;
    }
    return a;
  }
  virtual void set_alpha(float a){_config.alpha = a;}

  virtual void  write_json(JsonObject& dst) const;
  
  static void to_json(JsonObject entry, float v, const char* name);
  void set_config_imp( JsonObject obj);
  void set_config(JsonObject obj);
  virtual void write_config_to_json(JsonObject& obj);
  float _mapped[2];
  float _filtered[2];
 protected:
  virtual bool begin_impl() = 0;
  virtual bool sample_impl(uint32_t now_ms) = 0;
  void set_channel_names();
  float apply_lowpass(float current, float previous);
  Logging _logger;
 private:
  UnitConfig _config;

  bool   _initialized{false};
  uint32_t _sample_period_ms{16};
  uint32_t _last_sample_ms{0};
  



};

#endif  // UNIT_H
