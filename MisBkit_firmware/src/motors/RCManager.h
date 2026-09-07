#ifndef MISBKIT_RCMANAGER_H
#define MISBKIT_RCMANAGER_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include "pins.h"
#include "Configuration.h"
#include "RCMotor.h"
struct RCTelemetrySnapshot {
  bool configured{false};
  bool active{false};
  bool attached{false};
  int16_t target_angle{-1};
  uint16_t speed_intent_dps{0U};
  uint8_t easing_type{0U};
  bool easing_active{false};
};

class RCManager {
 public:
  static constexpr uint8_t k_max_channels = rcConfiguration::k_max_channels;

 private:
  RCMotor _channels[k_max_channels];
  mutable Logging _logger;
  static void prefix_print(Print* _logOutput, int logLevel);
  uint8_t get_pin(uint8_t channel);
  bool validate_index(uint8_t i) const;
 public:
  RCManager() = default;

  void initialize(const rcConfiguration& config);
  void shutdown();

  RCMotor* get(uint8_t channel);
  const RCMotor* get(uint8_t channel) const;
  bool moveJoint(uint8_t channel, float target_angle, uint16_t speed_override_dps = 0U);
  bool setJointSpeed(uint8_t channel, uint16_t speed_dps);
  bool stop(uint8_t channel);
  void stopAll();
  bool activate(uint8_t channel, bool onoff);
};

#endif  // MISBKIT_RCMANAGER_H