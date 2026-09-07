/**
 * @file motorManager.h
 * @author Etienne Montenegro
 * @brief The motor manager is used to manage all the motors connected to a
 * MisbKit.
 *
 */
#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Dynamixel2Arduino.h>

// #include "MisBKit.h"
#include "MotorClass.h"
#include "Pins.h"
#if defined(ARDUINO_FEATHER_ESP32)
#define DXL_SERIAL Serial1
#elif defined(ARDUINO_M5Stack_StampS3)

// todo : Test serial 0 on a breadboard setup. If not working, switch to Serial1 and select available pins.
#define DXL_SERIAL Serial0 // 0 must be specified because CDC serial emulation uses Serial
#define RX1PIN 44
#define TX1PIN 43
#endif

#define MAX_NUM_MOTOR 16
#define POS_WRITE_ADDRESS 30
#define POS_ADDRESS_LEN 2

#define SPEED_WRITE_ADDRESS 32
#define SPEED_ADDRESS_LEN 2

// sw_data type can be used to send synced instruction to motors 
typedef struct sw_data{
  int16_t data;
} __attribute__((packed)) sw_data_t;


class MotorManager {
  Dynamixel2Arduino dxl{DXL_SERIAL, pins::motorsControl};
  Logging _logger;
  
  uint8_t nMotors = 0;
  Motor* foundMotorIds[MAX_NUM_MOTOR] = {nullptr};
  bool scan_all_bauds = false;

  bool sync_mode = false;

  sw_data_t sw_data[MAX_NUM_MOTOR];
  DYNAMIXEL::InfoSyncWriteInst_t sw_infos;
  DYNAMIXEL::XELInfoSyncWrite_t info_xels_sw[MAX_NUM_MOTOR];

  uint32_t dxlBaud = 1000000;  // communication speed with motors
  const unsigned int DXL_PROTOCOL_VERSION = 1;

  struct MotorRange {
      Motor* const* b;
      uint8_t n;
      Motor* const* begin() const { return b; }
      Motor* const* end()   const { return b + n; }
  };
  


  void initialize_sync_write_buffers(const uint16_t start_address, const uint16_t address_len);
  void update_sync_write_buffers(uint8_t* id_list, uint8_t len);
  void send_sync_write();
  static void prefix_print(Print* _logOutput, int logLevel);
 public:
  MotorManager();
  void initialize();
  int16_t getMotorIndex(uint8_t _id);
  Motor* get(uint8_t id);
  uint8_t scanMotors();
  MotorRange motors() const { return {foundMotorIds, nMotors}; }
  
  
  bool isEmpty();
  void stopAll();
  const Motor * const * getFoundMotorIds() const { return foundMotorIds; }
  Motor ** getFoundMotorIds() { return foundMotorIds; }


  void reconfigureBadBaud();
  void factory_reset();
  void setControlBaud(uint32_t newBaudRate);

  void stop_sync(uint8_t* id_list, uint8_t len);
  void set_wheel_speed_sync(uint8_t* id_list, uint8_t len, uint16_t speed);
  void set_speed_sync(uint8_t* id_list, uint8_t len, uint16_t speed);
  void set_joint_speed_sync(uint8_t* id_list, uint8_t len, uint16_t speed);
  void set_goal_position_sync(uint8_t* id_list, uint8_t len, uint16_t goal);
  void set_mode_sync(uint8_t* id_list, uint8_t len, modes mode);


};

#endif