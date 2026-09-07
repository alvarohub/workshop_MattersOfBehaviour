/**
 * @file MotorClass.cpp
 * @author Etienne Montenegro
 * @brief implementation file of MotorClass.h
 *
 */

#include "MotorClass.h"

#include <ArduinoLog.h>

using namespace ControlTableItem;

#define DEFAULT_MAX_SPEED_IN_JOINT_MODE 256 // was 128, could be changed to 0: no limit up to 1023

void Motor::prefix_print(Print* _logOutput, int logLevel) {
  _logOutput->print("[Motor] ");
}

Motor::Motor(Dynamixel2Arduino& _dxl, int _id) : dxl(_dxl), id(_id) {
  _logger.begin(LOG_LEVEL_TRACE, &Serial);
  _logger.setPrefix(&Motor::prefix_print);
  _logger.traceln("motor_id=%d init", id);
  setMode(JOINT_MODE);
  setSpeed(DEFAULT_MAX_SPEED_IN_JOINT_MODE);
  setGoalPosition(512);
}


void Motor::readLimits() {
  limits.angleCW = dxl.readControlTableItem(CW_ANGLE_LIMIT, id);
  limits.angleCCW = dxl.readControlTableItem(CCW_ANGLE_LIMIT, id);
  limits.temperature = dxl.readControlTableItem(TEMPERATURE_LIMIT, id);
  limits.voltageMin = dxl.readControlTableItem(MIN_VOLTAGE_LIMIT, id);
  limits.voltageMax = dxl.readControlTableItem(MAX_VOLTAGE_LIMIT, id);
  limits.torqueMax = dxl.readControlTableItem(MAX_TORQUE, id);

  _logger.infoln("motor_id=%d limits ccw=%d cw=%d temp=%d v_min=%d v_max=%d torque_max=%d",
                 id,
                 limits.angleCCW,
                 limits.angleCW,
                 limits.temperature,
                 limits.voltageMin,
                 limits.voltageMax,
                 limits.torqueMax);
}

/**
 * @brief Translate a DXLLibErrorCode_t error code to a human readable string
 *
 * @param err the error code to translate
 * @return a human readable string corresponding to the error code
 */
const char* LibErrCodeToString(DXLLibErrorCode_t err) {
  switch (err) {
    case DXL_LIB_OK:
      return "OK";
    case DXL_LIB_PROCEEDING:
      return "PROCEEDING";
    case DXL_LIB_ERROR_NOT_SUPPORTED:
      return "NOT_SUPPORTED";
    case DXL_LIB_ERROR_TIMEOUT:
      return "TIMEOUT";
    case DXL_LIB_ERROR_INVAILD_ID:
      return "INVAILD_ID";
    case DXL_LIB_ERROR_NOT_SUPPORT_BROADCAST:
      return "NOT_SUPPORT_BROADCAST";
    case DXL_LIB_ERROR_NULLPTR:
      return "NULLPTR";
    case DXL_LIB_ERROR_LENGTH:
      return "LENGTH";
    case DXL_LIB_ERROR_INVAILD_ADDR:
      return "INVAILD_ADDR";
    case DXL_LIB_ERROR_ADDR_LENGTH:
      return "ADDR_LENGTH";
    case DXL_LIB_ERROR_BUFFER_OVERFLOW:
      return "BUFFER_OVERFLOW";
    case DXL_LIB_ERROR_PORT_NOT_OPEN:
      return "PORT_NOT_OPEN";
    case DXL_LIB_ERROR_WRONG_PACKET:
      return "WRONG_PACKET";
    case DXL_LIB_ERROR_CHECK_SUM:
      return "CHECK_SUM";
    case DXL_LIB_ERROR_CRC:
      return "CRC";
    case DXL_LIB_ERROR_INVAILD_DATA_LENGTH:
      return "INVAILD_DATA_LENGTH";
    case DXL_LIB_ERROR_MEMORY_ALLOCATION_FAIL:
      return "MEMORY_ALLOCATION_FAIL";
    case DXL_LIB_ERROR_INVAILD_PROTOCOL_VERSION:
      return "INVAILD_PROTOCOL_VERSION";
    case DXL_LIB_ERROR_NOT_INITIALIZED:
      return "NOT_INITIALIZED";
    case DXL_LIB_ERROR_NOT_ENOUGH_BUFFER_SIZE:
      return "NOT_ENOUGH_BUFFER_SIZE";
    case DXL_LIB_ERROR_PORT_WRITE:
      return "PORT_WRITE";

    default:
      return "UNKNOWN";
  }
}

void Motor::setID(uint8_t new_id) {
  _logger.infoln("motor_id=%d request_set_id=%d", id, new_id);
  if (!dxl.setID(id, new_id)) {
    _logger.errorln("motor_id=%d set_id_failed status_error=%d", id,
                    dxl.getLastStatusPacketError());
  }
  id = new_id;
}



