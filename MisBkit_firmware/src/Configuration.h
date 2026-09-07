#ifndef MBK_CONFIGURATION_H
#define MBK_CONFIGURATION_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include "Pins.h"
#include "sensors/units/UnitModel.h"
#include "sensors/ports/PortType.h"

#define DEFAULT_ROUTER_SSID "MisBKit00"
#define DEFAULT_ROUTER_PSWD "ensadmbk00"

struct Iconfiguration : public Printable{
  bool updated = false;
  virtual void add_to_json(JsonDocument& doc) = 0;
  virtual void set_from_json(const JsonDocument& doc) = 0;
  virtual size_t printTo(Print& p) const = 0;
};

struct kitConfiguration : Iconfiguration{
    const uint8_t v_maj = V_MAJ;
    const uint8_t v_min = V_MIN;
    const uint8_t v_patch = V_PATCH;
    
    void add_to_json(JsonDocument& doc) override {

    }

    String v_as_str() const {
      char buf[16];
      snprintf(buf, sizeof(buf), "%d.%d.%d", v_maj, v_min, v_patch);
      return String(buf);
    }


    void set_from_json(const JsonDocument& doc ) {

    }

    size_t printTo(Print& p) const {
        size_t n = 0;
      char buf[64];
      snprintf(buf, sizeof(buf), "Kit config:\n    Version: %s",
           v_as_str().c_str());
      n += p.print(buf);
        return n;
    }
};

struct networkConfiguration : Iconfiguration{
    char ssid[32]{DEFAULT_ROUTER_SSID};
    char pswd[64]{DEFAULT_ROUTER_PSWD};
    bool dhcp{false};
    IPAddress ip{192,168,0,125};

    void add_to_json(JsonDocument& doc) override {
      doc["ssid"] = ssid;
      doc["pswd"] = pswd;
      doc["dhcp"] = dhcp;
      doc["ip"][0] = ip[0];
      doc["ip"][1] = ip[1];
      doc["ip"][2] = ip[2];
      doc["ip"][3] = ip[3];
    }

    void set_from_json(const JsonDocument& doc ){
      const char* in_ssid = doc["ssid"] | ssid;
      const char* in_pswd = doc["pswd"] | pswd;
      snprintf(ssid, sizeof(ssid), "%s", in_ssid);
      snprintf(pswd, sizeof(pswd), "%s", in_pswd);

      dhcp = doc["dhcp"] | dhcp;

      JsonArrayConst ip_arr = doc["ip"].as<JsonArrayConst>();
      if (ip_arr.size() == 4) {
        ip[0] = ip_arr[0].as<uint8_t>();
        ip[1] = ip_arr[1].as<uint8_t>();
        ip[2] = ip_arr[2].as<uint8_t>();
        ip[3] = ip_arr[3].as<uint8_t>();
      }
    }

    size_t printTo(Print& p) const {
        size_t n = 0;
      char buf[160];
      snprintf(buf, sizeof(buf),
           "Net config:\n    ssid: %s pswd: %s\n    ip: %d.%d.%d.%d\n    dhcp : %d",
           ssid, pswd, ip[0], ip[1], ip[2], ip[3], dhcp);
      n += p.print(buf);
        return n;
    }
};

struct rcConfiguration : Iconfiguration {
  static constexpr uint8_t k_max_channels = 4U;
  static constexpr uint16_t k_default_min_angle = 0U;
  static constexpr uint16_t k_default_max_angle = 180U;
  static constexpr uint16_t k_default_speed_dps = 90U;
  static constexpr uint8_t k_default_easing_type = 0U;

  bool channel_enabled[k_max_channels]{false};
  uint16_t channel_min_angle[k_max_channels]{k_default_min_angle};
  uint16_t channel_max_angle[k_max_channels]{k_default_max_angle};
  uint16_t channel_default_speed_dps[k_max_channels]{k_default_speed_dps};
  uint8_t channel_easing_type[k_max_channels]{k_default_easing_type};

  rcConfiguration() {
    reset_defaults();
  }

  void reset_defaults() {
    for (uint8_t i = 0; i < k_max_channels; ++i) {
      channel_enabled[i] = false;
      channel_min_angle[i] = k_default_min_angle;
      channel_max_angle[i] = k_default_max_angle;
      channel_default_speed_dps[i] = k_default_speed_dps;
      channel_easing_type[i] = k_default_easing_type;
    }
  }

