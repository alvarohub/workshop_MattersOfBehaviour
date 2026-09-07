/**
 * @file motorManager.cpp
 * @author Etienne Montenegro
 * @brief implementation file of motorManager.h
 */
#include "MotorManager.h"

#include <ArduinoLog.h>
#include <Chrono.h>
#include <Dynamixel2Arduino.h>

#include "MisBKit.h"
#include "Led.h"


using namespace ControlTableItem;  // Required to use Control table item names
// using namespace MisBKit;           // Required to use Control table item names

void MotorManager::prefix_print(Print* _logOutput, int logLevel) {
  _logOutput->print("[MotorManager] ");
}




MotorManager::MotorManager() {
}


void MotorManager::initialize_sync_write_buffers(const uint16_t start_address, const uint16_t address_len){
  // Reset the packet
  sw_infos.packet.p_buf = nullptr;
  sw_infos.packet.is_completed = false;

  // Set the address and length of the data structure for the sync write operation
  // each action has a specific write address
  sw_infos.addr = start_address;
  sw_infos.addr_length = address_len;

  // Set the address of the elements of the data structure
  // info_xels_sw contains the information about the Dynamixels to be written to.
  sw_infos.p_xels = info_xels_sw;

  // Set the number of motors in the data structure to 0
  sw_infos.xel_count = 0;
}

void MotorManager::initialize() {
  _logger.begin(LOG_LEVEL_TRACE, &Serial);
  _logger.setPrefix(&MotorManager::prefix_print);
  _logger.warningln("initializing");
  led::setColor(128,0,128,true);
  pinMode(pins::motorsControl, OUTPUT);
  #if defined(ARDUINO_M5Stack_StampS3)
  DXL_SERIAL.begin(dxlBaud, SERIAL_8N1, RX1PIN,TX1PIN);
  #endif
  dxl.begin(dxlBaud);
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

  _logger.infoln("dxl_bus_ready baud=%d protocol=%d", dxlBaud, DXL_PROTOCOL_VERSION);

  if (!dxl.scan()) {
    //led::error(); TODO : Change for warning instead of error
    _logger.errorln("scan_failed no_motors_detected");
  }

}

void MotorManager::reconfigureBadBaud() {
  _logger.infoln("reconfigure_bad_baud_start target_baud=%d", dxlBaud);

  int rates[10]{9600, 19200, 57600, 115200, 200000, 250000, 400000, 500000, 1000000, 2000000};

  for (int i = 0; i < sizeof(rates)/sizeof(int); i++) {
    if(rates[i] == dxlBaud) continue;
    dxl.begin(rates[i]);
    _logger.traceln("scan_baud=%d", rates[i]);
    for (uint8_t id = 0; id < DXL_BROADCAST_ID; id++) {
      if (dxl.ping(id)) {
        _logger.infoln("motor_id=%d found_at_baud=%d reconfigure_to=%d", id, rates[i], dxlBaud);
        if (!dxl.setBaudrate(id, dxlBaud)) {
          _logger.errorln("motor_id=%d set_baud_failed target_baud=%d", id, dxlBaud);
        }
      }
    }
  }
  dxl.begin(dxlBaud);
  _logger.infoln("reconfigure_bad_baud_done target_baud=%d", dxlBaud);
}

void MotorManager::setControlBaud( uint32_t newBaudRate )
{
  _logger.infoln("set_control_baud new_baud=%d", newBaudRate);
  dxlBaud = newBaudRate;
  initialize();
}

uint8_t MotorManager::scanMotors() {
  // Clean up any existing motors
  for (uint8_t i = 0; i < nMotors; i++) {
    if (foundMotorIds[i] != nullptr) {
      delete foundMotorIds[i];
      foundMotorIds[i] = nullptr;
    }
  }
  nMotors = 0;

  led::setColor(147,112,219,true);
  //factoryreset_all_motors();
  if(scan_all_bauds) reconfigureBadBaud();

  uint8_t nMotorsFound = 0;

  _logger.infoln("scan_start protocol=%d baud=%d", DXL_PROTOCOL_VERSION, dxlBaud);

  for (uint8_t id = 0; id < DXL_BROADCAST_ID; id++) {
    if (dxl.ping(id)) {
      _logger.infoln("motor_found motor_id=%d", id);
      foundMotorIds[nMotorsFound] = new Motor(dxl, id);
      foundMotorIds[nMotorsFound]->readLimits();
      foundMotorIds[nMotorsFound]->updateInfos();
      nMotorsFound++;
    }
  }
  _logger.infoln("scan_done motors_found=%d", nMotorsFound);
  nMotors = nMotorsFound;
  // factory_reset();
  led::off();
  return nMotors;
}

