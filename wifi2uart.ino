#include "config.h"

// Variables stored in EEPROM
// WiFi credentials
String ssidClient;
String passClient;
// WhatsApp credentials
String phoneNumber;
String apiKey;
// TCP socket ports
uint16_t port_1;
uint16_t port_2;
// UART ports paramaeters
uint32_t bps_1;
uint32_t bps_2;
uint8_t mode_1;
uint8_t mode_2;


//extern AsyncWebSocket ws;

// TCP socket servers
WiFiServer server_1(0);
WiFiServer server_2(0);
WiFiServer* servers[2] = {&server_1, &server_2};
WiFiClient TCPClient[2];

// UART ports
HardwareSerial Serial_1(1);
HardwareSerial Serial_2(2);
HardwareSerial* COM[2] = {&Serial_1, &Serial_2};

// Buffers and counters for intercepting & mirroring data from UART
uint8_t rollBuf[2][BUFFERSIZE];
uint32_t readPos[2] = {0, 0};
uint32_t writePos[2] = {0, 0};

// some metrics to report
uint32_t bytesIn[2] = {0, 0};
uint32_t bytesOut[2] = {0, 0};

// Task management for multi threaded processing
TaskHandle_t port1_h = NULL;
TaskHandle_t port2_h = NULL;
TaskHandle_t connections_h = NULL;
TaskHandle_t status_h = NULL;


// Self explanatory
void readEEPROMConfig(void) {
  EEPROM.begin(EEPROM_SIZE);
  port_1 = EEPROM.read(0);
  port_1 += EEPROM.read(1) << 8;
  port_2 = EEPROM.read(2);
  port_2 += EEPROM.read(3) << 8;
  bps_1 = EEPROM.read(4);
  bps_1 += EEPROM.read(5) << 8;
  bps_2 = EEPROM.read(6);
  bps_2 = EEPROM.read(7) << 8;
  mode_1 = EEPROM.read(8);
  mode_2 = EEPROM.read(9);
  ssidClient = EEPROM.readString(EEPROM_SSID_OFFSET);
  passClient = EEPROM.readString(EEPROM_SSID_OFFSET + ssidClient.length() + 1);
  phoneNumber = EEPROM.readString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + 2);
  apiKey = EEPROM.readString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 3);
}


void tablesInit(void) {
  extern uint8_t numInputs;
  extern uint8_t numZones;
  extern uint8_t numOutputs;
  extern uint8_t * inputsStates;
  extern uint8_t * zonesStates;
  extern uint8_t * outputsStates; 
  extern uint8_t * inputNumbers;
  extern uint8_t * zoneNumbers;
  extern uint8_t * outputNumbers; 
  extern uint32_t * inputFramesCtr;
  extern uint32_t * zoneFramesCtr;
  extern uint32_t * outputFramesCtr;

  File file = LittleFS.open("/inputs.cfg", "r");  
  while (file.available()) {file.readStringUntil('\n'); numInputs++;}
  file.close();
  file = LittleFS.open("/zones.cfg", "r");
  while (file.available()) {file.readStringUntil('\n'); numZones++;}
  file.close();
  file = LittleFS.open("/outputs.cfg", "r"); 
  while (file.available()) { file.readStringUntil('\n'); numOutputs++;}
  file.close();

  inputsStates = (uint8_t*) malloc(numInputs * sizeof(uint8_t));
  zonesStates = (uint8_t*) malloc(numZones * sizeof(uint8_t));
  outputsStates = (uint8_t*) malloc(numOutputs * sizeof(uint8_t));
  for (uint8_t i = 0; i < numInputs; i++) inputsStates[i] = 0;
  for (uint8_t i = 0; i < numZones; i++) zonesStates[i] = 0;
  for (uint8_t i = 0; i < numOutputs; i++) outputsStates[i] = 0;

  inputFramesCtr = (uint32_t*) malloc(sizeof(inputFrameCodes) * sizeof(uint32_t));
  zoneFramesCtr = (uint32_t*) malloc(sizeof(zoneFrameCodes) * sizeof(uint32_t));
  outputFramesCtr = (uint32_t*) malloc(sizeof(outputFrameCodes) * sizeof(uint32_t));
  for (uint8_t i = 0; i < sizeof(inputFrameCodes); i++) inputFramesCtr[i] = 0;
  for (uint8_t i = 0; i < sizeof(zoneFrameCodes); i++) zoneFramesCtr[i] = 0;
  for (uint8_t i = 0; i < sizeof(outputFrameCodes); i++) outputFramesCtr[i] = 0;

  inputNumbers = (uint8_t*) malloc(numInputs * sizeof(uint8_t));
  zoneNumbers = (uint8_t*) malloc(numZones * sizeof(uint8_t));
  outputNumbers = (uint8_t*) malloc(numOutputs * sizeof(uint8_t));
  file = LittleFS.open("/inputs.cfg", "r");  
  uint8_t i = 0;
  while (file.available()) {
    String numberStr = file.readStringUntil(':');
    inputNumbers[i] = atoi(numberStr.c_str());
    i++;
    file.readStringUntil('\n');
  }
  file.close();
  file = LittleFS.open("/zones.cfg", "r");  
  i = 0;
  while (file.available()) {
    String numberStr = file.readStringUntil(':');
    zoneNumbers[i] = atoi(numberStr.c_str());
    i++;
    file.readStringUntil('\n');
  }
  file.close();
  file = LittleFS.open("/outputs.cfg", "r");  
  i = 0;
  while (file.available()) {
    String numberStr = file.readStringUntil(':');
    outputNumbers[i] = atoi(numberStr.c_str());
    i++;
    file.readStringUntil('\n');
  }
  file.close();
}


