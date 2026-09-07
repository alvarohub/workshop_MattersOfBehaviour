/**
 * @file FaderUnit.cpp
 * @author Etienne Montenegro
 * @brief Implementation of FaderUnit.h
 */

#include "sensors/units/analog/FaderUnit.h"
#include <ArduinoLog.h>

FaderUnit::FaderUnit(PortC& port, UnitModel _model) : _port(port), Unit(_model){
}


bool FaderUnit::begin_impl() {
    _logger.setPrefix(prefix_print);
    pinMode(_port.pins[0],INPUT);
    return true;
}

bool FaderUnit::sample_impl(uint32_t now_ms) {
    _filtered[0] = apply_lowpass(map_to_float(analogRead(_port.pins[0]),0,4095), _filtered[0]);
    
    return true;
}


void FaderUnit::write_json(JsonObject& dst) const {
    Unit::write_json(dst);
    dst["val"].add(_filtered[0]);
}


