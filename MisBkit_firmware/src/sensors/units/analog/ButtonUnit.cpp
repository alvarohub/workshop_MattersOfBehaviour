/**
 * @file ButtonUnit.cpp
 * @author Etienne Montenegro
 * @brief Implementation of ButtonUnit.h
 */

#include "sensors/units/analog/ButtonUnit.h"
#include <ArduinoLog.h>

ButtonUnit::ButtonUnit(PortC& port, UnitModel _model) : _port(port), Unit(_model){

}


bool ButtonUnit::begin_impl() {
    _logger.setPrefix(prefix_print);
    pinMode(_port.pins[0],INPUT);
    return true;
}

bool ButtonUnit::sample_impl(uint32_t now_ms) {
    _filtered[0] = apply_lowpass(map_to_float(analogRead(_port.pins[0]),0,4095), _filtered[0]);
    return true;
}


void ButtonUnit::write_json(JsonObject& dst) const {
    Unit::write_json(dst);
    dst["val"].add(_filtered[0]);
}


