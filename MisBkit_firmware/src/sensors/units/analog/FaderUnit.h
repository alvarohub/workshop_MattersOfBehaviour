/**
 * @file FaderUnit.h
 * @author Etienne Montenegro
 */

#ifndef FADER_UNIT_H
#define FADER_UNIT_H


#include "sensors/units/Unit.h"
#include "sensors/ports/PortC.h"

class FaderUnit : public Unit {
 public:
  /**
   * @brief Construct a FaderUnit bound to a PortC GPIO resource.
   *
    * @param port  PortC that provides digital pin configuration/read helpers.
   *              The referenced PortC must outlive this object.
   * @param id    Logical unit id (matches physical port label on the PCB).
   */
  FaderUnit(PortC& port, UnitModel _model);


  /**
   * @brief Serialize button state into a JSON object.
   *
   * Calls Unit::write_json first (id, state), then appends:
   *   "pressed" : bool   — press event flag (does not consume it)
   *   "held"    : bool   — current held state
   *   "clicks"  : uint8  — click count since last reset
   */
  void write_json(JsonObject& dst) const override;

 protected:
  /**
   * @brief Configure PortC digital pin and initialize debounce state.
   *        Called by Unit::begin() when the unit is enabled.
   */
  bool begin_impl() override;

  /**
   * @brief Read digital state from PortC and detect edges.
   *        Called by Unit::tick() at the configured sample period.
   *        Not safe to call from an ISR.
   */
  bool sample_impl(uint32_t now_ms) override;

  

 private:
 PortC&   _port;
 static void prefix_print(Print* _logOutput, int logLevel){
      _logOutput->printf("[Unit - Fader] ");
    }
};

#endif  // FADER_UNIT_H
