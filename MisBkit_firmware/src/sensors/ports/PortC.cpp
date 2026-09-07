#include "sensors/ports/PortC.h"

#include "sensors/units/analog/AnalogUnits.h"


PortC::~PortC() = default;

bool PortC::begin() {
  return true;
}

Unit* PortC::create_unit(UnitModel model, int index, uint8_t address) {
  if (index < 0 || index >= k_max_units) {
    _logger.warningln("Port %d create_unit() invalid index=%d (max=%d)", _config.id, index, k_max_units);
    return nullptr;
  }
  std::unique_ptr<Unit> unit;
  switch (model) {
    case UnitModel::sensor_generic:
    case UnitModel::sensor_light:
    case UnitModel::sensor_mic:
      unit.reset(new GenericUnit(*this, model));
      break;
    case UnitModel::hmi_fader:
      unit.reset(new FaderUnit(*this, model));
      break;
    case UnitModel::hmi_button:
      unit.reset(new ButtonUnit(*this, model));
      break;
    default:
      _logger.warningln("Port %d create_unit() unsupported model %d", _config.id, static_cast<int>(model));
      return nullptr;
  }

  if (!unit) {
    return nullptr;
  }

  unit->set_model(model);
  unit->set_id(_config.unit_count + 1);
  UnitConfig cfg = unit->get_config();
  _owned_unit = std::move(unit);
  _config.add_unit(cfg);
  return _owned_unit.get();
}



void PortC::remove_unit(const Unit* unit) {
  if (!unit || !_owned_unit) {
    return;
  }

  if (_owned_unit.get() == unit) {
    _owned_unit.reset();
  }
}

void PortC::clear_units() {
  _owned_unit.reset();
}

uint8_t PortC::unit_count() const {
  return _owned_unit ? 1 : 0;
}

Unit* PortC::unit_at(uint8_t index) const {
  if (index != 0 || !_owned_unit) {
    return nullptr;
  }
  return _owned_unit.get();
}

void PortC::tick_units(uint32_t now) {
  if (_owned_unit) {
    _owned_unit->tick(now);
  }
}

void PortC::serialize_units(JsonArray& arr) {
  if (_owned_unit && _owned_unit->is_enabled()) {
    JsonObject obj = arr.add<JsonObject>();
    _owned_unit->write_json(obj);
  }
}

bool PortC::has_model(UnitModel model) const {
  return _owned_unit && _owned_unit->get_model() == model;
}
bool PortC::set_config(JsonObject obj) {
  bool send{false};
  JsonArray units = obj["units"].as<JsonArray>();
  if (units.size() == 0) return send;

  JsonObject unit_obj = units[0];

  UnitModel requested_model = model_str_to_enum(unit_obj["model"] | "none");
  if (requested_model == UnitModel::none) {
    _logger.warningln("Port %d set_config() unknown model", _config.id);
    return send;
  }

  if (_owned_unit->get_model() != requested_model) {
    _owned_unit.reset();
    _config.clear_units();
    Unit* u = create_unit(requested_model, 0, 0);
    if (!u) {
      _logger.warningln("Port %d set_config() failed to create model=%s", _config.id, unit_obj["model"] | "?");
      return send;
    }
    u->begin();
    send = true;
  }

  _owned_unit->set_config(unit_obj);
  return send;
}

void PortC::write_to_json(JsonObject& obj) {
   Port::write_to_json(obj);
  JsonArray units= obj["units"].to<JsonArray>();
  
    JsonObject u = units.add<JsonObject>();
    _owned_unit->write_config_to_json(u);
  
}