#ifndef MBK_COMMAND_H
#define MBK_COMMAND_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>

#define MAX_PAYLOAD_SIZE (512*2+30) // was 512, 1064 is too big, 1054 is ok

enum Command_t {
  CMD_STOP_ALL,
  CMD_SCAN,
  CMD_PING = 100,
  CMD_MOTOR_ID,
  CMD_MOTOR_FACTORY_RESET,
  CMD_DEEP_SCAN,
  CMD_CHANGE_MOTOR_BAUD,
  CMD_MOTOR_CURVE,
  CMD_MOTOR_CURVE_STOP,
  CMD_MOTOR_POSITION,
  CMD_SET_MODE,
  CMD_SET_STIFF, 
  CMD_WHEEL,
  CMD_JOINT,
  CMD_JOINT_VELOCITY,
  CMD_STOP,
  CMD_STATUS_DUMP = 300,
  CMD_SENSOR_CONFIG,
  CMD_SENSOR_DATA,
  CMD_SAVE_SENSOR_CONFIG,
  CMD_SENSOR_SCAN,
  CMD_SYNC_WHEEL,
  CMD_SYNC_JOINT,
  CMD_SYNC_JOINT_VELOCITY,
  CMD_SYNC_STOP,  
  CMD_FORMAT_SPIFFS,

  CMD_RC_JOINT = 400,
  CMD_RC_JOINT_VELOCITY,
  CMD_RC_STOP,
  CMD_RC_INFO,
  CMD_RC_ENABLE,
  CMD_REBOOT = 999,
  CMD_UNKNOWN = -1
};



struct Command{
    char data[MAX_PAYLOAD_SIZE]{};
    int len;
  
    Command() = default;
    Command(const char* cmd, int len) {
      memcpy(data, cmd, len);
      this->len = len;
    }

    bool is_valid(JsonDocument& doc) const {
        DeserializationError error = deserializeJson(doc,data,len);
        if (!error) {
            return true;
        }else{
            Log.errorln("Socket data deserialization error: %s", error.c_str());
            return false;
        }
    }

    bool is_single_command(JsonDocument& doc) const{
        if(doc["cmd"])return true;
        return false;
    }

    static Command_t to_enum(JsonObject action){
        Command_t cmd = CMD_UNKNOWN;
        auto cmd_str = action["cmd"].as<const char *>();

          if (strcmp(cmd_str, "reboot") == 0) {
            cmd = CMD_REBOOT;
          } else if (strcmp(cmd_str, "rc_joint") == 0) {
            cmd = CMD_RC_JOINT;
          } else if (strcmp(cmd_str, "rc_speed") == 0) {
            cmd = CMD_RC_JOINT_VELOCITY;
          } else if (strcmp(cmd_str, "rc_enable") == 0) {
            cmd =  CMD_RC_ENABLE;
          } else if (strcmp(cmd_str, "rc_stop") == 0) {
            cmd = CMD_RC_STOP;
          } else if (strcmp(cmd_str, "rc_info") == 0) {
            cmd = CMD_RC_INFO;
          } else if (strcmp(cmd_str, "stopall") == 0) {
            cmd = CMD_STOP_ALL;
          } else if (strcmp(cmd_str, "scan") == 0) {
            cmd = CMD_SCAN;
          } else if (strcmp(cmd_str, "stop") == 0) {
            cmd = CMD_STOP;
          } else if (strcmp(cmd_str, "motor_position") == 0) {
            cmd = CMD_MOTOR_POSITION;
          } 
          else if (strcmp(cmd_str, "set_mode") == 0) {
            cmd = CMD_SET_MODE;
          } else if (strcmp(cmd_str, "set_stiff") == 0) {
            cmd = CMD_SET_STIFF;
          } else if (strcmp(cmd_str, "wheel") == 0) {
            cmd = CMD_WHEEL;
          } else if (strcmp(cmd_str, "joint") == 0) {
            cmd = CMD_JOINT;
          } else if (strcmp(cmd_str, "speed") == 0) {
            cmd = CMD_JOINT_VELOCITY;
          } else if (strcmp(cmd_str, "infos") == 0) {
            cmd = CMD_STATUS_DUMP;
          } else if (strcmp(cmd_str, "sensorconfig") == 0) {
            cmd = CMD_SENSOR_CONFIG;
          } else if (strcmp(cmd_str, "sensor_scan") == 0) {
            cmd = CMD_SENSOR_SCAN;
          } else if (strcmp(cmd_str, "sensordata") == 0) {
            cmd = CMD_SENSOR_DATA;
          } else if (strcmp(cmd_str, "sensorsave") == 0) {
            cmd = CMD_SAVE_SENSOR_CONFIG;
          } else if (strcmp(cmd_str, "sync_wheel") == 0) {
            cmd = CMD_SYNC_WHEEL;
          }else if (strcmp(cmd_str, "sync_joint") == 0) {
            cmd = CMD_SYNC_JOINT;
          }else if (strcmp(cmd_str, "sync_speed") == 0) {
            cmd = CMD_SYNC_JOINT_VELOCITY;
          }else if (strcmp(cmd_str, "sync_stop") == 0) {
            cmd = CMD_SYNC_STOP;
          }else if (strcmp(cmd_str, "set_id") == 0) {
            cmd = CMD_MOTOR_ID;
          }else if (strcmp(cmd_str, "motor_factory_reset") == 0) {
            cmd = CMD_MOTOR_FACTORY_RESET;
          }else if (strcmp(cmd_str, "deep_scan") == 0) {
            cmd = CMD_DEEP_SCAN;
          }else if (strcmp(cmd_str, "change_motor_baud") == 0) {
            cmd = CMD_CHANGE_MOTOR_BAUD;
          }else if (strcmp(cmd_str, "motor_curve") == 0) {
            cmd = CMD_MOTOR_CURVE;
          }else if (strcmp(cmd_str, "motor_curve_stop") == 0) {
            cmd = CMD_MOTOR_CURVE_STOP;
          }else if (strcmp(cmd_str, "format_spiffs") == 0) {
            cmd = CMD_FORMAT_SPIFFS;
          }
          else{
            Log.errorln("to_enum: Command '%s' does not exist", cmd_str);
          }
          return cmd;
        }
    
  };
  

#endif //MBK_COMMAND_H
