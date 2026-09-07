#include "RCManager.h"

bool RCManager::validate_index(uint8_t i) const{
  if( i >= 0 && i < k_max_channels){
    return true;
  }
  return false;
}

uint8_t RCManager::get_pin(uint8_t channel){
  uint8_t pin = -1;
  switch (channel)
  {
  case 0:
    pin = pins::pwm1 ;
  break;
  case 1:
    pin = pins::pwm2 ;
  break;
  case 2:
    pin = pins::pwm3;
  break;
  case 3:
  pin = pins::pwm4;
  break;
  default:
    break;
  }
  return pin;
}

void RCManager::initialize(const rcConfiguration& config) {
  _logger.setPrefix(prefix_print);
  _logger.begin(LOG_LEVEL_TRACE,&Serial);
  _logger.infoln("Initializing RC Servo manager");
  
  shutdown();
  
  for (uint8_t i = 0; i < k_max_channels; ++i) {
    const bool channel_enabled = config.channel_enabled[i];
    _channels[i].configure(i, get_pin(i), channel_enabled, config.channel_min_angle[i], config.channel_max_angle[i], config.channel_default_speed_dps[i], config.channel_easing_type[i]);
    if (channel_enabled) {
      _channels[i].initialize();
    }
  }
   _logger.infoln("Initialization Successfull");
}

void RCManager::shutdown() {
   _logger.infoln("Shutting down all rc servos");
  for (uint8_t i = 0; i < k_max_channels; ++i) {
    _channels[i].shutdown();
  }
}

RCMotor* RCManager::get(uint8_t channel) {
  if (!validate_index(channel)) {
    return nullptr;
  }
  
  RCMotor& controller = _channels[channel];
  if (!controller.isActive()) {
    _logger.warningln("Target RC servo at channel %d is not active.", channel);
    return nullptr;
  }

  return &controller;
}

const RCMotor* RCManager::get(uint8_t channel) const {
  if (!validate_index(channel)) {
      return nullptr;
    }
  
  const RCMotor& controller = _channels[channel];
  if (!controller.isActive()) {
    _logger.warningln("Target RC servo at channel %d is not active.", channel);
    return nullptr;
  }

  return &controller;
}




  
  bool RCManager::moveJoint(uint8_t channel, float target_angle, uint16_t speed_override_dps){
    RCMotor* m = get(channel);
    if(!m){
      _logger.errorln("Target RC Servo at %d is nullptr", channel);
      return false;}
    return m->move_to(target_angle, speed_override_dps);
  }
  bool RCManager::setJointSpeed(uint8_t channel, uint16_t speed_dps){
    RCMotor* m = get(channel);
    if(!m)return false;
    return m->set_speed(speed_dps);
  }
  bool RCManager::stop(uint8_t channel){
    RCMotor* m = get(channel);
    if(!m)return false;

    return m->stop();
  }
  void RCManager::stopAll(){
    _logger.infoln("Stopping all RC servo");

    for(RCMotor& m : _channels){
      m.stop();
    }
  }

bool RCManager::activate(uint8_t channel, bool onoff){

    bool s = false;
    if(onoff){
      s = _channels[channel].initialize();
    }else{
      s = _channels[channel].shutdown();
    }
   return s;

}
  
void RCManager::prefix_print(Print* _logOutput, int logLevel) {
  _logOutput->print("[RC Manager] ");
}

