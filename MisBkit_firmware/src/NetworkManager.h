/**
 * @file NetworkManager.h
 * @author Etienne Montenegro
 * @brief File containing all functions and variables related to network
 * communication between MCU and Computer
 *
 */
#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H
#pragma once

#include <WiFi.h>
#include <ArduinoLog.h>

class networkConfiguration;

class NetworkHandler {
 private:

    networkConfiguration* mConfig = nullptr;

    IPAddress gateway{192, 168, 0, 1};
    IPAddress subnet{255, 255, 255, 0};

    // AP SSID AND PASSWORD
    char ap_ssid[8] = "mbk_000";
    char ap_pswd[13] = "ensad-mbk000";

    uint8_t connection_attempts = 1;

    wifi_event_id_t event_connect_id = 0;
    wifi_event_id_t event_ip_id = 0;

    wifi_event_id_t event_ap_start_id = 0;
    wifi_event_id_t event_ap_stop_id = 0;
    wifi_event_id_t event_ap_staconnected_id = 0;
    wifi_event_id_t event_ap_stadisconnected_id = 0;

    Logging _logger;
    static void prefix_print(Print* _logOutput, int logLevel);

    static void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    static void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);

    static void WiFiAPStart(WiFiEvent_t event, WiFiEventInfo_t info);
    static void WiFiAPStop(WiFiEvent_t event, WiFiEventInfo_t info);
    static void WiFiAPClientConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    static void WiFiAPClientDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

    IPAddress get_ip();
    
    bool connect(const char* ssid, const char* pswd, uint16_t timeout = 10000 );
    void create_ap();

    bool connect_to_router(uint8_t max);

    bool configure_station();
    void set_wifi_events();
    
 public:
    NetworkHandler(networkConfiguration* config) : mConfig(config){
        _logger.begin(LOG_LEVEL_TRACE, &Serial);
        _logger.setPrefix(prefix_print);
    }
    
    wifi_mode_t initialize();
    
    void update_ap_credentials(const uint8_t id);
    
    void remove_wifi_events();

    void printDiag();

};

#endif
