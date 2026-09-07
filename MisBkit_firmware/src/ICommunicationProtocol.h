#ifndef MISBKIT_ICOMMUNICATIONPROTOCOL_H
#define MISBKIT_ICOMMUNICATIONPROTOCOL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Command.h"

class ICommunicationProtocol {
    public:
    virtual ~ICommunicationProtocol() = default;
    
    // Core communication methods
    virtual void initialize() = 0;
    virtual void update() = 0;
    virtual void sendCommand(const JsonDocument& cmd) = 0;
    virtual void reply(const char *cmd, JsonDocument& val) = 0;
    
    
    // Message handling callbacks
    virtual void setMessageHandler(std::function<void(const Command&)> handler) = 0;
    virtual void setConnectionHandler(std::function<void(IPAddress)> handler) = 0;
    virtual void setDisconnectionHandler(std::function<void()> handler) = 0;
};




#endif  // MISBKIT_ICOMMUNICATIONPROTOCOL_H