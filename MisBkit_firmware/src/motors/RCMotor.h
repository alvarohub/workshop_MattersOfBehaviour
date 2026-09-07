#ifndef MISBKIT_RCMOTOR_H
#define MISBKIT_RCMOTOR_H

#include <Arduino.h>
#include <ArduinoLog.h>
#define SUPPRESS_HPP_WARNING
#include <ServoEasing.h>

#include "Configuration.h"



class RCMotor {
 private:
  uint8_t channel_{0U};
  uint8_t pin_{255U};
  bool configured_{false};
  bool active_{false};
  bool attached_{false};

  uint8_t easing_type_{EASE_CUBIC_IN_OUT};
  uint16_t min_angle_{0U};
  uint16_t max_angle_{270U};
  uint16_t default_speed_dps_{90U};
  int16_t last_commanded_angle_{-1};
  uint16_t last_speed_intent_dps_{0U};
  
  ServoEasing servo_{};
  Logging _logger;

  uint16_t clamp_angle(float angle) const;
  static void prefix_print(Print* _logOutput, int logLevel);

 public:
  RCMotor() = default;

  void configure(uint8_t channel,
                 uint8_t pin,
                 bool enabled,
                 uint16_t min_angle,
                 uint16_t max_angle,
                 uint16_t default_speed_dps,
                 uint8_t easing_type);
  bool initialize();
  bool shutdown();
  bool move_to(float target_angle, uint16_t speed_override_dps = 0U);
  bool set_speed(uint16_t speed_dps);
  bool stop();

  uint8_t getChannel() const;
  uint8_t getPin() const;
  bool isConfigured() const;
  bool isActive() const;
  bool isAttached() const;
  int16_t getLastCommandedAngle() const;
  uint16_t getLastSpeedIntentDps() const;
  uint8_t getEasingType() const;
  bool isEasingActive() const;
};

#endif  // MISBKIT_RCMOTOR_H