/**
 * @file PortC.h
 * @author Etienne Montenegro
 * @brief Grove PortC hardware resource.
 *
 * PortC is a plain GPIO resource for one Grove connector.
 * It carries no lifecycle, no enable flag, and no JSON serialization.
 *
 * Units that use a PortC connector hold a reference to a PortC and call
 * pin helper methods in begin_impl() / sample_impl().
 */

#ifndef PORT_C_H
#define PORT_C_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <memory>

#include "sensors/units/Unit.h"
#include "sensors/ports/Port.h"
#include "Configuration.h"

class PortC : public Port {
 public:
  /**
   * @brief Construct a PortC resource.
   *
   * @param pinA   GPIO number for the analog Grove signal line
   * @param pinB  GPIO number for the digital Grove signal line
   */
  PortC(uint8_t _id, uint8_t pinA, uint8_t pinB) : Port(_id, pinA, pinB) {
    _config.type = PortType::port_c;
  }
  ~PortC() override;

  bool begin() override;
  Unit* create_unit(UnitModel model, int index, uint8_t address) override;
  void remove_unit(const Unit* unit) override;
  void clear_units() override;
  uint8_t unit_count() const override;
  Unit* unit_at(uint8_t index) const override;
  void tick_units(uint32_t now) override;
  void serialize_units(JsonArray& arr) override;
  bool has_model(UnitModel model) const override;
  PortType type() const override { return PortType::port_c; }
  bool set_config(JsonObject obj) override;
  void write_to_json(JsonObject& obj) override;
  PortConfig _config;
  uint16_t raw[2];
  float _mapped[2];
  float _filtered[2];

 private:
  static constexpr uint8_t k_max_units = 1;
  std::unique_ptr<Unit> _owned_unit;
};

#endif  // PORT_C_H
