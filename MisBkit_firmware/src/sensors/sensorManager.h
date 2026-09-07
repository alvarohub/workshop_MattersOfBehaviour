/**
 * @file sensorManager.h
 * @author Etienne Montenegro
 * @brief Sensor manager holds all the sensor objects and is responsible of
 * handling updates and sending data
 *
 */
#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>

#include <Chrono.h>
#include <memory>
#include <array>

#include "Configuration.h"
#include "sensors/units/Unit.h"
#include "sensors/ports/Port.h"
#include "sensors/ports/PortType.h"
#include "Pins.h"

class SensorManager {
 private:
  std::unique_ptr<Port> port_bindings[MBK_PORT_COUNT];
  sensorConfiguration* sensor_config;
  Logging _logger;
  Port* port_at(uint8_t port_index);
  bool setup_port_binding(const uint8_t port_index, PortType requested_type);
  void detach_port_a_unit(uint8_t port_index);
  void create_units_for_port(uint8_t port_index);
  bool sync_unit_from_spec(uint8_t port_index, uint8_t unit_index, const UnitConfig& spec, bool log_replacement);
  bool has_config();
  bool is_index_valid(uint8_t i);
  bool is_type_a(Port* p);
  bool set_config(uint8_t port_id,JsonObject doc);

  template<typename Fn>
  void for_each_port(Fn&& fn) {
    for (uint8_t i = 0; i < MBK_PORT_COUNT; ++i) {
      Port* port = port_bindings[i].get();
      if (!port) {
        continue;
      }
      fn(i, port);
    }
  }
  
 public:
  SensorManager(sensorConfiguration* config);
  ~SensorManager();
  
  Unit* create_unit_routine(uint8_t port_index, UnitModel model);
  void autodetect_port(uint8_t port_index);
  void initialize();
  void add_data_to_json(JsonDocument& doc);
  void update();
  bool no_sensor_enabled();
  bool scan(uint8_t idx);
  bool set_config(JsonDocument doc);
  void write_to_json(JsonDocument& doc);

};

#endif