/**
 * @file network.cpp
 * @author Etienne Montenegro
 * @brief Implementation file of network.h
 *
 */
#include "NetworkManager.h"

#include <WiFiAP.h>

#include "Configuration.h"
#include "Led.h"

void NetworkHandler::WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Log.warningln("WiFi Connected to %s", info.wifi_sta_connected.ssid);
}

void NetworkHandler::WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    auto ip = info.got_ip.ip_info.ip.addr;
    Log.warningln("IP address: [%d.%d.%d.%d]", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
}

void NetworkHandler::WiFiAPStart(WiFiEvent_t event, WiFiEventInfo_t info) {
    Log.warningln("AP started, SSID: %s", WiFi.softAPSSID().c_str());
}

void NetworkHandler::WiFiAPStop(WiFiEvent_t event, WiFiEventInfo_t info) {
    Log.warningln("AP stopped");
}

void NetworkHandler::WiFiAPClientConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    auto& mac = info.wifi_ap_staconnected.mac;
    Log.warningln("AP client connected, AID: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                  info.wifi_ap_staconnected.aid,
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void NetworkHandler::WiFiAPClientDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    auto& mac = info.wifi_ap_stadisconnected.mac;
    Log.warningln("AP client disconnected, AID: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                  info.wifi_ap_stadisconnected.aid,
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

IPAddress NetworkHandler::get_ip() {
  if (WiFi.getMode() == WIFI_MODE_AP) {
    return WiFi.softAPIP();
  }
  return WiFi.localIP();
}

void NetworkHandler::create_ap() {
  _logger.traceln("Configuring access point...");
  led::setColor(255,255,0, true);
  WiFi.mode(WIFI_MODE_AP);

  if(!WiFi.softAP(ap_ssid, ap_pswd)){
     led::error();
    _logger.errorln("Failed to set up access point");
  }

  delay(500);

  if (!mConfig->dhcp) {
    _logger.traceln(" using static ip from config");
    IPAddress lease_start = mConfig->ip;
    lease_start[3] = lease_start[3]+1;
    if(!WiFi.softAPConfig(mConfig->ip, mConfig->ip, subnet, lease_start)){
      _logger.errorln("Failed to configure AP");
    }else{
      _logger.infoln("Access point created. IP address %p", get_ip());
    }
  }else{
    _logger.traceln("using dhcp");
  }

  delay(500);
}

void NetworkHandler::set_wifi_events() {
  event_connect_id = WiFi.onEvent( WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  event_ip_id = WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  event_ap_start_id = WiFi.onEvent(WiFiAPStart, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_START);
  event_ap_stop_id = WiFi.onEvent(WiFiAPStop, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STOP);
  event_ap_staconnected_id = WiFi.onEvent(WiFiAPClientConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  event_ap_stadisconnected_id = WiFi.onEvent(WiFiAPClientDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
}

void NetworkHandler::remove_wifi_events() {
  WiFi.removeEvent(event_connect_id);
  WiFi.removeEvent(event_ip_id);

  WiFi.removeEvent(event_ap_start_id);
  WiFi.removeEvent(event_ap_stop_id);
  WiFi.removeEvent(event_ap_staconnected_id);
  WiFi.removeEvent(event_ap_stadisconnected_id);
}


bool NetworkHandler::connect(const char* ssid, const char* pswd, uint16_t timeout) {
  unsigned long start_time = millis();
  unsigned long last_dot = 0;

  _logger.infoln("Connecting to SSID: %s", ssid);
  WiFi.begin(ssid, pswd);

  while (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    auto s = WiFi.status();
    if (s == WL_NO_SSID_AVAIL || s == WL_CONNECT_FAILED) {
      if (now - start_time > 2000) {
        _logger.warningln("Network unreachable or wrong password");
        return false;
      }
    }

    if (now - start_time > timeout) {
      break;
    }

    yield();
  }

  _logger.traceln(" ");
  _logger.setShowLevel(true);

  return WiFi.status() == WL_CONNECTED;
}

bool NetworkHandler::configure_station() {
  _logger.infoln("Network interface configuration for station mode");
  // delete old config
  WiFi.disconnect(true);
  WiFiClass::mode(WIFI_STA);

  if(!mConfig->dhcp){
    _logger.infoln("Network interface configuration for station mode using static IP");

    IPAddress ip = mConfig->ip;
    if (!WiFi.config(mConfig->ip, gateway, subnet))
    {
      _logger.errorln("STA Failed to configure");
      return false;
    }
  } else {
    _logger.infoln("Network interface configuration for station mode using DHCP");

  }
  return true;
}

bool NetworkHandler::connect_to_router(uint8_t max) {
    led::setColor(0,0,255,true);
  if (!configure_station()) {
    led::error();

    return false;
  }

  // // first try: dedicated MisBKit router
  // for (uint8_t i = 0; i < max; i++) {
  //   _logger.infoln("Try number %d (MisBKit router)", i + 1);
  //   _logger.setShowLevel(false);
  //   if (connect(DEFAULT_ROUTER_SSID, DEFAULT_ROUTER_PSWD)) {
  //     return true;
  //   }
  // }

  // second try: user-configured network
  for (uint8_t i = 0; i < max; i++) {
    _logger.infoln("Try number %d (configured network)", i + 1);
    _logger.setShowLevel(false);
    if (connect(mConfig->ssid, mConfig->pswd)) {
      return true;
    }
  }
  led::error();
  return false;
}

void NetworkHandler::printDiag() {
    auto mode = WiFi.getMode();
    _logger.infoln("--- WiFi Diagnostic ---");
    _logger.infoln("Mode: %s", mode == WIFI_MODE_AP ? "AP" : mode == WIFI_MODE_STA ? "STA" : "OFF");
    _logger.infoln("MAC: %s", WiFi.macAddress().c_str());

    if (mode == WIFI_MODE_AP) {
        _logger.infoln("AP SSID: %s", WiFi.softAPSSID().c_str());
        _logger.infoln("AP IP: %s", WiFi.softAPIP().toString().c_str());
        _logger.infoln("AP clients: %d", WiFi.softAPgetStationNum());
    } else if (mode == WIFI_MODE_STA) {
        _logger.infoln("STA SSID: %s", WiFi.SSID().c_str());
        _logger.infoln("STA IP: %s", WiFi.localIP().toString().c_str());
        _logger.infoln("RSSI: %d dBm", WiFi.RSSI());
        _logger.infoln("Gateway: %s", WiFi.gatewayIP().toString().c_str());
        _logger.infoln("Subnet: %s", WiFi.subnetMask().toString().c_str());
    }
    _logger.infoln("--- End WiFi Diagnostic ---");
}

void NetworkHandler::update_ap_credentials(const uint8_t id){
    snprintf(ap_ssid,sizeof(ap_ssid), "mbk_%.3d", id);
    snprintf(ap_pswd,sizeof(ap_pswd), "ensad-mbk%.3d", id);
};


wifi_mode_t NetworkHandler::initialize(){
  wifi_mode_t mode = WIFI_MODE_NULL;
  _logger.warningln("Initializing");
  
  //temporary setting gateway directly from ip. Will see if we add a full network config into user config file
  gateway = IPAddress{mConfig->ip[0],mConfig->ip[1],mConfig->ip[2],1};
  
  set_wifi_events();

  _logger.infoln("Trying to connect to router");

  if (strlen(mConfig->ssid) > 0 && strlen(mConfig->pswd) > 0) {
    if (connect_to_router(connection_attempts)) {
      mode = WIFI_MODE_STA;
      WiFi.setAutoReconnect(true);
      printDiag();
      return mode;
    }
  }
  _logger.errorln("WiFi unable to connect, Creating Access point.");
  create_ap();
  mode = WIFI_MODE_AP;
  printDiag();
  return mode;
}

  
void NetworkHandler::prefix_print(Print* _logOutput, int logLevel) {
  _logOutput->print("[Network] ");
}
