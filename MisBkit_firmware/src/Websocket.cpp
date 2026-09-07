#include "Websocket.h"
#include <ArduinoLog.h>

#include "motors/MotorManager.h"

WebSocketProtocol::WebSocketProtocol(AsyncWebServer* server) : server(server) {
}

void WebSocketProtocol::initialize(){
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        this->eventHandler(server, client, type, arg, data, len);
    });

  server->addHandler(&ws).addMiddleware([this](AsyncWebServerRequest *request,
                                          ArMiddlewareNext next) {

    if (ws.count() > 0) {
      request->send(503, "text/plain", "Server is busy. Max client reached");
    } else {
      // process next middleware and at the end the handler
      next();
    }
  });

  server->addHandler(&ws);

  Log.infoln("Websocket protocol initialized");
}

void WebSocketProtocol::update(){
    if(messageHandler && !message_buffer.isEmpty()){
        const Command& cmd = message_buffer.pop();
        messageHandler(cmd);
    }
  

    static uint32_t ln = 0;
    uint32_t n = millis();
    if (n - ln > 2000) {
        ln = n;
        ws.cleanupClients(1);
    }



}

void  WebSocketProtocol::sendCommand(const JsonDocument& cmd) { 
  const size_t len = measureJson(cmd);
  AsyncWebSocketMessageBuffer* buff = ws.makeBuffer(len);
  serializeJson(cmd, buff->get(), len);
  ws.textAll(buff); 
}

void  WebSocketProtocol::reply(const char *cmd, JsonDocument& val) {
  JsonDocument _reply;
  _reply["reply"] = cmd;
  _reply["val"] = val.as<JsonObject>();
  // Commenting next output
  //serializeJson(val,Serial);
  //Serial.println();
  sendCommand(_reply);
}

void  WebSocketProtocol::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  for (size_t i = 0; i < len; i++) {
    socketData[currSocketBufferIndex] = data[i];
    currSocketBufferIndex++;
  }

  if (currSocketBufferIndex >= info->len && info->final) {
      socketData[currSocketBufferIndex] = '\0';
      currSocketBufferIndex = 0;
      if(!message_buffer.isFull()){
        message_buffer.push(socketData,len);
      }
  }
}

void  WebSocketProtocol::eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client,
                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      if(connectionHandler){
        Log.info("WebSocket client #%u connected from %s\n", client->id(),
        client->remoteIP().toString().c_str());
        client->setCloseClientOnQueueFull(false);
        clientID = client->id();
        connectionHandler(client->remoteIP());
      }
      break;

    case WS_EVT_DISCONNECT:
        if(disconnectionHandler && clientID == client->id()){
            Log.info("WebSocket client #%u disconnected\n", client->id());
            disconnectionHandler();
        }
      break;

    case WS_EVT_DATA: {
        handleWebSocketMessage(arg, data, len);
      break;
    }

    case WS_EVT_PONG:
      Log.traceln("pong");
      break;

    case WS_EVT_ERROR:
      Log.errorln("WS Error");
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      for (size_t i = 0; i < len; i++) {
        socketData[currSocketBufferIndex] = data[i];
        currSocketBufferIndex++;
      }
      socketData[currSocketBufferIndex] = '\0';
      Serial.printf("%s\n", socketData);
      Serial.println(info->opcode);
      break;
  }
}

