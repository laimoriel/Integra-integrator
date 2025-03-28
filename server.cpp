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


// Init websocket for sending updates on state of the system to web browser
void initWebSocket(void) {
  server.addHandler(&ws);
}


// Send notification containing updates of the system state
void notifyClients(String notification) {
  ws.textAll(notification);
}


// Send WhatsApp notifications to preconfigured phone number via callmebot service api
void sendWhatsAppMessage(String message) {
  extern String phoneNumber;
  extern String apiKey;
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);   
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(url);
  http.end();
}


// Handle GET parameters from settings page and write them to EEPROM
void handleParamSettings(String const &name, String const &value) {
  extern String ssidClient;
  extern String passClient;
  extern String phoneNumber;
  extern String apiKey;
  extern uint16_t port_1;
  extern uint16_t port_2;
  extern uint32_t bps_1;
  extern uint32_t bps_2;
  extern uint8_t mode_1;
  extern uint8_t mode_2;
  if (name == "port_1") {
    port_1 = value.toInt(); 
    EEPROM.write(0, port_1);
    EEPROM.write(1, port_1 >> 8);
    EEPROM.commit();
  }
  if (name == "port_2") {
    port_2 = value.toInt(); 
    EEPROM.write(2, port_2);
    EEPROM.write(3, port_2 >> 8);
    EEPROM.commit();
  }
  if (name == "bps_1") {
    bps_1 = value.toInt(); 
    EEPROM.write(4, bps_1);
    EEPROM.write(5, bps_1 >> 8);
    EEPROM.commit();
    uartInit();
  }
  if (name == "bps_2") {
    bps_2 = value.toInt(); 
    EEPROM.write(6, bps_2);
    EEPROM.write(7, bps_2 >> 8);
    EEPROM.commit();
    uartInit();
  }
  if (name == "mode_1") {
    mode_1 = value.toInt();
    EEPROM.write(8, mode_1);
    EEPROM.commit();
    uartInit();
  }
  if (name == "mode_2") {
    mode_2 = value.toInt();
    EEPROM.write(9, mode_2);
    EEPROM.commit();
    uartInit();
  }
  if (name == "ssid") {
    ssidClient = value;
    EEPROM.writeString(EEPROM_SSID_OFFSET, ssidClient);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + 1, passClient);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + 1, phoneNumber);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 1, apiKey);
    EEPROM.commit();
  }
  if (name == "pass") {
    passClient = value;
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + 1, passClient);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + 2, phoneNumber);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 3, apiKey);
    EEPROM.commit();
  }
  if (name == "phone") {
    phoneNumber = value;
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + 2, phoneNumber);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 3, apiKey);
    EEPROM.commit();
  }
  if (name == "apikey") {
    apiKey = value;
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 3, apiKey);
    EEPROM.commit();
  }
}


// Fill corresponding placeholders in the settings HTML code with variables
String processorSettings(const String & var) {
  extern String ssidClient;
  extern String phoneNumber;
  extern uint16_t port_1;
  extern uint16_t port_2;
  extern uint32_t bps_1;
  extern uint32_t bps_2;
  extern uint8_t mode_1;
  extern uint8_t mode_2;
  if (var == "SSID") return ssidClient;
  else if (var == "PORT1") return String(port_1);
  else if (var == "PORT2") return String(port_2);
  else if (var == "BPS1") return String(bps_1);
  else if (var == "BPS2") return String(bps_2);
  else if (var == "MODE1") return mode2code(mode_1);
  else if (var == "MODE2") return mode2code(mode_2);
  else if (var == "PHONE") return String(phoneNumber);
  else return "n/a";
}


// Convert UART communication mode codes into readable form
String mode2code(uint16_t mode) {
  switch (mode) {                
    case 16: return "5N1";
    case 48: return "5N2"; 
    case 18: return "5E1"; 
    case 50: return "5E2"; 
    case 19: return "5O1"; 
    case 51: return "5O2"; 
    case 20: return "6N1"; 
    case 52: return "6N2"; 
    case 22: return "6E1"; 
    case 54: return "6E2"; 
    case 23: return "6O1"; 
    case 55: return "6O2"; 
    case 24: return "7N1"; 
    case 56: return "7N2"; 
    case 26: return "7E1"; 
    case 58: return "7E2"; 
    case 27: return "7O1"; 
    case 59: return "7O2"; 
    case 28: return "8N1"; 
    case 60: return "8N2"; 
    case 30: return "8E1"; 
    case 62: return "8E2"; 
    case 31: return "8O1"; 
    case 63: return "8O2"; 
  }
}