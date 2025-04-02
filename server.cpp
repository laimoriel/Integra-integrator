#include "config.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


// Standard web server init
void webServerInit(void) {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){request->redirect("/panel");});
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){request->send(LittleFS, "/settings.html", String(), false, processorSettings);});
  server.on("/sendframe", HTTP_GET, [](AsyncWebServerRequest *request){request->send(LittleFS, "/sendframe.html", String(), false, processorIntegra);});
  server.on("/panel", HTTP_GET, [](AsyncWebServerRequest *request){request->send(LittleFS, "/panel.html", String(), false, processorIntegra);});
  server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *request){request->send(LittleFS, "/stats.html", String(), false, processorStats);});
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){request->send(LittleFS, "/style.css", "text/css");});
  server.on("/settingsget", HTTP_GET, [](AsyncWebServerRequest *request){handleGetSettings(request);});
  server.on("/sendframeget", HTTP_GET, [](AsyncWebServerRequest *request){handleGetSendFrame(request);});
  server.on("/panelget", HTTP_GET, [](AsyncWebServerRequest *request){handleGetPanel(request);});
  server.onNotFound(notFound);
  server.begin();
}


// Handle GET request from sendframe.html
void handleGetSendFrame(AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i = 0; i < params; i++) {
    const AsyncWebParameter* p = request->getParam(i);
    handleParamSendFrame(p->name().c_str(), p->value().c_str());
  }
  request->redirect("/sendframe");
}


// Handle GET request from panel.html
void handleGetPanel(AsyncWebServerRequest *request) {
  extern uint8_t payloadReady;
  payloadReady = 0;
  int params = request->params();
  for(int i = 0; i < params; i++) {
    const AsyncWebParameter* p = request->getParam(i);
    handleParamPanel(p->name().c_str(), p->value().c_str());
  }
  request->redirect("/panel");
}


// Handle GET request from settings.html
void handleGetSettings(AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i = 0; i < params; i++) {
    const AsyncWebParameter* p = request->getParam(i);
    handleParamSettings(p->name().c_str(), p->value().c_str());
  }
  request->redirect("/settings");
}


// What to do if url not found
void notFound(AsyncWebServerRequest *request) {
   request->redirect("/");
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      ws.cleanupClients(3);
      break;
    case WS_EVT_DISCONNECT:
      break;
    case WS_EVT_DATA:
      //handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}


// Init websocket for sending updates on state of the system to web browser
void initWebSocket(void) {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


// Send notification containing updates of the system state
void notifyClients(String notification) {
  ws.textAll(notification);
}


// Send WhatsApp notifications to preconfigured phone number via callmebot service api
void sendWhatsAppMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    extern String phoneNumber;
    extern String apiKey;
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);   
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST(url);
    http.end();
  }
}