  void apply_channel_json(uint8_t index, JsonObjectConst channel) {
    channel_enabled[index] = channel["enabled"] | channel_enabled[index];
    channel_default_speed_dps[index] = channel["default_speed_dps"] | channel_default_speed_dps[index];

    const uint16_t min_angle = channel["min_angle"] | channel_min_angle[index];
    const uint16_t max_angle = channel["max_angle"] | channel_max_angle[index];
    if (min_angle > max_angle) {
      channel_min_angle[index] = max_angle;
      channel_max_angle[index] = min_angle;
    } else {
      channel_min_angle[index] = min_angle;
      channel_max_angle[index] = max_angle;
    }

    channel_easing_type[index] = channel["easing_type"] | channel_easing_type[index];
  }

  void add_to_json(JsonDocument& doc) override {
    JsonObject rc = doc["rc"].to<JsonObject>();
    JsonArray channels_json = rc["channels"].to<JsonArray>();
    for (uint8_t i = 0; i < k_max_channels; ++i) {
      JsonObject channel = channels_json.add<JsonObject>();
      channel["enabled"] = channel_enabled[i];
      channel["min_angle"] = channel_min_angle[i];
      channel["max_angle"] = channel_max_angle[i];
      channel["default_speed_dps"] = channel_default_speed_dps[i];
      channel["easing_type"] = channel_easing_type[i];
    }
  }

  void set_from_json(const JsonDocument& doc) override {
    reset_defaults();

    JsonObjectConst rc = doc["rc"].as<JsonObjectConst>();
    if (rc.isNull()) {
      updated = true;
      Serial.println("RC is null");
      return;
    }


    JsonArrayConst channels_json = rc["channels"].as<JsonArrayConst>();
    if (!channels_json.isNull()) {
      uint8_t fallback_index = 0U;
      for (JsonVariantConst channel_variant : channels_json) {
        if (fallback_index >= k_max_channels) {
          break;
        }

        JsonObjectConst channel = channel_variant.as<JsonObjectConst>();
        if (channel.isNull()) {
          ++fallback_index;
          continue;
        }

        uint8_t target_index = channel["channel"] | fallback_index;
        if (target_index >= k_max_channels) {
          target_index = fallback_index;
        }

        apply_channel_json(target_index, channel);
        ++fallback_index;
      }
    }

    updated = true;
  }

  size_t printTo(Print& p) const override {
    size_t n = 0;
    char buf[256];

    n += p.print("RC config:\n");
    for (uint8_t i = 0; i < k_max_channels; ++i) {
      snprintf(buf,
               sizeof(buf),
               "    channel %u: enabled=%d min=%u max=%u speed=%u easing=%u\n",
               static_cast<unsigned>(i),
               channel_enabled[i],
               static_cast<unsigned>(channel_min_angle[i]),
               static_cast<unsigned>(channel_max_angle[i]),
               static_cast<unsigned>(channel_default_speed_dps[i]),
               static_cast<unsigned>(channel_easing_type[i]));
      n += p.print(buf);
    }
    return n;
  }
};


// Unit Json structure
//    {
// 					"model": "sensor_generic",
// 					"io": 0,
// 					"alpha": 0.5,
//          "address": 0,
// 					"channels": [
// 						"analog",
// 						"digital"
// 					]
// 				}
// 			]
// 		},
struct UnitConfig  {
    uint8_t id{0};
    UnitModel model{UnitModel::none};
    bool enabled{false};
    float alpha{0.5f};
    uint8_t addr{0};
    const char* const* channels;
    uint8_t           channel_count;

    UnitConfig() = default;
    explicit UnitConfig(UnitModel m) : model(m) {}

void write_to_json(JsonObject& obj) const {
    obj["id"]=id;
    obj["model"] = model_to_str(model);
    obj["io"]    = static_cast<int>(enabled);
    obj["alpha"] = alpha;
    if (addr >= 0) obj["addr"] = addr;

    uint8_t ch_count = 0;
    const char* const* ch_names = get_channel_names(model, ch_count);
    if (ch_count > 0 && ch_names != nullptr) {
        JsonArray chArr = obj["channels"].to<JsonArray>();
        for (uint8_t c = 0; c < ch_count; c++) {
            chArr.add(ch_names[c]);
        }
    }
}
};


// {
// 			"type": "portC",
// 			"units": [{},{},{},{}]
// 		},
struct PortConfig  {
  static constexpr uint8_t k_max_units = 8;
  uint8_t id {0};
  PortType type{PortType::port_c};
  UnitConfig units[k_max_units]{};
  uint8_t unit_count{0};

