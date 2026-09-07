/**
 * @file PortA.h
 * @author Etienne Montenegro
 * @brief Grove PortA hardware resource placeholder (I2C-oriented connector).
 *
 * PortA is a plain GPIO resource stub. It carries no lifecycle.
 * Populated when PortA runtime bring-up (P6.2) is implemented.
 */

#ifndef PORT_A_H
#define PORT_A_H

#include <stdint.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <array>
#include <memory>

#include "sensors/units/Unit.h"
#include "sensors/units/UnitModel.h"
#include "sensors/ports/Port.h"
#include "sensors/units/i2c/UnitI2C.h"

class Unit;

class PortA : public Port {
 public:
  PortA(uint8_t _id, uint8_t _sda, uint8_t _scl) : Port(_id, _sda, _scl) {
    _config.type = PortType::port_a;
  }
  ~PortA() override;

  bool begin() override;
  int find_next_index();
  Unit* create_unit(UnitModel model, int index, uint8_t address) override;
  void remove_unit(const Unit* unit) override;
  void clear_units() override;
  uint8_t unit_count() const override;
  Unit* unit_at(uint8_t index) const override;
  void tick_units(uint32_t now) override;
  void serialize_units(JsonArray& arr) override;
  bool has_model(UnitModel model) const override;
  PortType type() const override { return PortType::port_a; }
  DetectedModels autodetect_units() override;

  bool attach_unit(Unit* unit);
  void detach_unit(const Unit* unit);


  uint8_t connected_unit_count() const { return _attached_unit_count; }
  Unit* connected_unit(uint8_t index) const;

  bool begin_bus_if_needed();
  bool bus_initialized() const { return _bus_initialized; }

  TwoWire* get_interface() { return &Wire; }
  bool set_config(JsonObject obj)override;
 private:
  
  static constexpr uint8_t k_max_units = 1;
  std::array<std::unique_ptr<Unit>, k_max_units> _owned_units{};
  uint8_t _owned_unit_count{0};

  std::array<Unit*, k_max_units> _attached_units{nullptr};
  uint8_t _attached_unit_count{0};
  bool _bus_initialized{false};
 static void prefix_print(Print* _logOutput, int logLevel);
 void write_to_json(JsonObject& obj)override;
};

#endif  // PORT_A_H