// TODO: Finish implementing bulkread to get all values at once
void Motor::updateInfos() {  // WIP
  // float temp_reading;

  // // goal_pos = dxl.readControlTableItem(GOAL_POSITION,id);
  // // auto current_speed = dxl.readControlTableItem(MOVING_SPEED,id);
  // float temp_reading = dxl.getPresentVelocity(id);
  // if (temp_reading == 0) {
  //   auto error = dxl.getLastLibErrCode();
  //   if (error != DXL_LIB_OK) {
  //     Log.errorln("Failed to get present velocity. LibErrCode : %s", LibErrCodeToString(error));
  //   }
  // }else {
  // moving_speed = speed;
  // }


  // float temp_reading = dxl.getPresentPosition(id);
  // if (temp_reading == 0 && dxl.getLastLibErrCode() != DXL_LIB_OK) {
  //   Log.errorln("Failed to get present position. LibErrCode : %s", LibErrCodeToString(dxl.getLastLibErrCode()));
  // }else{
  //   pos = temp_reading;
  // }



  // load = dxl.readControlTableItem(PRESENT_LOAD,id);
  // voltage = dxl.readControlTableItem(PRESENT_VOLTAGE,id);
  // temperature = dxl.readControlTableItem(PRESENT_TEMPERATURE,id);
  // torqueLimit = dxl.readControlTableItem(TORQUE_LIMIT,id);

  // shutdownError = dxl.readControlTableItem(SHUTDOWN,id); // not sure if
  // needed.. value never change

  // moving = dxl.readControlTableItem(MOVING,id);
  // Serial.printf("GOAL: %.4d, MOVINGSPEED : %.4d, POS: %.4d, SPEED: %.4d,
  // LOAD: %d, VOLTAGE: %d, TEMP: %d, TORQUELIMIT: %d, MOVING: %d, ERROR:
  // %d\n",goal_pos, moving_speed, pos, speed,load, voltage, temperature,
  // torqueLimit, moving, shutdownError);
}

bool Motor::factory_reset(){
  if(!dxl.factoryReset(id, 0x02)){
    _logger.errorln("motor_id=%d factory_reset_failed", id);
    return false;
  }
  _logger.infoln("motor_id=%d factory_reset_ok", id);
  return true;
}

void Motor::setMode(modes mode) { 
  
  if(this->mode == mode) return;

  if (dxl.getTorqueEnableStat(id)) {
    if (!dxl.writeControlTableItem(TORQUE_ENABLE, id, false)) {
      _logger.errorln("motor_id=%d torque_off_failed", id);
      return;
    }
  }
   bool ret = false;
  switch (mode) {
    case WHEEL_MODE:{
      bool cw_ok  = dxl.writeControlTableItem(ControlTableItem::CW_ANGLE_LIMIT,  id,    0);
      bool ccw_ok = dxl.writeControlTableItem(ControlTableItem::CCW_ANGLE_LIMIT, id, 0);
      _logger.infoln("motor_id=%d set_cw=0 ok=%d set_ccw=1023 ok=%d lib_err=%s status_err=%d",
                     id, cw_ok, ccw_ok,
                     LibErrCodeToString(dxl.getLastLibErrCode()),
                     dxl.getLastStatusPacketError());
      if (!cw_ok || !ccw_ok) {
        _logger.errorln("motor_id=%d set_mode_failed mode=Wheel", id);
        break;
      }
      
      int32_t read_cw  = dxl.readControlTableItem(ControlTableItem::CW_ANGLE_LIMIT, id);
      int32_t read_ccw = dxl.readControlTableItem(ControlTableItem::CCW_ANGLE_LIMIT, id);
      _logger.infoln("motor_id=%d verify_limits cw=%d ccw=%d", id, read_cw, read_ccw);
      this->mode = WHEEL_MODE;
      _logger.infoln("motor_id=%d set_mode_ok mode=wheel", id);
      break;
      }
    case JOINT_MODE: {
      bool cw_ok  = dxl.writeControlTableItem(ControlTableItem::CW_ANGLE_LIMIT,  id,    0);
      bool ccw_ok = dxl.writeControlTableItem(ControlTableItem::CCW_ANGLE_LIMIT, id, 1023);
      _logger.infoln("motor_id=%d set_cw=0 ok=%d set_ccw=1023 ok=%d lib_err=%s status_err=%d",
                     id, cw_ok, ccw_ok,
                     LibErrCodeToString(dxl.getLastLibErrCode()),
                     dxl.getLastStatusPacketError());
      if (!cw_ok || !ccw_ok) {
        _logger.errorln("motor_id=%d set_mode_failed mode=joint", id);
        break;
      }
      int32_t read_cw  = dxl.readControlTableItem(ControlTableItem::CW_ANGLE_LIMIT, id);
      int32_t read_ccw = dxl.readControlTableItem(ControlTableItem::CCW_ANGLE_LIMIT, id);
      _logger.infoln("motor_id=%d verify_limits cw=%d ccw=%d", id, read_cw, read_ccw);
      this->mode = JOINT_MODE;
      _logger.infoln("motor_id=%d set_mode_ok mode=joint", id);
      setSpeed(DEFAULT_MAX_SPEED_IN_JOINT_MODE);
      break;
    }

    default:
      _logger.errorln("motor_id=%d set_mode_failed mode=unknown", id);
      break;
  }

  if (!dxl.writeControlTableItem(TORQUE_ENABLE, id, true)) {
    _logger.errorln("motor_id=%d torque_on_failed", id);
  }
}

