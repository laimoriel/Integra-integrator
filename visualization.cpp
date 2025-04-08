#include "config.h"

// Number of inputs/zones/outputs from config files
extern uint8_t numInputs;
extern uint8_t numZones;
extern uint8_t numOutputs;
// Numbers of the inputs/zones/outputs being monitored
extern uint8_t * inputsNumbers;
extern uint8_t * zonesNumbers;
extern uint8_t * outputsNumbers; 
// Descriptions of inputs/zones/outputs from config
extern char * inputsNames;
extern char * zonesNames;
extern char * outputsNames;

// Variables which store current state of inputs/zones/outputs being monitored
extern uint8_t * inputsStates;
extern uint8_t * zonesStates;
extern uint8_t * outputsStates; 
// Some metrics to report the number of frames received in each category
extern uint32_t * inputFramesCtr;
extern uint32_t * zoneFramesCtr;
extern uint32_t * outputFramesCtr;

extern uint16_t bufInDepth[2];
extern uint16_t bufOutDepth[2];
extern uint32_t bytesIn[2];
extern uint32_t bytesOut[2];
extern uint32_t readPos[2];
extern uint32_t writePos[2];

extern uint32_t port1hMaxTime;
extern uint32_t port2hMaxTime;
extern uint32_t connectionhMaxTime;
extern uint32_t integrarxhMaxTime;
extern uint32_t whatsapphMaxTime;
extern uint32_t websocketMaxTime;



// Handle dynamic content in panel.html view:
// And we always generate tables to visualize states of inputs/zones/outputs based on the config files.
String processorIntegra(const String & var) {
  char type = var[0];
  if (var == "TABLEINPUTS") return generateStatesTable('i', numInputs, sizeof(inputFrameCodes), inputsNames, inputsStates);
  else if (var == "TABLEZONES") return generateStatesTable('z', numZones, sizeof(zoneFrameCodes), zonesNames, zonesStates);
  else if (var == "TABLEOUTPUTS") return generateStatesTable('o', numOutputs, sizeof(outputFrameCodes), outputsNames, outputsStates);
  else if (var == "UPTIME") return upTime();
  else if (var == "ZONESFORM") {
    uint16_t arraySize = 50 * numZones;
    char * output = (char *) malloc(arraySize * sizeof(char));
    uint16_t index = 0;
    for (uint8_t zone = 0; zone < numZones; zone++) {
      strcpy(output + index, "\n<option value=\"");
      index += 16;
      sprintf(output + index, "%.2d", zonesNumbers[zone]);
      index += 2;
      strcpy(output + index, "\">");
      index += 2;
      strcpy(output + index, zonesNames + 16 * zone);
      index += strlen(zonesNames + 16 * zone);
      strcpy(output + index, "</option>");
      index += 9;
    }
    String outputStr = String(output);
    free(output);
    return outputStr;
  }
}


