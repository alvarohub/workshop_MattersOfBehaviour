/**
 * @file PortB.cpp
 * @author Etienne Montenegro
 * @brief PortB GPIO resource stub — no implementation until P6.1.
 */

#include "PortB.h"
#include <ArduinoJson.h>
#include "sensors/units/Unit.h"

PortB::~PortB() = default;

bool PortB::begin() {
  return true;
}

Unit* PortB::create_unit(UnitModel model, int index, uint8_t address) {
  (void)model;
  return nullptr;
}



void PortB::remove_unit(const Unit* unit) {
  if (!unit || !_owned_unit) {
    return;
  }

  if (_owned_unit.get() == unit) {
    _owned_unit.reset();
  }
}

void PortB::clear_units() {
  _owned_unit.reset();
}

uint8_t PortB::unit_count() const {
  return _owned_unit ? 1 : 0;
}

Unit* PortB::unit_at(uint8_t index) const {
  if (index != 0 || !_owned_unit) {
    return nullptr;
  }
  return _owned_unit.get();
}

void PortB::tick_units(uint32_t now) {
  if (_owned_unit) {
    _owned_unit->tick(now);
  }
}

void PortB::serialize_units(JsonArray& arr) {
  if (_owned_unit && _owned_unit->is_enabled()) {
    JsonObject obj = arr.add<JsonObject>();
    _owned_unit->write_json(obj);
  }
}

bool PortB::has_model(UnitModel model) const {
  return _owned_unit && _owned_unit->get_model() == model;
}

bool PortB::set_config(JsonObject obj){
return false;
}