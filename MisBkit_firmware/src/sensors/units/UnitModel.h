/**
 * @file UnitModel.h
 * @author Etienne Montenegro
 * @brief Concrete module model selected by the user for a port.
 */

#ifndef UNIT_MODEL_H
#define UNIT_MODEL_H

#include <cstring>
#include <stdint.h>



enum class UnitModel : uint8_t {
  none = 0,

  // Generic profiles
  sensor_generic,
  
  // Sensor modules
  sensor_mic,
  sensor_light,
  sensor_ultrasonic_io,
  sensor_tof4m,
  sensor_tvoc,
  sensor_ultrasonic_i2c,
  sensor_dlight,
  sensor_accel,
  sensor_pahub,
  
  // HMI modules
  hmi_button,
  hmi_fader,
  hmi_angle,
  hmi_8angle
};

// Channel name arrays — one per model, order matches write_json() output order.
static constexpr const char* k_ch_generic[]        = {"analog", "digital"};
static constexpr const char* k_ch_mic[]            = {"level"};
static constexpr const char* k_ch_light[]          = {"lux"};
static constexpr const char* k_ch_ultrasonic_io[]  = {"distance"};
static constexpr const char* k_ch_tof4m[]          = {"distance"};
static constexpr const char* k_ch_tvoc[]           = {"tvoc", "eco2"};
static constexpr const char* k_ch_ultrasonic_i2c[] = {"distance"};
static constexpr const char* k_ch_dlight[]         = {"lux"};
static constexpr const char* k_ch_accel[]          = {"accelx", "accely", "accelz", "gyrox", "gyroy", "gyroz", "temp"};
static constexpr const char* k_ch_button[]         = {"pressed"};
static constexpr const char* k_ch_fader[]          = {"value"};
static constexpr const char* k_ch_angle[]          = {"angle"};
static constexpr const char* k_ch_8angle[]         = {"angle1", "angle2", "angle3", "angle4", "angle5", "angle6", "angle7", "angle8", "switch"};

// i2c_address == 0 means the model is not I2C-detectable.
struct UnitModelEntry {
  UnitModel         model;
  const char*       name;
  uint8_t           i2c_address;
  const char* const* channels;
  uint8_t           channel_count;
};



// Keep this table in sync with the UnitModel enum.
constexpr UnitModelEntry k_unit_model_map[] = {
    {UnitModel::none,                  "none",                  0,     nullptr,             0},
    {UnitModel::sensor_generic,        "sensor_generic",        0,     k_ch_generic,        2},
    {UnitModel::sensor_mic,            "sensor_mic",            0,     k_ch_mic,            1},
    {UnitModel::sensor_light,          "sensor_light",          0,     k_ch_light,          1},
    {UnitModel::sensor_ultrasonic_io,  "sensor_ultrasonic_io",  0,     k_ch_ultrasonic_io,  1},
    {UnitModel::sensor_tof4m,          "sensor_tof4m",          0x29,  k_ch_tof4m,          1},
    {UnitModel::sensor_tvoc,           "sensor_tvoc",           0x58,  k_ch_tvoc,           2},
    {UnitModel::sensor_ultrasonic_i2c, "sensor_ultrasonic_i2c", 0x57,  k_ch_ultrasonic_i2c, 1},
    {UnitModel::sensor_dlight,         "sensor_dlight",         0x23,  k_ch_dlight,         1},
    {UnitModel::sensor_accel,          "sensor_accel",          0x68,  k_ch_accel,          7},
    {UnitModel::sensor_pahub,          "sensor_pahub",          0x70,  nullptr,             0},
    {UnitModel::sensor_pahub,          "sensor_pahub",          0x71,  nullptr,             0},
    {UnitModel::sensor_pahub,          "sensor_pahub",          0x72,  nullptr,             0},
    {UnitModel::sensor_pahub,          "sensor_pahub",          0x73,  nullptr,             0},
    {UnitModel::sensor_pahub,          "sensor_pahub",          0x74,  nullptr,             0},
    {UnitModel::sensor_pahub,          "sensor_pahub",          0x75,  nullptr,             0},
    {UnitModel::sensor_pahub,          "sensor_pahub",          0x76,  nullptr,             0},
    {UnitModel::sensor_pahub,          "sensor_pahub",          0x77,  nullptr,             0},
    {UnitModel::hmi_button,            "hmi_button",            0,     k_ch_button,         1},
    {UnitModel::hmi_fader,             "hmi_fader",             0,     k_ch_fader,          1},
    {UnitModel::hmi_angle,             "hmi_angle",             0,     k_ch_angle,          1},
    {UnitModel::hmi_8angle,            "hmi_8angle",            0x43,  k_ch_8angle,         9},
};

inline const char* const* get_channel_names(UnitModel m, uint8_t& out_count) {
  for (const auto& entry : k_unit_model_map) {
    if (entry.model == m) {
      out_count = entry.channel_count;
      return entry.channels;
    }
  }
  out_count = 0;
  return nullptr;
}

inline const char* model_to_str(UnitModel m) {
  for (const auto& entry : k_unit_model_map) {
    if (entry.model == m) {
      return entry.name;
    }
  }

  return "none";
}

inline UnitModel model_str_to_enum(const char* model) {
  if (model == nullptr) {
    return UnitModel::none;
  }

  for (const auto& entry : k_unit_model_map) {
    if (strcmp(entry.name, model) == 0) {
      return entry.model;
    }
  }

  return UnitModel::none;
}

inline UnitModel model_from_i2c_address(uint8_t address) {
  for (const auto& entry : k_unit_model_map) {
    if (entry.i2c_address != 0 && entry.i2c_address == address) {
      return entry.model;
    }
  }
  return UnitModel::none;
}

constexpr uint8_t count_i2c_entries() {
  uint8_t n = 0;
  for (const auto& entry : k_unit_model_map) {
    if (entry.i2c_address != 0) n++;
  }
  return n;
}

// Fixed-size result type for I2C autodetection — no heap allocation.
constexpr uint8_t k_i2c_detectable_count = count_i2c_entries();

struct DetectedModels {
  UnitModel models[k_i2c_detectable_count]{};
  uint8_t   count{0};

  void push(UnitModel m) {
    if (count < k_i2c_detectable_count) {
      models[count++] = m;
    }
  }
};


#endif  // UNIT_MODEL_H