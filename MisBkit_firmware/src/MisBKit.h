/**
 * @file MisBKit.h
 * @author Etienne Montenegro
 * @brief This file contains all the methods and variables related to a MisBKit
 * configuration and status
 */
#ifndef MISBKIT_H
#define MISBKIT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>



#include "Configuration.h"
#include "PersistantConfiguration.h"
#include "NetworkManager.h"
#include "motors/RCManager.h"
#include "motors/MotorManager.h"
#include "sensors/sensorManager.h"
#include "WebInterfaceClass.h"
#include "Websocket.h"


#define WS_NBR_MAX_MOTORS 6
#define WS_NBR_MAX_CURVE_POINT 500

#define WS_NBR_MAX_SENSORS 10



class MisBKit{
  kitConfiguration kit_config;
  networkConfiguration net_config;
  sensorConfiguration sensor_config;
  rcConfiguration rc_config;
  PersistantConfiguration config_manager;

  NetworkHandler network_manager{&net_config};
  AsyncWebServer server{80};
  WebInterface web_interface{&server};
  WebSocketProtocol ws{&server};

  MotorManager motor_manager;
  RCManager rc_manager;
  SensorManager sensor_manager{&sensor_config};



  int16_t aaCurvePos_[WS_NBR_MAX_MOTORS][WS_NBR_MAX_CURVE_POINT];
  int     anLenCurve_[WS_NBR_MAX_MOTORS];
  float   arPosCurve_[WS_NBR_MAX_MOTORS]; // -1 if not playing
  float   arIncCurve_[WS_NBR_MAX_MOTORS]; // positif: normal loop, negatif: pingpong
  int     anLoopMode_[WS_NBR_MAX_MOTORS];
  bool    abIsPlaying_[WS_NBR_MAX_MOTORS]; // pause or stop the playing



  

  bool connected{false};
  bool scan_flag = false;
  bool reboot_flag = false;

  void load_configurations();
  void save_configuration();
  void reboot();

  void pair(IPAddress ip);
  inline void disconnect(){connected = false;}


  //maybe have an abstract interface to buffer handle and parse so that ws can be replaced easily.
  // the Misbkit should have a function that receives a MisBKit::Command_t as an argument to execute an action
  void process_command(const Command& c);
  void reply_all(const char* cmd, JsonDocument& val);
  void execute_action(JsonObject action);

  // void readBattery(int interval); // make this run in a task
  void  sendMotorsPosition();
  void  sendSensorsValues();
  public:
  MisBKit(){

  }
  void initialize(bool clear);
  void update();


void resetCurve();
int idToIdx(int id_motor );
int16_t * getCurveBuffer( int id_motor );
void playCurve( int id_motor, int len_curve, int loop_mode = 0, float duration = 9.0f );
void stopCurve( int id_motor );
void update_curve_play();

};

#endif