// Standard template
void wifiInit(void) {
  //WiFi config in AP mode (fallback)
  const char* ssidAp = "TCP_to_UART";
  const char* passAp = NULL;
  IPAddress localIp(192,168,1,1);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  if (ssidClient.length() > 0) {
    uint8_t ssidLen = ssidClient.length() + 1;
    uint8_t passLen = passClient.length() + 1;
    char char_ssidClient[ssidLen];
    char char_passClient[passLen];
    ssidClient.toCharArray(char_ssidClient, ssidLen);
    passClient.toCharArray(char_passClient, passLen);
    WiFi.mode(WIFI_STA);
    WiFi.begin(char_ssidClient, char_passClient);
    for (uint8_t i = 0; i < 20; i++) {
      delay(500);
      if (WiFi.status() == WL_CONNECTED) {
        break;
      }
    }
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    delay(200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidAp, passAp);
    WiFi.softAPConfig(localIp, gateway, subnet);
  }
}


// Again nothing special
void tcpServerInit(void) {
  server_1.begin(port_1);
  server_1.setNoDelay(true);
  server_2.begin(port_2);
  server_2.setNoDelay(true);
}


// Standard to the extreme
void uartInit(void) {
  if (COM[0] != NULL) COM[0]->end();
  if (COM[1] != NULL) COM[1]->end();
  COM[0]->begin(bps_1, mode_1, SERIAL1_RXPIN, SERIAL1_TXPIN);
  COM[1]->begin(bps_2, mode_2, SERIAL2_RXPIN, SERIAL2_TXPIN);
}


// Forward communication from TCP socket to UART port
void TCP2COM(uint8_t port) {
  uint8_t bufOut[BUFFERSIZE];
  if (TCPClient[port]) {
    uint16_t i = 0;
    while (TCPClient[port].available()) {
      bufOut[i] = TCPClient[port].read();  // read char from client
      i++;
      if (i == BUFFERSIZE - 1) break;
    }
    bytesOut[port] += i;
    COM[port]->write(bufOut, i);  // now send to UART(num): 
  }
}


// Forward communication from UART port to TCP socket
// If listen == 1 also intercept and mirror incoming data to rotating buffer for separate processing
void COM2TCP(uint8_t port, uint8_t listen) {
  uint8_t bufIn[BUFFERSIZE];
  if (COM[port]->available()) {
    int readCount = COM[port]->read(&bufIn[0], BUFFERSIZE); 
    if (readCount > 0) {
      if (TCPClient[port]) TCPClient[port].write(bufIn, readCount);
      bytesIn[port] += readCount;
      // copy received data into rolling buffer
      if (listen == 1) {
        for (uint16_t i = 0; i < readCount; i++) {
          rollBuf[port][writePos[port] % BUFFERSIZE] = bufIn[i];
          writePos[port]++;
        }
      }
    }
  }
}


// Handler for incoming connections on TCP sockets
void connectionHandler(void *pvParameters) {
  while(1) {
    for (int num = 0; num < 2 ; num++) {
      if (servers[num]->hasClient()) {
        if (TCPClient[num].connected()) {
          servers[num]->available().stop();
        }
        else {
          TCPClient[num] = servers[num]->available();
        }
      }
    }
    delay(500);
  }
}


// Handler of UART 1
void port1Handler(void *pvParameters) {
  while (1) {
    TCP2COM(0);
    COM2TCP(0, SERIAL1_MIRRORING);
    delay(10);
  }
}


// Handler of UART 2
void port2Handler(void *pvParameters) {
  while(1) {
    TCP2COM(1);
    COM2TCP(1, SERIAL2_MIRRORING);
    delay(10);
  }
}


// Simple linear setup
void setup() {
  readEEPROMConfig();
  wifiInit();
  ArduinoOTA.begin();
  tcpServerInit();
  webServerInit();
  initWebSocket();
  uartInit();
  LittleFS.begin();
  tablesInit();
  xTaskCreate(port1Handler, "Handling Port 1", 8192, NULL, 1, &port1_h);
  xTaskCreate(port2Handler, "Handling Port 2", 8192, NULL, 1, &port2_h);
  xTaskCreate(connectionHandler, "Handling incoming connections", 8192, NULL, 1, &connections_h);
  xTaskCreate(integraRxHandler, "Handling status updates", 8192, NULL, 1, &status_h);
  sendWhatsAppMessage("TCP2UART restarted.\n");
}


// Just handle OTA, nothing else to be done in main loop
void loop() {
  ArduinoOTA.handle();
  delay(10);
}