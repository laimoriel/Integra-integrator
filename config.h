#include <Arduino.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <UrlEncode.h>

// hardware setup
#define SERIAL1_TXPIN 18
#define SERIAL1_RXPIN 19
#define SERIAL2_TXPIN 21
#define SERIAL2_RXPIN 26
#define EEPROM_SIZE 160
#define BUFFERSIZE 512
#define EEPROM_SSID_OFFSET 20

struct zoneStatusNotification {
  char type;
  uint8_t objNum;
  uint8_t newState;
  uint8_t change;
};

// Define codes of the incoming frames we want to process
const uint8_t inputFrameCodes[] = {0x00, 0x02, 0x04, 0x06, 0x08, 0x0A};
const uint8_t zoneFrameCodes[] = {0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
const uint8_t outputFrameCodes[] = {0x1C};
const TickType_t xDelay = 10 / portTICK_PERIOD_MS;

// Initialization functions
void readEEPROMConfig(void);
void wifiInit(void);
void tcpServerInit(void);
void uartInit(void);
void tablesInit(void);
void initWebSocket(void); 
String readConfigFile(String);
uint16_t countLines(String);
void readObjectNumbersNames(String, uint8_t, uint8_t *, char *);

// Main handlers
void port1Handler(void *);
void port2Handler(void *);
void connectionHandler(void *);
void mgmtHandler(void *);
void connectionHandler(void *);

// Actual transfer between TCP sockets and UART
void TCP2COM(uint8_t);
void COM2TCP(uint8_t, uint8_t);

// General web server functions
void webServerInit(void);
void handleGetSettings(AsyncWebServerRequest *);
void handleGetSendFrame(AsyncWebServerRequest *);
void handleParamSettings(String const &, String const &);
void notFound(AsyncWebServerRequest *);
void onEvent(AsyncWebSocket *, AsyncWebSocketClient *, AwsEventType, void *, uint8_t *, size_t);
void handleWebSocketMessage(void *, uint8_t *, size_t);

// Receive and decode frames from Integra
void integraRxHandler(void *);
void findFrame(uint8_t);
uint8_t extractFrame(uint8_t *, uint8_t, uint8_t *, uint32_t *);
uint8_t validateCRC(uint8_t *, uint8_t);

// Encode and send frames to Integra
void handleParamSendFrame(String const &, String const &);
void handleParamPanel(String const &, String const &);
uint8_t validateChar(char); 
int8_t validateInput(const char *, uint8_t);
void unSeparate(const char *, uint8_t, char *);
void digitalize(const char *, uint8_t, char *);
void calcCRC(char *, uint8_t, char *);
void dispatchFrame(char *, uint8_t);

// Visualization and notifications
String upTime(void);
void updateStates(uint8_t, char, uint8_t *, uint8_t, uint8_t *, uint8_t *);
void refreshStates(uint8_t, uint8_t, uint8_t, uint8_t);
void notifyClients(String);
void notifyWhatsApp(char, uint8_t, uint8_t, uint8_t);
void sendWhatsAppMessage(String);
String processorIntegra(const String &);
String processorSettings(const String &);
String processorStats(const String &);
String generateStatesTable(char, uint8_t, uint8_t, char *, uint8_t *);
String generateFramesTable(uint8_t, const uint8_t *, uint32_t *);
String generateSelectForm(String, uint32_t);
String generateNumbersTable(uint8_t, uint8_t *, char *, String);

