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
uint8_t port_mirroring = 0;
// UART ports paramaeters
uint32_t bps_1;
uint32_t bps_2;
uint8_t mode_1;
uint8_t mode_2;


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
uint8_t bufIn[2][BUFFERSIZE];
uint8_t bufOut[2][BUFFERSIZE];
uint8_t rollBuf[2][BUFFERSIZE];
uint32_t readPos[2] = {0, 0};
uint32_t writePos[2] = {0, 0};
uint16_t bufInDepth[2] = {0, 0};
uint16_t bufOutDepth[2] = {0, 0};

// some metrics to report
uint32_t bytesIn[2] = {0, 0};
uint32_t bytesOut[2] = {0, 0};

// Task management for multi threaded processing
TaskHandle_t port1_h = NULL;
TaskHandle_t port2_h = NULL;
TaskHandle_t connections_h = NULL;
TaskHandle_t status_h = NULL;
TaskHandle_t whatsApp_h = NULL;
QueueHandle_t whatsAppQ;
TaskHandle_t websocket_h = NULL;
QueueHandle_t websocketQ;


// Numbers of the inputs/zones/outputs being monitored
uint8_t * inputsNumbers;
uint8_t * zonesNumbers;
uint8_t * outputsNumbers; 
// Sizes of descriptions of inputs/zones/outputs from config
uint8_t * inputsNameSizes;
uint8_t * zonesNameSizes;
uint8_t * outputsNameSizes; 
// Descriptions of inputs/zones/outputs from config
char * inputsNames;
char * zonesNames;
char * outputsNames;
// Self explanatory
uint8_t numInputs = 0;
uint8_t numZones = 0;
uint8_t numOutputs = 0;
// Some metrics to report the number of frames received in each category
uint32_t * inputFramesCtr;
uint32_t * zoneFramesCtr;
uint32_t * outputFramesCtr;
// Variables which store current state of inputs/zones/outputs being monitored
uint8_t * inputsStates;
uint8_t * zonesStates;
uint8_t * outputsStates; 

uint32_t port1hMaxTime = 0;
uint32_t port2hMaxTime = 0;
uint32_t connectionhMaxTime = 0;
uint32_t integrarxhMaxTime = 0;
uint32_t whatsapphMaxTime = 0;
uint32_t websocketMaxTime = 0;

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
  port_mirroring = EEPROM.read(10);
  ssidClient = EEPROM.readString(EEPROM_SSID_OFFSET);
  passClient = EEPROM.readString(EEPROM_SSID_OFFSET + ssidClient.length() + 1);
  phoneNumber = EEPROM.readString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + 2);
  apiKey = EEPROM.readString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 3);
}

uint16_t countLines(String input) {
  uint32_t strIndex = 0;
  uint16_t numLines = 0;
  if (input.length() > 0) {
    numLines++;
    while (strIndex < input.length()) {
      strIndex = input.indexOf('\n', strIndex + 1);
      if (strIndex == -1) break;
      numLines++;
    }
  return numLines;
  }
  else return 0;
}

uint16_t countFileLines(String filename) {
  uint16_t numLines = 0;
  File file = LittleFS.open(filename, "r");
  while (file.available()) {
    String content = file.readStringUntil('\n');
    numLines++;
  }
  file.close();
  return numLines;
}


void readObjectNumbersNames(String filename, uint8_t numObjects, uint8_t * objNumbers, char * objNames) {
  File file = LittleFS.open(filename, "r");  
  for (uint8_t num = 0; num < numObjects; num++) {
    String objNumberStr = file.readStringUntil(':');
    String objNameStr = file.readStringUntil('\n');
    objNumbers[num] = atoi(objNumberStr.c_str());
    uint8_t nameSize = min(15, (int)objNameStr.length());
    objNameStr.toCharArray(objNames + 16 * num, nameSize);
    objNames[16 * num + nameSize + 1] = '\0';
  }
  file.close();
}


// Some arrays can only be created and initialized AFTER we read the main config:
// - how many inputs, zones and outputs are in the supported system, 
// how many characters we need for description of those objects.
// Afterwards we need to initialize them accordingly.
void tablesInit(void) {
  // Read config files into arrays
  numInputs = countFileLines("/integra_inputs.cfg");
  numZones = countFileLines("/integra_zones.cfg");
  numOutputs = countFileLines("/integra_outputs.cfg");
  inputsNumbers = (uint8_t*) malloc(numInputs * sizeof(uint8_t));
  zonesNumbers = (uint8_t*) malloc(numZones * sizeof(uint8_t));
  outputsNumbers = (uint8_t*) malloc(numOutputs * sizeof(uint8_t));
  inputsNames = (char *) malloc(numInputs * 16 * sizeof(char));
  zonesNames = (char *) malloc(numZones * 16 * sizeof(char));
  outputsNames = (char *) malloc(numOutputs * 16 * sizeof(char));
  readObjectNumbersNames("/integra_inputs.cfg", numInputs, inputsNumbers, inputsNames);
  readObjectNumbersNames("/integra_zones.cfg", numZones, zonesNumbers, zonesNames);
  readObjectNumbersNames("/integra_outputs.cfg", numOutputs, outputsNumbers, outputsNames);

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
  if (TCPClient[port]) {
    uint16_t i = 0;
    while (TCPClient[port].available()) {
      bufOut[port][i] = TCPClient[port].read();  // read char from client
      i++;
      if (i == BUFFERSIZE - 1) break;
    }
    bytesOut[port] += i;
    if (i > (bufOutDepth[port])) bufOutDepth[port] = i;
    COM[port]->write(bufOut[port], i);  // now send to UART(num): 
  }
}


