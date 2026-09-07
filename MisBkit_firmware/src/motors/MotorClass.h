/**
 * @file MotorClass.h
 * @author Etienne Montenegro
 * @brief Motor class. A motor object is used to communicate with a
 *        AX-12A motor from dynamixel. For now the basic motor controls
 *        are implemented:
 *          - Wheel
 *          - Joint
 *          - Speed
 *          - Stop
 *         It is possible to ask informations to the motor. For now only
 *         the temperature is implemented.
 *
 */

#ifndef MOTORCLASS_H
#define MOTORCLASS_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Dynamixel2Arduino.h>
const char* LibErrCodeToString(DXLLibErrorCode_t err);
enum modes
{
  WHEEL_MODE = OP_VELOCITY,
  JOINT_MODE = OP_POSITION,
  ERROR_MODE = -1,
  UNKNOWN_MODE = -2
};
struct MotorLimits {
  int angleCCW;
  int angleCW;
  int temperature;
  int voltageMin;
  int voltageMax;
  int torqueMax;
};

class Motor {
 private:
  Dynamixel2Arduino& dxl;  // reference to dxl object
  int id{};                // id of the motor
  modes mode = UNKNOWN_MODE;  // movement mode // changed, by default you can't know what mode is the motor (putting joint_mode will prevent it to really send joint mode settings during init phasis)
  Logging _logger;


  MotorLimits limits;
  uint16_t goal_pos = 0;
  uint16_t moving_speed = 0;

  uint16_t pos = 0;
  uint16_t speed = 0;
  uint16_t torqueLimit = 0;
  uint16_t load = 0;
  uint8_t voltage = 0;
  uint8_t temperature = 0;
  bool moving = false;
  uint32_t timeout_ms = 6U;
  uint8_t shutdownError;

  void setSpeed(int32_t speed);
  static void prefix_print(Print* _logOutput, int logLevel);

 public:
  Motor() = delete;  // Remove default constructor since we need dxl reference
  Motor(Dynamixel2Arduino& _dxl, int _id);

  void updateInfos();

  bool factory_reset();
  
  void setID(uint8_t new_id);

  /**
   * @brief Read the motor limits from the control table EEPROM area
   */
  void readLimits();

  /**
   * @brief Stop the motor in place
   */
  void stop();

  /**
   * @brief Verify if the motor is currently in Wheel mode
   *
   * @return true - Is in Wheel mode
   * @return false - Is in joint mode
   */
  bool isWheel();


  void setWheelSpeed(int32_t speed);


  void setJointSpeed(int32_t speed);


  void setGoalPosition(int32_t goal);

  void setGoalPositionNoRx(int32_t goal);


  /**
   * @brief Set the motor to the desired mode (Wheel or Joint)
   *
   * @param mode WHEEL_MODE or JOINT_MODE
   */
  void setMode(modes mode);

  /**
   * @brief return the motor's id.
   *
   * @return uint8_t motor's id
   */
  uint8_t getID() const;

  int16_t getActualPosition() const;

  void setTorqueOn();
  void setTorqueOff();

  uint16_t getCapabilities() const;

 
};

#endif