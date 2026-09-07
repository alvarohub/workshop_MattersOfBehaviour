#include "sensors/ports/Port.h"
#include "sensors/units/Unit.h"

Port::~Port() = default;



bool Port::set_config(JsonObject obj) {
  return false;
}

void Port::write_to_json(JsonObject& obj){
  _config.write_to_json(obj);
}

DetectedModels Port::autodetect_units() {
  // Default no-op scan for ports that do not implement scanning.
}