// Forward communication from UART port to TCP socket
// If listen == 1 also intercept and mirror incoming data to rotating buffer for separate processing
void COM2TCP(uint8_t port, uint8_t listen) {
  if (COM[port]->available()) {
    int readCount = COM[port]->read(&bufIn[port][0], BUFFERSIZE); 
    if (readCount > 0) {
      if (TCPClient[port]) TCPClient[port].write(bufIn[port], readCount);
      bytesIn[port] += readCount;
      if (readCount > (bufInDepth[port])) bufInDepth[port] = readCount;
      // copy received data into rolling buffer
      if (listen == 1) {
        for (uint16_t i = 0; i < readCount; i++) {
          rollBuf[port][writePos[port] % BUFFERSIZE] = bufIn[port][i];
          writePos[port]++;
        }
      }
    }
  }
}


// Handler for incoming connections on TCP sockets
void connectionHandler(void *pvParameters) {
  while(1) {
    uint32_t start = millis();
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
    uint32_t time = millis() - start;
    if (time > connectionhMaxTime) connectionhMaxTime = time;
    vTaskDelay(xDelay * 5);
  }
}


// Handler of UART 1
void port1Handler(void *pvParameters) {
  while (1) {
    uint32_t start = millis();
    TCP2COM(0);
    uint8_t listen = port_mirroring & 0x01;
    COM2TCP(0, listen);
    uint32_t time = millis() - start;
    if (time > port1hMaxTime) port1hMaxTime = time;
    vTaskDelay(xDelay);
  }
}


// Handler of UART 2
void port2Handler(void *pvParameters) {
  while(1) {
    uint32_t start = millis();
    TCP2COM(1);
    uint8_t listen = (port_mirroring & 0x02) >> 1;
    COM2TCP(1, listen);
    uint32_t time = millis() - start;
    if (time > port2hMaxTime) port2hMaxTime = time;
    vTaskDelay(xDelay);
  }
}


void whatsAppHandler(void *pvParameters) {
  struct zoneStatusNotification notification;
  while(1) {
    uint32_t start = millis();
    if (xQueueReceive(whatsAppQ, (void *) &notification, 0) == pdPASS) {
      Serial.printf("Received message from queue\n");
      notifyWhatsApp(notification.type, notification.objNum, notification.newState, notification.change);
    }
    uint32_t time = millis() - start;
    if (time > whatsapphMaxTime) whatsapphMaxTime = time;
    vTaskDelay(xDelay);
  }
}


void websocketHandler(void *pvParameters) {
  char notification[7];
  while(1) {
    uint32_t start = millis();
    if (xQueueReceive(websocketQ, (void *) &notification, 0) == pdPASS) {
      notifyClients(String(notification));
    }
    uint32_t time = millis() - start;
    if (time > websocketMaxTime) websocketMaxTime = time;
    vTaskDelay(xDelay);
  }
}


// Simple linear setup
void setup() {
  Serial.begin(115200);
  readEEPROMConfig();
  wifiInit();
  ArduinoOTA.begin();
  tcpServerInit();
  webServerInit();
  initWebSocket();
  uartInit();
  LittleFS.begin();
  tablesInit();
  xTaskCreate(port1Handler, "Handling Port 1", 4096, NULL, 1, &port1_h);
  xTaskCreate(port2Handler, "Handling Port 2", 4096, NULL, 1, &port2_h);
  xTaskCreate(connectionHandler, "Handling incoming connections", 4096, NULL, 1, &connections_h);
  xTaskCreate(integraRxHandler, "Handling status updates", 8192, NULL, 1, &status_h);
  whatsAppQ = xQueueCreate(10, sizeof(zoneStatusNotification));
  xTaskCreate(whatsAppHandler, "Handling WhatsApp notifications", 8192, NULL, 1, &whatsApp_h);
  websocketQ = xQueueCreate(10, 7 * sizeof(char));
  xTaskCreate(websocketHandler, "Handling communication over websocket", 8192, NULL, 1, &websocket_h);
  sendWhatsAppMessage("TCP2UART restarted.\n");
}


// Just handle OTA, nothing else to be done in main loop
void loop() {
  ArduinoOTA.handle();
  delay(10);
}