  PortConfig() = default;
  explicit PortConfig(PortType t) : type(t) {
    UnitConfig c;
    c.model = UnitModel::sensor_generic;
    c.addr = 0;
    c.channels= k_ch_generic;
    c.channel_count = 2;

    add_unit(c);
  }

  void clear_units() {
    unit_count = 0;
    for (size_t j = 0; j < k_max_units; ++j) {
      units[j] = UnitConfig();
    }
  }

  bool add_unit(const UnitConfig& u) {
    if (unit_count >= k_max_units) return false;
    units[unit_count] = u;
    units[unit_count].addr = -1;
    unit_count++;
    return true;
  }

  void write_to_json(JsonObject& obj) const {
    obj["id"] = id;
    obj["type"] = port_type_to_string(type);
  }

  void set_from_json(JsonObjectConst obj) {
    clear_units();

    const char* type_str = obj["type"] | "";
    if (type_str && type_str[0] != '\0') {
      type = port_type_from_string(type_str);
      if (type == PortType::none) {
        Log.warningln("PortConfig.set_from_json: invalid port type '%s'", type_str);
      }
    }

    JsonArrayConst unitsArr = obj["units"].as<JsonArrayConst>();
    if (unitsArr.isNull()) return;

    for (JsonVariantConst u : unitsArr) {
      if (unit_count >= k_max_units) {
        Log.warningln("PortConfig: reached k_max_units (%u), skipping remaining units", (unsigned)k_max_units);
        break;
      }

      const char* model_str = u["model"] | "";
      units[unit_count].model = model_str_to_enum(model_str);
      units[unit_count].enabled = u["io"].as<bool>() | false;
      units[unit_count].alpha = u["alpha"] | 0.0f;
      units[unit_count].addr = u["addr"] | -1;
      unit_count++;
    }
  }

  size_t print(Print& p, uint8_t port_index) const {
    size_t n = 0;
    char buf[256];
    const char* type_str = port_type_to_string(type);
    snprintf(buf, sizeof(buf), "  Port %u: type=%s units=%u\n", (unsigned)port_index, type_str, (unsigned)unit_count);
    n += p.print(buf);

    for (uint8_t u = 0; u < unit_count; u++) {
      const UnitConfig& uc = units[u];
      const char* model = model_to_str(uc.model);
      const char* addr_str = (uc.addr >= 0) ? "set" : "none";
      snprintf(buf, sizeof(buf), "    [%u] model=%s enabled=%d alpha=%.3f addr=%s\n",
               (unsigned)u, model, uc.enabled ? 1 : 0, uc.alpha, addr_str);
      n += p.print(buf);
    }
    if (unit_count == 0) {
      n += p.print("    (no units)\n");
    }
    return n;
  }
};



struct sensorConfiguration : Iconfiguration {

  PortConfig ports[MBK_PORT_COUNT]{
    PortConfig{PortType::port_c}, 
    PortConfig{PortType::port_c}, 
    PortConfig{PortType::port_c}, 
    PortConfig{PortType::port_c}, 
    PortConfig{PortType::port_a}
  };

  void add_to_json(JsonDocument& doc) override {
    JsonArray portsArr = doc["ports"].to<JsonArray>();
    for (size_t i = 0; i < MBK_PORT_COUNT; i++) {
      JsonObject portObj = portsArr.add<JsonObject>();
      ports[i].write_to_json(portObj);
    }
  }

  void set_from_json(const JsonDocument& doc) override {
    if (!doc["ports"].is<JsonArrayConst>()) {
      Log.warningln("sensorConfiguration.set_from_json: missing or invalid ports array");
      return;
    }

    JsonArrayConst portsArr = doc["ports"].as<JsonArrayConst>();
    size_t n = portsArr.size();

    for (size_t i = 0; i < MBK_PORT_COUNT; i++) {
      if (i >= n) {
        ports[i] = PortConfig();
        continue;
      }
      ports[i].set_from_json(portsArr[i].as<JsonObjectConst>());
    }
    updated = true;
  }

  size_t printTo(Print& p) const {
    size_t n = 0;
    n += p.print("Sensor configuration:\n");
    for (size_t i = 0; i < MBK_PORT_COUNT; i++) {
      n += ports[i].print(p, i);
    }
    return n;
  }
};
#endif // MBK_CONFIGURATION_H