bool MotorManager::isEmpty() { return nMotors == 0; }

void MotorManager::factory_reset(){
  for (uint8_t i = 0; i < nMotors; i++) {
      uint8_t id = foundMotorIds[i]->factory_reset();
}
}

int16_t MotorManager::getMotorIndex(uint8_t _id) {
  for (uint8_t idx = 0; idx < nMotors; idx++) {
    if (foundMotorIds[idx] != nullptr && foundMotorIds[idx]->getID() == _id) {
      // Log.traceln("Found motor in array. index is %d", idx);
      return idx;
    }
  }
  return -1;
}


Motor* MotorManager::get(uint8_t id) {
  int16_t idx = getMotorIndex(id);
  if (idx != -1 && foundMotorIds[idx] != nullptr) {
    return foundMotorIds[idx];
  }
  return nullptr;
}


void MotorManager::stopAll() {
  for (uint8_t i = 0; i < nMotors; i++) {
    foundMotorIds[i]->stop();
  }
}

void  MotorManager::update_sync_write_buffers(uint8_t* id_list, uint8_t len){
  //update the Syncwrite structure with the discovered motor informations
    for(uint8_t i = 0; i < len; i++){
      // Set the ID of the element
      info_xels_sw[i].id = id_list[i];
      // Set the address of the element's data
      info_xels_sw[i].p_data = (uint8_t*)&sw_data[i].data;
        //set the number of motorsto sync write to
      sw_infos.xel_count++;
    }
}


void MotorManager::send_sync_write(){
    if(!dxl.syncWrite(&sw_infos) == true){
        _logger.errorln("sync_write_failed lib_err=%d xel_count=%d addr=%d len=%d",
                        dxl.getLastLibErrCode(),
                        sw_infos.xel_count,
                        sw_infos.addr,
                        sw_infos.addr_length);
      }
}

  void MotorManager::set_wheel_speed_sync(uint8_t* id_list, uint8_t len, uint16_t speed){
        set_mode_sync(id_list, len, WHEEL_MODE);
        set_speed_sync(id_list,len,speed);
  }


  void MotorManager::set_speed_sync(uint8_t* id_list, uint8_t len,uint16_t speed) {
      _logger.traceln("set_speed_sync len=%d speed=%d", len, speed);
      initialize_sync_write_buffers(SPEED_WRITE_ADDRESS, SPEED_ADDRESS_LEN);
      update_sync_write_buffers(id_list, len);
      for(uint8_t i = 0; i < len; i++){
        sw_data[i].data = speed;   
      }
      sw_infos.is_info_changed = true;
      send_sync_write();
  }

  void MotorManager::set_joint_speed_sync(uint8_t* id_list, uint8_t len,uint16_t speed) {
    _logger.traceln("set_joint_speed_sync len=%d speed=%d", len, speed);
    set_mode_sync(id_list, len, JOINT_MODE);
    set_speed_sync(id_list,len,speed);
  }
  void MotorManager::set_goal_position_sync(uint8_t* id_list, uint8_t len,uint16_t goal){
    _logger.traceln("set_goal_position_sync len=%d goal=%d", len, goal);
    set_mode_sync(id_list, len, JOINT_MODE);
    initialize_sync_write_buffers(POS_WRITE_ADDRESS, POS_ADDRESS_LEN);
    update_sync_write_buffers(id_list, len);
    for(uint8_t i = 0; i < len; i++){
      sw_data[i].data = goal;   
    }
    sw_infos.is_info_changed = true;
    send_sync_write();
  }

  void MotorManager::stop_sync(uint8_t* id_list, uint8_t len){
    for(uint8_t i = 0 ; i <len; i++){
      foundMotorIds[getMotorIndex(id_list[i])]->stop();
    }
  }
  void MotorManager::set_mode_sync(uint8_t* id_list, uint8_t len, modes mode){
    for(uint8_t i = 0 ; i <len; i++){
      foundMotorIds[getMotorIndex(id_list[i])]->setMode(mode);
    }
  }
