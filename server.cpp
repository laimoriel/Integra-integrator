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
      ws.cleanupClients(2);
      break;
    case WS_EVT_DISCONNECT:
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    char payload[14];
    uint8_t payloadSize = 0;
    uint8_t mode = *data - '0';
    if (mode == 1) {
      payloadSize = 14;
      payload[0] = 0x70;
      payload[13] = 0x00;
    }
    else if (mode == 0) {
      payloadSize = 13;
      payload[0] = 0x71;
    }
    else if (mode == 2) {
      payloadSize = 13;
      payload[0] = 0x72;
    }
    // Convert 2 character decimal number to a 32-bit register indicating zone number
    uint32_t zone = 1 << ((*(data + 1) - '0') * 10 + (*(data + 2) - '0'));
    payload[9] = zone & 0XFF;
    payload[10] = (zone >> 8) & 0xFF;
    payload[11] = (zone >> 16) & 0xFF;
    payload[12] = zone >> 24;
    // Convert password from array of decimal chars into string compliant with Satel protocol
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t c16 = (2 * i < len - 3) ? (*(data + 2 * i + 3) - '0') : (0xA);
      uint8_t c1 = (2 * i + 1 < len - 3) ? (*(data + 2 * i + 4) - '0') : (0xA);
      payload[i + 1] = 16 * c16 + c1;
    }
    dispatchFrame(payload, payloadSize);
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

