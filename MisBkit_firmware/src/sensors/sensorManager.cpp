/**
 * @file sensorManager.cpp
 * @author Etienne Montenegro
 * @brief Implementation file of sensorManager.h
 *
 */

#include "sensors/sensorManager.h"
#include <Arduino.h>
#include "sensors/ports/PortA.h"
#include "sensors/ports/PortB.h"
#include "sensors/ports/PortC.h"

namespace {

// Board-specific helper that maps logical port indices to the correct PortC pins.
// PortC is not a generic bus type, so each board defines its own analog/digital mapping.
bool resolve_port_pins(const uint8_t port_index, uint8_t& analog_pin, uint8_t& digital_pin) {
#if defined(ARDUINO_M5Stack_StampS3)
	switch (port_index) {
		case 0:
			analog_pin = pins::analogSensor1;
			digital_pin = pins::digitalSensor1;
			return true;
		case 1:
			analog_pin = pins::analogSensor2;
			digital_pin = pins::digitalSensor2;
			return true;
		case 2:
			analog_pin = pins::analogSensor3;
			digital_pin = pins::digitalSensor3;
			return true;
		case 3:
			analog_pin = pins::analogSensor4;
			digital_pin = pins::digitalSensor4;
			return true;
		case 4:
			analog_pin = pins::sda;
			digital_pin = pins::scl;
			return true;
		default:
			return false;
	}
#elif defined(ARDUINO_FEATHER_ESP32)
	switch (port_index) {
		case 0:
			analog_pin = pins::analogSensor1;
			digital_pin = pins::digitalSensor1;
			return true;
		case 1:
			analog_pin = pins::analogSensor2;
			digital_pin = pins::digitalSensor2;
			return true;
		case 2:
			analog_pin = pins::analogSensor3;
			digital_pin = pins::digitalSensor3;
			return true;
		case 3:
			analog_pin = pins::sda;
			digital_pin = pins::scl;
			return true;
		default:
			return false;
	}
#else
	(void)port_index;
	(void)analog_pin;
	(void)digital_pin;
	return false;
#endif
}

}  // namespace


SensorManager::SensorManager(sensorConfiguration* config) : sensor_config(config) {
}

SensorManager::~SensorManager() {
}

void printPrefix(Print* _logOutput, int logLevel) {
	_logOutput->print("[SensorManager] ");
}

bool SensorManager::set_config(uint8_t port_id, JsonObject doc){
	Port* port = port_at(port_id-1);
	if (!port) {
		_logger.warningln("set_config() port %d not found - skipping", port_id);
		return false;
	}
	return port->set_config(doc);
}


bool SensorManager::set_config(JsonDocument doc){
	bool send{false};
	JsonArray ports = doc["ports"].as<JsonArray>();
	for(JsonObject p : ports ){
		send = set_config(p["id"].as<uint8_t>(),p);
	}
	return send;
}

bool SensorManager::has_config() {
	if (!sensor_config) {
		_logger.warningln("No sensor_config pointer");
		return false;
	}
	return true;
}

bool SensorManager::is_index_valid(uint8_t i){
	if (i >= MBK_PORT_COUNT || i < 0) {
		_logger.errorln("Invalid port %d index", i);
		return false;
	}
	return true;
}

bool SensorManager::is_type_a(Port* p){
	if(p->type() != PortType::port_a){
		return false;
	}
	return true;
}

Port* SensorManager::port_at(uint8_t port_index) {
	if (!is_index_valid(port_index)) {
		return nullptr;
	}

	return port_bindings[port_index].get();
}

bool SensorManager::sync_unit_from_spec(uint8_t port_index, uint8_t unit_index, const UnitConfig& spec, bool log_replacement) {
	if (!has_config()) {
		return false;
	}

	Port* port = port_at(port_index);
	if (!port || spec.model == UnitModel::none) {
		return false;
	}

	Unit* unit = port->unit_at(unit_index);
	if (!unit || unit->get_model() != spec.model) {
		if (log_replacement) {
			_logger.infoln("Unit %d on port %d changed. Creating new object", unit_index, port_index);
		}


		if (unit && unit->get_model() != spec.model) {
			port->remove_unit(unit);
			unit = nullptr;
		}

		unit = port->create_unit(spec.model, unit_index);
		if (!unit) {
			_logger.errorln("Failed to create unit %d on port %d", unit_index, port_index);
			return false;
		}

		if (!unit->begin()) {
			_logger.errorln("Failed to begin unit %d on port %d", unit_index, port_index);
			port->remove_unit(unit);
			return false;
		}
	}

	if (unit->is_enabled() != spec.enabled) {
		unit->set_enabled(spec.enabled);
	}

	if (unit->get_alpha() != spec.alpha) {
		unit->set_alpha(spec.alpha);
	}

	return true;
}
void SensorManager::write_to_json(JsonDocument& doc){
	JsonArray ports = doc["ports"].to<JsonArray>();;
	for_each_port([&](uint8_t p,Port* port_ptr) {
		JsonObject port = ports.add<JsonObject>();
		port_ptr->write_to_json(port);
	});
}