void Motor::setGoalPosition(int32_t goal) {
  if (isWheel()) {
    setMode(JOINT_MODE);
  }

  if (!dxl.writeControlTableItem(GOAL_POSITION, id, goal, timeout_ms)) {
    _logger.errorln("motor_id=%d set_goal_failed goal=%d", id, goal);
    _logger.errorln("Last StatusPacketerr: %B",dxl.getLastStatusPacketError());
  }else{
    // _logger.infoln("t= %l lmotor_id=%d set_goal_ok goal=%d",millis(), id, goal);
    // Serial.println(LibErrCodeToString(dxl.getLastLibErrCode()));
    goal_pos = goal;
  }
}

void Motor::setGoalPositionNoRx( int32_t goal )
{
  if (isWheel()) {
    setMode(JOINT_MODE);
  }

  uint8_t param[2];

  #define ADDR_GOAL_POSITION_L 30
  param[0] = goal & 0xFF;
  param[1] = (goal >> 8) & 0xFF;
  dxl.write( id, ADDR_GOAL_POSITION_L, param, 2, timeout_ms ); // par defaut, attend 103ms, mais si on met 0 => n'attend pas. 
}


void Motor::setSpeed(int32_t speed){
      if (!dxl.writeControlTableItem(MOVING_SPEED, id, speed)) {
      _logger.errorln("motor_id=%d set_speed_failed speed=%d", id, speed);
      Serial.println(dxl.getLastStatusPacketError(),BIN);
      
    } else {
      moving_speed = speed;
      _logger.traceln("motor_id=%d set_speed_ok speed=%d", id, speed);
    }
}

void Motor::setJointSpeed(int32_t speed) {
  if (isWheel()) {
    setMode(JOINT_MODE);
  }
  setSpeed(speed);
}

void Motor::setWheelSpeed(int32_t speed) {
  if (!isWheel()) {
    setMode(WHEEL_MODE);
  }
  setSpeed(speed);

}

void Motor::stop() {
  if (isWheel()) {
    setSpeed(0);
  } else {
    setGoalPositionNoRx(dxl.readControlTableItem(PRESENT_POSITION,id));
  }
}

bool Motor::isWheel() {
  if(mode == WHEEL_MODE) return true;
  return false;
}

uint8_t Motor::getID() const { return id; }

int16_t Motor::getActualPosition() const
{
  // return dxl.getPresentPosition(id);

#define ADDR_PRESENT_POSITION 36 // 36 in protocol 1, 132 in protocol 2.
#define LEN_PRESENT_POSITION 2    // 2 in protocol 1, 4 in protocol 2.

  uint8_t buffer[2];
  bool success;

  // si on met 6U, et qu'on debranche un moteur, on n'a pas d'erreur, par contre on recoit 0 qui sera donc vu comme une valeur impossible.
  // mais au moins ca ne ralentit pas la lecture.
  success = dxl.read( id, ADDR_PRESENT_POSITION, LEN_PRESENT_POSITION, buffer, sizeof(buffer), timeout_ms);
  if (!success)
  {
    // Echec de communication
    Serial.print("ERR: dxl.read: err comm with: ");
    Serial.println( id );
    return -1;
  }

  // Reconstruction int32 (little endian)

  int32_t position = (int32_t)buffer[0] | (int32_t)buffer[1] << 8;

  /*
  Serial.print("DBG: dxl.read: ");
  Serial.print( id );
  Serial.print( ", buf0: " );
  Serial.print( buffer[0] );
  Serial.print( ", buf1: " );
  Serial.print( buffer[1] );
  Serial.print( ", pos: " );
  Serial.println( position );
  */
 
  return position;
}

void Motor::setTorqueOn()
{
  _logger.infoln("motor_id=%d torque_on_request", id);
  dxl.torqueOn(id);
}

void Motor::setTorqueOff()
{
  _logger.infoln("motor_id=%d torque_off_request", id);
  dxl.torqueOff(id);
}

