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
#define BUFFERSIZE 1024
#define SERIAL1_MIRRORING 1
#define SERIAL2_MIRRORING 0
#define EEPROM_SSID_OFFSET 20


// Define codes of the incoming frames we want to process
const uint8_t inputFrameCodes[] = {0x00, 0x02, 0x04, 0x06, 0x08, 0x0A};
const uint8_t zoneFrameCodes[] = {0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
const uint8_t outputFrameCodes[] = {0x1C};


// Initialization functions
void readEEPROMConfig(void);
void wifiInit(void);
void tcpServerInit(void);
void uartInit(void);
void tablesInit(void);
void initWebSocket(void); 

// Main handlers
void port1Handler(void *);
void port2Handler(void *);
void connectionHandler(void *);
void mgmtHandler(void *);

// Actual transfer between TCP sockets and UART
void TCP2COM(uint8_t);
void COM2TCP(uint8_t, uint8_t);

// General web server functions
void webServerInit(void);
void handleGetSettings(AsyncWebServerRequest *);
void handleGetSendFrame(AsyncWebServerRequest *);
void handleGetPanel(AsyncWebServerRequest *);
void handleParamSettings(String const &, String const &);
void notFound(AsyncWebServerRequest *);

// Receive and decode frames from Integra
void integraRxHandler(void *);
void findFrame(uint8_t);
uint32_t extractFrame(uint8_t *, uint8_t, uint8_t *, uint32_t);
uint8_t validateCRC(uint8_t *, uint8_t);

// Encode and send frames to Integra
void handleParamSendFrame(String const &, String const &);
void handleParamPanel(String const &, String const &);
uint8_t validateChar(char); 
int8_t validateInput(const char *, uint8_t);
void unSeparate(const char *, uint8_t, char *);
void digitalize(const char *, uint8_t, uint8_t *);
void calcCRC(uint8_t *, uint8_t, uint8_t *);
void dispatchFrame(void);

// Visualization and notifications
void updateStates(uint8_t, char, uint8_t *, uint8_t, uint8_t *, uint8_t *, uint8_t, const uint8_t *, uint32_t *);
void refreshStates(uint8_t, uint8_t, char, uint8_t);
void notifyClients(String);
void notifyWhatsApp(uint8_t, uint8_t, uint8_t);
void sendWhatsAppMessage(String);
String processorIntegra(const String &);
String processorSettings(const String &);
String processorStats(const String &);
String generateStatesTable(String, uint8_t, char, uint8_t *);
String generateFramesTable(uint8_t, const uint8_t *, uint32_t *);
String generateNumbersList(uint8_t *, uint8_t);
String mode2code(uint16_t);

