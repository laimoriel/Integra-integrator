#include "config.h"

const char *hexChars = "0123456789ABCDEF";
const char *separatorChars = "$ -";

String notificationStr;
uint8_t payload[13];
uint8_t payloadSize = 0;  
uint8_t payloadReady = 0;


void handleParamSendFrame(String const &name, String const &value) { 
  if (name == "message") { 
    // validate if input string is correct
    const char *input = value.c_str();
    uint8_t inputLength = value.length();
    int8_t hexDigits = validateInput(input, inputLength);
    // if input string is not compliant display error notification and interrupt further processing
    if (hexDigits == -1) {notificationStr = "Not a valid hex!"; return;}
    else if (hexDigits == -2) {notificationStr = "Odd count of digits!"; return;}
    // eliminate separators from input string
    char hexString[hexDigits];
    unSeparate(input, inputLength, hexString);
    // convert input string to array of 8-bit numbers
    payloadSize = hexDigits / 2;
    digitalize(hexString, hexDigits, payload);
    dispatchFrame();
  }
}


void handleParamPanel(String const &name, String const &value) {
  if (name == "mode") {
    uint8_t mode = value.toInt();
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
    payloadReady++;
  }
  if (name == "password") {
    uint8_t length = value.length();
    for (uint8_t i = 0; i < 8; i++) {
      uint8_t c16 = (2 * i < length) ? (value[2 * i] - '0') : (0xA);
      uint8_t c1 = (2 * i + 1 < length) ? (value[2 * i + 1] - '0') : (0xA);
      payload[i + 1] = 16 * c16 + c1;
    }
    payloadReady++;
  }
  if (name == "zone") {
    uint32_t zone;
    zone = 1 << (value.toInt());
    payload[9] = zone & 0XFF;
    payload[10] = (zone >> 8) & 0xFF;
    payload[11] = (zone >> 16) & 0xFF;
    payload[12] = zone >> 24;
    payloadReady++;
  }
  if (payloadReady == 3) {
    dispatchFrame();
    payloadReady = 0;
  }
}


// check if the given character is on list of allowed characters
// return 0 = hex char
// return 1 = separator
// return 2 = none
uint8_t validateChar(char input) {
  for (uint8_t i = 0; i < strlen(hexChars); i++) {
    if (toupper(input) == hexChars[i]) {
      return 0;
    }
  }
  for (uint8_t i = 0; i < strlen(separatorChars); i++) {
    if (input == separatorChars[i]) {
      return 1;
    }
  }
  return 2;
}


// calculate CRCs according to the procedure defined in Satel Integra integration protocol
void calcCRC(uint8_t *payload, uint8_t payloadSize, uint8_t *crc) {
  crc[0] = 0;
  for (uint8_t i = 0; i < payloadSize; i++) {
    crc[0] = crc[0] ^ payload[i];
  }
  uint16_t crc12 = 0x13B7;
  for (uint8_t i = 0; i < payloadSize + 1; i++) {
    crc12 += payload[i];
    crc12 = (crc12 << 1) | (crc12 >> 15);
  }
  crc[1] = crc12 >> 8;
  crc[2] = (crc12 & 0x00FF);
  crc[3] = 0x3D;
  for (uint8_t i = 0; i < payloadSize + 3; i++) {
    crc[3] += payload[i];
    crc[3] = ((crc[3] & 0xF0) >> 4) | ((crc[3] & 0x0F) << 4);
  }
  crc[4] = 0x4D;
  for (uint8_t i = 0; i < payloadSize + 4; i++) {
    crc[4] += payload[i];
  }
}


// validate input string and count actual hex digits in it
// if the string:
// - contains any character other than hex digits and separators return -1
// - contains correct characters but number of hex digits is odd, return -2
// - contains correct characters and has an even number of hex digits, return actual number digits
int8_t validateInput(const char * input, uint8_t length) {
  uint8_t hexDigits = 0;
  for (uint8_t i = 0; i < length; i++) {
    if (validateChar(input[i]) == 0) {
      hexDigits ++;
    }
    else if (validateChar(input[i]) == 2) {      
      return -1;
    }
  }
  if (hexDigits % 2 != 0) { 
    return -2;
  }
  return hexDigits;
}


// remove separator characters from input string
void unSeparate(const char *input, uint8_t length, char *output) {
  uint8_t digitCtr = 0;
  for (uint8_t i = 0; i < length; i++) {
    if (validateChar(input[i]) == 0) {
      output[digitCtr] = input[i];
      digitCtr++;
    }
  }
}


// convert array of ASCII hex digits into array of int numbers
void digitalize(const char *input, uint8_t length, uint8_t *output) {
  for (uint8_t i = 0; i < length; i++) {
    char temp[3];
    temp[0] = input[2 * i];
    temp[1] = input[2 * i + 1];
    temp[2] = '\n';
    output[i] = strtol(temp, NULL, 16);
  }
}


// build frame to send, dispatch it and provide notification
void dispatchFrame(void) {
  extern TaskHandle_t port1_h;
  extern HardwareSerial* COM[2];
  extern WiFiClient TCPClient[2];
  uint8_t frameSize = payloadSize + 9;
  uint8_t frame[frameSize];
  frame[0] = 0xFF;
  frame[1] = 0xFF;
  for (uint8_t i = 0; i < payloadSize; i++) {
    frame[i + 2] = payload[i];
  }
  calcCRC(&frame[2], payloadSize, &frame[payloadSize + 2]);
  frame[payloadSize + 7] = 0xFF;
  frame[payloadSize + 8] = 0x99;
  COM[0]->write(frame, frameSize); 
}