#include "RCMotor.h"
#include "RCManager.h"


uint16_t RCMotor::clamp_angle(float angle) const {
  if (angle < static_cast<float>(min_angle_)) {
    return min_angle_;
  }
  if (angle > static_cast<float>(max_angle_)) {
    return max_angle_;
  }
  return static_cast<uint16_t>(angle);
}

void RCMotor::configure(uint8_t channel, uint8_t pin, bool enabled, uint16_t min_angle, uint16_t max_angle, uint16_t default_speed_dps, uint8_t easing_type) {
  _logger.setPrefix(prefix_print);
  _logger.begin(LOG_LEVEL_TRACE,&Serial);
  channel_ = channel;
  pin_ = pin;
  configured_ = true;
  active_ = enabled;
  attached_ = false;
 
  
  if (min_angle > max_angle) {
    min_angle_ = max_angle;
    max_angle_ = min_angle;
  } else {
    min_angle_ = min_angle;
    max_angle_ = max_angle;
  }
  
  default_speed_dps_ = default_speed_dps;
  easing_type_ = easing_type;
  last_commanded_angle_ = -1;
  last_speed_intent_dps_ = default_speed_dps_;
  _logger.infoln("Servo %d configuration: \n          Pin: %d, Enable: %d, min_angle: %d, max_angle: %d, speed: %d, Easing:%d", channel_, pin_, active_, min_angle_, max_angle_, default_speed_dps_, easing_type_);
 
}

bool RCMotor::initialize() {
    if (!configured_) {
    active_ = false;
    attached_ = false;
    _logger.warningln("Failed to initialize RC servo %d, Configured: %d pin: %d", channel_, configured_, pin_);
    return false;
  }

  //digitalWrite(pin_, LOW);
  pinMode(pin_, OUTPUT);

  const uint16_t initial_angle = static_cast<uint16_t>((min_angle_ + max_angle_) / 2U);
  servo_.setEasingType(easing_type_);
  servo_.setSpeed(default_speed_dps_);

  // attached_ = servo_.attach(pin_, initial_angle) != INVALID_SERVO;
  attached_ = servo_.attach(pin_, initial_angle, 500, 2500, min_angle_, max_angle_) != INVALID_SERVO;
  if(!attached_){
    _logger.warningln("Failed to attach RC Servo %d on pin %d", channel_, pin_);
    active_ = false;
    return false;
  }

  active_ = true;
  last_commanded_angle_ = static_cast<int16_t>(initial_angle);
  last_speed_intent_dps_ = default_speed_dps_;
  _logger.infoln("Successfully initialized RC servo %d", channel_);
  //servo_.print(&Serial);
  return true;

}

bool RCMotor::shutdown() {
  if (attached_) {
    servo_.detach();
    attached_ = false;
    _logger.infoln("Detached RC servo %d", channel_);
    digitalWrite(pin_, LOW);
    pinMode(pin_, INPUT);
    active_ = false;
    return true;
  }
  return false;
}

bool RCMotor::move_to(float target_angle, uint16_t speed_override_dps) {
  if (!active_) {
    _logger.warningln("Can't move. Servo %d is not active", channel_);
    return false;
  }

  if (!attached_) {
    _logger.warningln("Can't move. Servo %d is not attached. Trying to attach...", channel_);
      return false;
  }

  const uint16_t bounded_target = clamp_angle(target_angle);

  if(speed_override_dps != 0U) set_speed(speed_override_dps);

  const bool started = servo_.startEaseTo(static_cast<int>(bounded_target), default_speed_dps_);
  if(started){
      last_commanded_angle_ = static_cast<int16_t>(bounded_target);
      _logger.infoln("Moving servo %d to %d at speed %d",channel_, last_commanded_angle_,last_speed_intent_dps_);
  }else{
      _logger.errorln("Failed moving servo %d",channel_);

  }
  return started;
}

bool RCMotor::set_speed(uint16_t speed_dps) {
  if (!active_ || speed_dps == 0U) {
    _logger.warningln("Servo %d Not active. Can't set speed",channel_);
    return false;
  }

  default_speed_dps_ = speed_dps;
  servo_.setSpeed(default_speed_dps_);
  last_speed_intent_dps_ = default_speed_dps_;
  _logger.infoln("Servo %d speed set to %d", channel_, last_speed_intent_dps_);
  return true;
}

bool RCMotor::stop() {
  if (!active_) {
    _logger.warningln("Servo %d not active. Cannot stop", channel_);

    return false;
  }
  
  if (attached_) {
    servo_.stop();
    _logger.infoln("Servo %d stopped", channel_);
  }
  return true;
}

uint8_t RCMotor::getChannel() const {
  return channel_;
}

uint8_t RCMotor::getPin() const {
  return pin_;
}

bool RCMotor::isConfigured() const {
  return configured_;
}

bool RCMotor::isActive() const {
  return active_;
}

bool RCMotor::isAttached() const {
  return attached_;
}

uint8_t RCMotor::getEasingType() const {
  return easing_type_;
}

 bool RCMotor::isEasingActive() const{
    return ! easing_type_ == EASE_LINEAR;
 }

int16_t RCMotor::getLastCommandedAngle() const {
  return last_commanded_angle_;
}

uint16_t RCMotor::getLastSpeedIntentDps() const {
  return last_speed_intent_dps_;
}





void RCMotor::prefix_print(Print* _logOutput, int logLevel) {
  _logOutput->print("[RC Servo] ");
}