// Fill corresponding placeholders in the settings HTML code with variables
String processorStats(const String & var) {

  if (var == "PORT1IN") return String(bytesIn[0]);
  else if (var == "PORT1OUT") return String(bytesOut[0]);
  else if (var == "PORT2IN") return String(bytesIn[1]);
  else if (var == "PORT2OUT") return String(bytesOut[1]);
  else if (var == "PORT1INDEPTH") return String(bufInDepth[0]);
  else if (var == "PORT1OUTDEPTH") return String(bufOutDepth[0]);
  else if (var == "PORT2INDEPTH") return String(bufInDepth[1]);
  else if (var == "PORT2OUTDEPTH") return String(bufOutDepth[1]);
  else if (var == "PORT1HMAXTIME") return String(port1hMaxTime);
  else if (var == "PORT2HMAXTIME") return String(port2hMaxTime);
  else if (var == "CONNECTIONHMAXTIME") return String(connectionhMaxTime);
  else if (var == "WHATSAPPHMAXTIME") return String(whatsapphMaxTime);
  else if (var == "INTEGRARXHMAXTIME") return String(integrarxhMaxTime);
  else if (var == "WEBSOCKETMAXTIME") return String(websocketMaxTime);
  else if (var == "INPUTFRAMETABLE") return generateFramesTable(sizeof(inputFrameCodes), inputFrameCodes, inputFramesCtr);
  else if (var == "ZONEFRAMETABLE") return generateFramesTable(sizeof(zoneFrameCodes), zoneFrameCodes, zoneFramesCtr);
  else if (var == "OUTPUTFRAMETABLE") return generateFramesTable(sizeof(outputFrameCodes), outputFrameCodes, outputFramesCtr);
  else if (var == "INPUTS") return generateNumbersTable(numInputs, inputsNumbers, inputsNames, "Inputs");
  else if (var == "ZONES") return generateNumbersTable(numZones, zonesNumbers, zonesNames, "Zones");
  else if (var == "OUTPUTS") return generateNumbersTable(numOutputs, outputsNumbers, outputsNames, "Outputs");
  else if (var == "UPTIME") return upTime();
  else if (var == "HEAP") return String(ESP.getFreeHeap());
  else if (var == "HEAPLOW") return String(ESP.getMinFreeHeap());
  else if (var == "HEAPSIZE") return String(ESP.getHeapSize());
  else if (var == "HEAPALLOC") return String(ESP.getMaxAllocHeap());
  else return "n/a";
}


// Generate HTML tables for use with TCP sockets to dynamically visualize state of inputs/zones/outputs
// Config files are stored in data directory. Each file contains content of 1 table. Format of config files:
// number:name, e.g.:
// 3:Zone X
// 12:PIR detector in sleeping room
// Each cell in the tables will contain an id based on input/zone/output number and bit which corresponds with particular state.
// e.g. <td id="z021"> means: Zone 02, bit 1 (armed state). For description of particular bits see updateStates function.
String generateStatesTable(char type, uint8_t rows, uint8_t columns, char * names, uint8_t * states) {
  uint16_t arraySize = ((36 * columns) + 34) * rows;
  char * output = (char *) malloc(arraySize * sizeof(char));
  uint16_t index = 0;
  for (uint8_t rowNum = 0; rowNum < rows; rowNum++) {
    strcpy(output + index, "\n<tr><th>");
    index += 9;
    uint8_t nameSize = strlen(names + 16 * rowNum);
    strncpy(output + index, names + 16 * rowNum, nameSize);
    index += nameSize;
    for (uint8_t colNum = 0; colNum < columns; colNum++) {
      strcpy(output + index, "<td class=\"innertd\" id=\"");
      index += 24;
      sprintf(output + index, "%c%.2d%d", type, rowNum, colNum);
      index += 4;
      strcpy(output + index, "\">");
      index += 2;
      char state;
      if ((states[rowNum] >> colNum) & 0x01 != 0) state = '#'; else state = ' ';
      output[index] = state;
      index += 1;
      strcpy(output + index, "</td>");
      index += 5;
    }
    strcpy(output + index, "</tr>");
    index += 5;
  }
  output[index] = '\0';
  String outputStr = String(output);
  free(output);
  return outputStr;
}


// Generate HTML to display tables which visualize statistics of received frames.
// We can always change the list of frames we're interested in so they need to be created dynamically.
String generateFramesTable(uint8_t numCodes, const uint8_t * codes, uint32_t * counter)
{
  uint16_t arraySize = 41 + (48 + 10) * numCodes;
  char * output = (char *) malloc(arraySize * sizeof(char));
  uint16_t index = 0;
  strcpy(output + index, "\n<tr><th>Code: </th><th>Count: </th></tr>");
  index += 41;
  for (uint8_t numCode = 0; numCode < numCodes; numCode++) {
    strcpy(output + index, "\n<tr><th>");
    index += 9;
    sprintf(output + index, "0x%.2X", codes[numCode]);
    index += 4;
    strcpy(output + index, "</th><td class=\"frametd\">");
    index += 25;
    char counterStr[11];
    sprintf(counterStr, "%d\0", counter[numCode]);
    sprintf(output + index, "%d", counter[numCode]);
    index += strlen(counterStr);
    strcpy(output + index, "</td></tr>");
    index += 10;
  }
  output[index] = '\0';
  String outputStr = String(output);
  free(output);
  return outputStr;
}


