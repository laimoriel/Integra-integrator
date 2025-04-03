#include "config.h"

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
const uint8_t * frameCodes[3] = {inputFrameCodes, zoneFrameCodes, outputFrameCodes};


// Handler looking for incoming frames in the rotating buffer intercepted from selected UART port
void integraRxHandler(void *pvParameters) {
  extern uint32_t readPos[2];
  extern uint32_t writePos[2];
  while(1) {
    for (uint8_t i = 0; i < 2; i++) {
      if (writePos[i] >= (readPos[i] + 256)) {
        findFrame(i);
      }
    }
    delay(10);
  }
}


// Scan rotating buffer for incoming frames.
// Marker of frame always starts with FF FF xx where xx is frame code and must be != FF.
// When we encounter this sequence we extract the frame. Frame ends with another FF FF sequence (which usually starts next frame).
// After extracting we check if frame code is on one of lists of supported codes.
// Only frames which are found on any of the lists are processed.
// We recognize frame type (inputs/zones/outputs) based on which list frame code was found.
// After extraction we receive frame structure:
// 1 byte = frame size
// 1 byte = payload code
// x bytes (usually 4) = payload
// 1 byte = frame number + KLCD sender buffer state (empty/not empty)
// 4 bytes = crcs      
void findFrame(uint8_t port) {
  extern uint8_t rollBuf[2][BUFFERSIZE];
  extern uint32_t readPos[2];
  extern uint32_t writePos[2];
  extern uint8_t * inputNumbers;
  extern uint8_t * zoneNumbers;
  extern uint8_t * outputNumbers; 
  uint8_t rcvFrame[24];
  // writePos must always precede readPos as we only want to look for data AFTER we received it ;)
  while (writePos[port] > readPos[port] + 224) {
    // We recognize beginning of a frame by sequence of bytes FF FF xx - where xx != FF and is code of the frame
    if ((rollBuf[port][readPos[port] % BUFFERSIZE] == 0xFF) && (rollBuf[port][(readPos[port] + 1) % BUFFERSIZE] == 0xFF) && (rollBuf[port][(readPos[port] + 2) % BUFFERSIZE] != 0xFF)) {
      // Skip the initial FF FF bytes
      readPos[port] += 2;
      // And now proceed with extracting the whole frame, but do not exceed 24 bytes (valid frames we're interested in will be shorter than that)
      readPos[port] = extractFrame(rcvFrame, 24, rollBuf[port], readPos[port]);
      // Validate CRCs of the extracted frames, if it's not correct just ignore the frame
      if (validateCRC(&rcvFrame[1], rcvFrame[0]) == 0) {
        // Now we want to recognize if the frame contains data about inputs, zones or outputs
        // We do it by matching frame code to corresponding lists of known & interesting frame codes of each particular type
        for (uint8_t k = 0; k < sizeof(inputFrameCodes); k++) {
          if (rcvFrame[1] == inputFrameCodes[k]) updateStates(k, 0, &rcvFrame[2], numInputs, inputsStates, inputNumbers, inputFramesCtr);  
        }
        for (uint8_t k = 0; k < sizeof(zoneFrameCodes); k++) {
          if (rcvFrame[1] == zoneFrameCodes[k]) updateStates(k, 1, &rcvFrame[2], numZones, zonesStates, zoneNumbers, zoneFramesCtr);
        }
        for (uint8_t k = 0; k < sizeof(outputFrameCodes); k++) {
          if ((rcvFrame[1] == outputFrameCodes[k]) && (rcvFrame[2] == 0)) updateStates(k, 2, &rcvFrame[3], numOutputs, outputsStates, outputNumbers, outputFramesCtr); 
        }
      }
    }
    // If there was no frame here let's continue starting from next byte
    else {
      readPos[port]++;
    }
  }
}