void SensorManager::initialize() {
	_logger.begin(LOG_LEVEL_TRACE,&Serial);
	_logger.setPrefix(printPrefix);
	Serial.println();
	_logger.warningln("Initialization...");

	// Validate that the sensor configuration is available before any port setup.
	if (!has_config()) {
		return;
	}

	_logger.traceln("Configuring ports based on config.json");
	for (size_t i = 0; i < MBK_PORT_COUNT; i++) {
		_logger.traceln("Port=%d configured_units=%d type=%s", i, (int)sensor_config->ports[i].unit_count, port_type_to_string(sensor_config->ports[i].type));

		// Configure the physical port binding first. This may create a new PortA/B/C instance or leave the port empty if the configuration requested no binding.
		PortType requested = sensor_config->ports[i].type;
		if (!setup_port_binding(i, requested)) {
			_logger.errorln("Failed to setup port=%d. Type requested=%s)", i, port_type_to_string(requested));
		}
		
		// Initialize the bound port hardware before creating any units.
		Port* port = port_bindings[i].get();
		if (port) {
			if (!port->begin()) {
				_logger.warningln("Failed to begin port=%d", i);
				continue;
			}

			if (is_type_a(port)) {
				autodetect_port(i);
			}
		}
	}

	// Go throuhg every port and create unit objects 
	Serial.println();
	_logger.traceln("Creating unit objects");
	for (size_t i = 0; i < MBK_PORT_COUNT; i++) {
		Port* port = port_bindings[i].get();
		if (!port) {
			_logger.warningln("Port=%d has no valid binding", i);
        	continue;
    	}

		// Create units for this port only when the config specifies one or more units.
		if (sensor_config->ports[i].unit_count > 0) {
			create_units_for_port(i);
		} else {
			_logger.warningln("Port=%d has no configured units", i);
		}
	}
}

void SensorManager::autodetect_port(uint8_t port_index) {
	Port* port = port_at(port_index);
	if (!port || !is_type_a(port)) {
		return;
	}

	DetectedModels found = port->autodetect_units();
	if (found.count == 0) {
		return;
	}

	sensor_config->ports[port_index].clear_units();
	for (uint8_t j = 0; j < found.count; j++) {
		UnitConfig spec;
		spec.model = found.models[j];
		sensor_config->ports[port_index].add_unit(spec);
	}
}

bool SensorManager::setup_port_binding(const uint8_t port_index, PortType requested_type) {
	if(!is_index_valid(port_index)){
		return false;
	}

	port_bindings[port_index].reset();
	uint8_t pinA, pinB = 0;
	if(!resolve_port_pins(port_index,pinA,pinB)){
		_logger.errorln("Error while getting port pins for ports %d", port_index);
		return false;
	}
	switch(requested_type){
		case PortType::none:
			// Unbind the port immediately for a 'none' request and skip any hardware setup.
			return false;
		break;

		case PortType::port_a:
			// PortA uses I2C pins and may host I2C-based unit modules.
			// This branch binds the logical port index to a hardware-specific PortA class.
			port_bindings[port_index].reset(new PortA(port_index, pinA, pinB));
			_logger.traceln("Created PortA on port=%d sda=%d scl=%d", port_index, pinA, pinB);
			return true;
		break;
		
		case PortType::port_b:
			// PortB uses serial pins and is for UART-based units.
			port_bindings[port_index].reset(new PortB(port_index, pinA, pinB));
			_logger.traceln("Created PortB on port=%d tx=%d rx=%d", port_index, pinA, pinB);
			return true;
		break;
		
		case PortType::port_c:
			port_bindings[port_index].reset(new PortC(port_index, pinA, pinB));
			_logger.traceln("Created PortC on port=%d analog=%d digital=%d", port_index, pinA, pinB);
			return true;
		break;
	}

	return false;
}

void SensorManager::create_units_for_port(uint8_t port_index) {
	if (!has_config() || !is_index_valid(port_index)) {
		return;
	}

	// Create each unit described by the configuration for the selected port.
	// The configuration may describe multiple units; each one is created in order.
	uint8_t expected = sensor_config->ports[port_index].unit_count;

	for (uint8_t u = 0; u < expected; u++) {
		const auto &spec = sensor_config->ports[port_index].units[u];
		UnitModel m = spec.model;

		if (m == UnitModel::none) {
			continue; // skip empty specs
		}
		// _logger.traceln("Port=%d unit[%d] Specs: model=%s enabled=%d alpha=%F addr=%d", port_index, u, model_to_str(m), spec.enabled ? 1 : 0, spec.alpha, spec.addr);
		sync_unit_from_spec(port_index, u, spec, false);
	}
}