// Generate list of inputs/zones/outputs which are stored in the config
String generateNumbersTable(uint8_t rows, uint8_t * numbers, char * names, String header) {
  uint16_t arraySize = 50 + rows * 72;
  char * output = (char *) malloc(arraySize * sizeof(char));
  uint16_t index = 0;
  strcpy(output + index, "\n<tr><th colspan=\"2\">");
  index += 21;
  strcpy(output + index, header.c_str());
  index += header.length();
  strcpy(output + index, "</th></tr>");
  index += 9;
  for (uint8_t numObj = 0; numObj < rows; numObj++) {
    strcpy(output + index, "\n<tr><th>");
    index += 9;
    sprintf(output + index, "%.2d", numbers[numObj]);
    index += 2;
    strcpy(output + index, "</td><td class=\"frametd\">");
    index += 25;
    strcpy(output + index, names + 16 * numObj);
    index += strlen(names + 16 * numObj);
    strcpy(output + index, "</td></tr>");
    index += 10;
  }
  output[index] = '\0';
  String outputStr = String(output);
  free(output);
  return outputStr;
}


// When a state of a zone changes we send WhatsApp notification
// State of inputs, bits:
// 0 0x01 - input violation (frame code 0x00)
// 1 0x02 - input tamper (frame code 0x02)
// 2 0x04 - input alarm (frame code 0x04)
// 3 0x08 - input tamper alarm (frame code 0x06)
// 4 0x10 - input alarm memory (frame code 0x08)
// 5 0x20 - input tamper alarm memory (frame code 0x0A)
// State of zones, bits:
// 0 0x01 - mask 0xFE, armed (frame 0x12)
// 1 0x02 - mask 0xFD, entry time (frame 0x13)
// 2 0x04 - mask 0xFB, exit time > 10s (frame 0x14)
// 3 0x08 - mask 0xF7, exit time < 10s (frame 0x15)
// 4 0x10 - mask 0xEF, alarm (frame 0x16)
// 5 0x20 - mask 0xDF, fire alarm (frame 0x17)
// 6 0x40 - mask 0xBF, alarm memory (frame 0x18)
// 7 0z80 - mask 0x7F, fire alarm memory (frame 0x19)
// This is highly customized - at the moment we only notify of zone arming/disarming and alarms
// Entry time and exit time change too dynamically, it usually wouldn't make sense in my case
void notifyWhatsApp(char type, uint8_t objChanged, uint8_t newState, uint8_t change) {
  String output = "";
  if (type == 'z') {
    if ((change & 0x01) != 0) {
      output = "Strefa " + String(zonesNames + objChanged * 16);
      if (newState & 0x01 != 0) {
        output += " uzbrojona";
      }
      else {
        output += " rozbrojona";
      } 
    }
  }
  else if (type =='i') {
    if ((newState & 0x14) != 0) output = "Alarm " + String(inputsNames + objChanged * 16);
    if ((newState & 0x28) != 0) output = "Sabotaz " + String(inputsNames + objChanged * 16);
  }
  sendWhatsAppMessage(output);
}


String upTime(void) {
  uint64_t microSeconds = esp_timer_get_time();
  uint32_t timeSeconds = microSeconds / 1000000;
  uint32_t timeMinutes = timeSeconds / 60;
  uint32_t timeHours = timeMinutes / 60;
  uint16_t timeDays = timeHours / 24;
  char uptime[18];
  sprintf(uptime, "%i days %.2i:%.2i:%.2i\0", timeDays, timeHours % 24, timeMinutes % 60, timeSeconds % 60);
  return String(uptime);
}