// After receiving a relevant frame we need to update variables which hold states of inputs/zones/outputs
// State of each input/zone/output is contained in a single byte, particular bits have a distinct meaning:
// State of inputs, bits:
// 0 0x01 - input violation (frame code 0x00)
// 1 0x02 - input tamper (frame code 0x02)
// 2 0x04 - input alarm (frame code 0x04)
// 3 0x08 - input tamper alarm (frame code 0x06)
// 4 0x10 - input alarm memory (frame code 0x08)
// 5 0x20 - input tamper alarm memory (frame code 0x0A)
// State of zones, bits:
// 0 0x01 - armed (frame code 0x12)
// 1 0x02 - entry time (frame code 0x13)
// 2 0x04 - exit time > 10s (frame code 0x14)
// 3 0x08 - exit time < 10s (frame code 0x15)
// 4 0x10 - alarm (frame code 0x16)
// 5 0x20 - fire alarm (frame code 0x17)
// 6 0x40 - alarm memory (frame code 0x18)
// 7 0z80 - fire alarm memory (frame code 0x19)
// State of outputs, bits:
// 0 0x01 - on/off (frame code 0x1C)
// That's the function with most parameters in the code and I'm particularly proud of it as it's as universal as only possible
void updateStates(uint8_t codeNum, char type, uint8_t * frame, uint8_t numObjects, uint8_t * objStates, uint8_t * objNumbers, uint32_t * frameCtrs) {
  uint32_t framePayload = frame[0] + (frame[1] << 8) + (frame[2] << 16) + (frame[3] << 24);
  // Iterate through all inputs/zones/outputs
  for (uint8_t objNum = 0; objNum < numObjects; objNum++) {
    // Now compare updated state of the input/zone/output with previous state to see if the state changed
    // If it changed we dispatch according update to all connected http clients via websocket
    // If it's a zone additionally send a notification over WhatsApp
    uint8_t oldState = objStates[objNum];
    // Apply a bitmask to zero only the bit codeNum
    uint8_t newState = objStates[objNum] & (~(0x01 << codeNum));
    newState |= (((framePayload >> objNumbers[objNum]) & 0x01) << codeNum);
    if (newState != oldState) {
      objStates[objNum] = newState;
      refreshStates(objNum, codeNum, type, newState);
      if (type == 1) notifyWhatsApp(objNum, newState, oldState);
    }
  }
  frameCtrs[codeNum]++;
}


// Extract a frame from buf starting from pos but do not exceed maxFrameSize
// Simply read & copy bytes until we encounter another FF - marker of frame end (usually also start of next).
// The first byte of returned frame will contain number of bytes in the frame
// It's easier to later pass it around this way than using a separate variable
uint32_t extractFrame(uint8_t * frame, uint8_t maxFrameSize, uint8_t * buf, uint32_t pos) {
  uint8_t rcvBytes = 0;
  while ((buf[pos % BUFFERSIZE] != 0xFF) && (rcvBytes < maxFrameSize)) {
    frame[rcvBytes + 1] = buf[pos % BUFFERSIZE];
    rcvBytes++;
    pos++;
  }
  frame[0] = rcvBytes;
  return pos;
}


// Validate CRC of received frames according to Satel integration protocol
// Received frames will be discarded if CRC validation fails
// If you want details on how to calculate the CRCs just read the documentation from Satel... 
uint8_t validateCRC(uint8_t * frame, uint8_t length) {
    uint8_t crc[4];
    uint16_t crc12 = 0x13B7;
    for (uint8_t i = 0; i < length - 4; i++) {
      crc12 += frame[i];
      crc12 = (crc12 << 1) | (crc12 >> 15);
    }
    crc[0] = crc12 >> 8;
    crc[1] = (crc12 & 0x00FF);
    if ((crc[0] != frame[length - 4]) || (crc[1] != frame[length - 3])) return 1;
    crc[2] = 0x3D;
    for (uint8_t i = 0; i < length - 2; i++) {
      crc[2] += frame[i];
      crc[2] = ((crc[2] & 0xF0) >> 4) | ((crc[2] & 0x0F) << 4);
    }
    if (crc[2] != frame[length - 2]) return 2;
    crc[3] = 0x4D;
    for (uint8_t i = 0; i < length - 1; i++) {
      crc[3] += frame[i];
    }
    if (crc[3] != frame[length - 1]) return 3;
    return 0;
}