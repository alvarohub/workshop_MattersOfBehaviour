/**
 * @file Port.h
 * @author Etienne Montenegro
 * @brief 
 */

#ifndef PORT_H
#define PORT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <memory>

#include "sensors/units/UnitModel.h"
#include "sensors/ports/PortType.h"
#include "Configuration.h"
class Unit;

class Port {
 public:
  Port() = default;
  Port(uint8_t _id, uint8_t pinA, uint8_t pinB) :pins{pinA, pinB} {
    _config.id = _id;
  }

  virtual ~Port();

  virtual bool begin() = 0;
  virtual Unit* create_unit(UnitModel model, int index = 0, uint8_t address = 0) = 0;
  virtual void remove_unit(const Unit* unit) = 0;
  virtual void clear_units() = 0;
  virtual uint8_t unit_count() const = 0;
  virtual Unit* unit_at(uint8_t index) const = 0;
  virtual void tick_units(uint32_t now) = 0;
  virtual void serialize_units(JsonArray& arr) = 0;
  virtual bool has_model(UnitModel model) const = 0;
  virtual PortType type() const = 0;
  virtual DetectedModels autodetect_units();
  PortConfig* config(){return &_config;}
  void set_enabled(bool val);
  virtual bool set_config(JsonObject obj);
  virtual void write_to_json(JsonObject& obj);
  const uint8_t pins[2];

 protected:
  PortConfig _config;
  Logging _logger;
};

#endif  // PORT_H
