/**
 * @file Ultrasonic.h
 * @author Etienne Montenegro
 * @brief Ultrasonic unit — lifecycle actor for a Grove PortC
 *
 
 */

#ifndef ULTRASONIC_UNIT_H
#define ULTRASONIC_UNIT_H


#include "sensors/units/Unit.h"
#include "sensors/ports/PortC.h"

class UltrasonicUnit : public Unit {
 public:

  UltrasonicUnit(PortC& port, UnitModel _model);


  /**
   * @brief Serialize Ultrasonic state into a JSON object.
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
 unsigned long nextTick;
 uint16_t phase;
};

#endif  // ULTRASONIC_UNIT_H
