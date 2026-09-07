/**
 * @file PortB.h
 * @author Etienne Montenegro
 * @brief Grove PortB hardware resource placeholder (UART-oriented connector).
 *
 * PortB is a plain GPIO resource stub. It carries no lifecycle.
 * Populated when PortB runtime bring-up (P6.1) is implemented.
 */

#ifndef PORT_B_H
#define PORT_B_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <memory>

#include "sensors/units/Unit.h"
#include "sensors/units/UnitModel.h"
#include "sensors/ports/Port.h"

class PortB : public Port {
 public:
  PortB(uint8_t _id, uint8_t _tx, uint8_t _rx) : Port(_id, _tx, _rx), tx{_tx}, rx{_rx} {}
  ~PortB() override;

  bool begin() override;
  Unit* create_unit(UnitModel model, int index, uint8_t address) override;
  void remove_unit(const Unit* unit) override;
  void clear_units() override;
  uint8_t unit_count() const override;
  Unit* unit_at(uint8_t index) const override;
  void tick_units(uint32_t now) override;
  void serialize_units(JsonArray& arr) override;
  bool has_model(UnitModel model) const override;
  PortType type() const override { return PortType::port_b; }
  bool set_config(JsonObject obj)override;
  const uint8_t tx;
  const uint8_t rx;

 private:
  std::unique_ptr<Unit> _owned_unit;
};

#endif  // PORT_B_H