#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "CircularBuffer.h" 
#include "ICommunicationProtocol.h"

#define SOCKET_DATA_SIZE 2000

class MotorManager;


class WebSocketProtocol : public ICommunicationProtocol {
    private:
        AsyncWebServer* server;
        AsyncWebSocket ws{"/ws"};
        int clientID = -1;
        
        CircularBuffer<64> message_buffer;
        char socketData[SOCKET_DATA_SIZE];
        int currSocketBufferIndex = 0;

        MotorManager* pmotor_manager_ = NULL;
        
        // Callbacks
        std::function<void(const Command&)> messageHandler;
        std::function<void(IPAddress)> connectionHandler;
        std::function<void()> disconnectionHandler;
        
        // Event handlers
        void eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
        void handleWebSocketMessage(void* arg, uint8_t* data, size_t len);
    
    public:
        WebSocketProtocol(AsyncWebServer* server);
        ~WebSocketProtocol() override = default;
        
        void initialize() override;
        void setMotorManager( MotorManager* pmotor_manager ) { pmotor_manager_ = pmotor_manager;}
        void update() override;
        void sendCommand(const JsonDocument& cmd) override;
        void reply(const char *cmd, JsonDocument& val) override;

        
        inline void setMessageHandler(std::function<void(const Command&)> handler) override{
            messageHandler = handler;
        }
        inline void setConnectionHandler(std::function<void(IPAddress)> handler) override{
            connectionHandler = handler;
        }

        inline void setDisconnectionHandler(std::function<void()> handler) override{
            disconnectionHandler = handler;
        }


};

#endif