Unit* SensorManager::create_unit_routine(uint8_t port_index, UnitModel model){
	Port* port = port_bindings[port_index].get();

	// Ensure a valid port instance exists before attempting unit creation.
	if (!port) {
		_logger.warningln("Port %d model %s has no valid port binding", port_index, model_to_str(model));
		return nullptr;
	}

	// Attempt to create the requested unit model on the bound port.
	Unit* unit = port->create_unit(model);
	if (!unit) {
		_logger.warningln("Port %d model %s is not implemented or no capacity", port_index, model_to_str(model));
		return nullptr;
	}

	// Initialize the unit and remove it if initialization fails.
	if (!unit->begin()) {
		port->remove_unit(unit);
		return nullptr;
	}

	return unit;
}

bool SensorManager::scan(uint8_t idx) {
	bool changed = false;

	Port* base_port = port_at(idx);
	if (!base_port || !is_type_a(base_port)) {
		return false;
	}

	PortA* port = static_cast<PortA*>(base_port);
	if (!port->begin_bus_if_needed()) {
		_logger.warningln("Port %d rescan() cannot initialize bus", idx);
		return false;
	}

	DetectedModels found = port->autodetect_units();
	if (found.count == 0) {
		_logger.infoln("Port %d scan() found no units", idx);
		return false;
	}

	for (uint8_t i = 0; i < found.count; ++i) {
		UnitModel model = found.models[i];
		if (port->has_model(model)) {
			_logger.traceln("Port %d already has model %s", idx, model_to_str(model));
			continue;
		}

		UnitConfig spec;
		spec.model = model;
		spec.enabled = false;
		spec.alpha = 0.5f;
		spec.addr = -1;

		// PortA currently owns a single unit. Replace slot 0 when a new model is detected.
		if (sync_unit_from_spec(idx, 0, spec, false)) {
			// sensor_config->ports[idx].clear_units();
			// sensor_config->ports[idx].add_unit(spec);
			port_bindings[idx]->config()->clear_units();
			port_bindings[idx]->config()->add_unit(spec);
			changed = true;
			_logger.infoln("Port %d rescan() set model %s", idx, model_to_str(model));
			break;
		}
	}

	if (changed && !sensor_config->updated) {
		sensor_config->updated = true;
	}

	return changed;
}

bool SensorManager::no_sensor_enabled() {
	// Scan all ports and units to determine if any sensor is currently enabled.
	// This is used to decide whether the system should continue sampling sensor data.
	bool any_enabled = false;
	for_each_port([&](uint8_t, Port* port) {
		for (uint8_t u = 0; u < port->unit_count(); ++u) {
			Unit* unit = port->unit_at(u);
			if (unit && unit->is_enabled()) {
				any_enabled = true;
				return;
			}
		}
	});
	return !any_enabled;
}

void SensorManager::add_data_to_json(JsonDocument &doc) {

	JsonArray ports = doc["ports"].to<JsonArray>();

	for_each_port([&](uint8_t p, Port* port_ptr) {
		JsonObject port = ports.add<JsonObject>();
		port["id"] = p;
		JsonArray units = port["units"].to<JsonArray>();
		
		// Ensure there is a JSON array available for port data serialization.
		// The same array is used across all ports, so each unit appends its object to it.
		for (uint8_t u = 0; u < port_ptr->unit_count(); ++u) {
			Unit* unit_ptr = port_ptr->unit_at(u);
			if (unit_ptr && unit_ptr->is_enabled()) {
				JsonObject unit_json = units.add<JsonObject>();
				unit_ptr->write_json(unit_json);
			}
		}
	});
}

void SensorManager::update() {
	long n = millis();

	if (sensor_config->updated) {
		_logger.infoln("Configuration changed. Applying to ports and units");
		sensor_config->updated = false;

		for_each_port([&](uint8_t i, Port* port) {
			PortType requested = sensor_config->ports[i].type;
			if (requested == PortType::none) {
				_logger.warningln("Requested port is of type None. Can't apply config");
				return;
			}

			const uint8_t expected = sensor_config->ports[i].unit_count;
			if (expected == 0) {
				_logger.warningln("Port=%d has no configured units", i);
				return;
			}

			for (uint8_t u = 0; u < expected; u++) {
				const auto& spec = sensor_config->ports[i].units[u];
				sync_unit_from_spec(i, u, spec, true);
			}
		});
	}

	for_each_port([&](uint8_t, Port* port) {
		if (port->unit_count() == 0) {
			return;
		}
		port->tick_units(n);
	});
}




