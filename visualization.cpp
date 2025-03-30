#include "config.h"

// Numbers of the inputs/zones/outputs being monitored
uint8_t * inputNumbers;
uint8_t * zoneNumbers;
uint8_t * outputNumbers; 


// Handle dynamic content in panel.html view:
// And we always generate tables to visualize states of inputs/zones/outputs based on the config files.
String processorIntegra(const String & var) {
  char type = var[0];
  extern uint8_t * inputsStates;
  extern uint8_t * zonesStates;
  extern uint8_t * outputsStates; 
  if (var == "TABLEINPUTS") return generateStatesTable("/integra_inputs.cfg", sizeof(inputFrameCodes), 'i', inputsStates);
  if (var == "TABLEZONES") return generateStatesTable("/integra_zones.cfg", sizeof(zoneFrameCodes), 'z', zonesStates);
  if (var == "TABLEOUTPUTS") return generateStatesTable("/integra_outputs.cfg", sizeof(outputFrameCodes), 'o', outputsStates);
  if (var == "ZONESFORM") {
    String form = "";
    String name = "";
    String numberStr = "";
    uint8_t number = 0;
    File file = LittleFS.open("/integra_zones.cfg", "r");
    if (!file) return "File not found";
    while (file.available()) {
      numberStr = file.readStringUntil(':');
      number = atoi(numberStr.c_str());
      name = file.readStringUntil('\n');
      name.trim();
      form += "\n<option value=\"" + String(number) + "\">" + name + "</option>";
    }
    form += "\n";
    file.close();
    return form;
  }
}


// Fill corresponding placeholders in the settings HTML code with variables
String processorStats(const String & var) {
  extern uint32_t bytesIn[2];
  extern uint32_t bytesOut[2];
  extern uint32_t readPos[2];
  extern uint32_t writePos[2];
  extern uint32_t * inputFramesCtr;
  extern uint32_t * zoneFramesCtr;
  extern uint32_t * outputFramesCtr;
  extern uint8_t numInputs;
  extern uint8_t numZones;
  extern uint8_t numOutputs;
  extern uint8_t * inputNumbers;
  extern uint8_t * zoneNumbers;
  extern uint8_t * outputNumbers; 
  if (var == "PORT1IN") return String(bytesIn[0]);
  else if (var == "PORT1OUT") return String(bytesOut[0]);
  else if (var == "PORT2IN") return String(bytesIn[1]);
  else if (var == "PORT2OUT") return String(bytesOut[1]);
  else if (var == "READPOS1") return String(readPos[0]);
  else if (var == "WRITEPOS1") return String(writePos[0]);
  else if (var == "READPOS2") return String(readPos[1]);
  else if (var == "WRITEPOS2") return String(writePos[1]);
  else if (var == "INPUTFRAMETABLE") return generateFramesTable(sizeof(inputFrameCodes), inputFrameCodes, inputFramesCtr);
  else if (var == "ZONEFRAMETABLE") return generateFramesTable(sizeof(zoneFrameCodes), zoneFrameCodes, zoneFramesCtr);
  else if (var == "OUTPUTFRAMETABLE") return generateFramesTable(sizeof(outputFrameCodes), outputFrameCodes, outputFramesCtr);
  else if (var == "INPUTS") return generateNumbersTable("/integra_inputs.cfg", "Inputs");
  else if (var == "ZONES") return generateNumbersTable("/integra_zones.cfg", "Zones");
  else if (var == "OUTPUTS") return generateNumbersTable("/integra_outputs.cfg", "Outputs");
  else return "n/a";
}


// Generate HTML tables for use with TCP sockets to dynamically visualize state of inputs/zones/outputs
// Config files are stored in data directory. Each file contains content of 1 table. Format of config files:
// number:name, e.g.:
// 3:Zone X
// 12:PIR detector in sleeping room
// Each cell in the tables will contain an id based on input/zone/output number and bit which corresponds with particular state.
// e.g. <td id="z021"> means: Zone 02, bit 1 (armed state). For description of particular bits see updateStates function.
String generateStatesTable(String filename, uint8_t columns, char type, uint8_t * states) {
  String numberStr = "";
  uint8_t number = 0;
  String name = "";
  String output = "";
  File file = LittleFS.open(filename, "r");
  if (!file) return "File not found";
  while (file.available()) {
    // Read corresponding config file line by line and split each line according to expected pattern as described above
    numberStr = file.readStringUntil(':');
    name = file.readStringUntil('\n');
    name.trim();
    output += "\n<tr><th>" + name + "</th>";
    for (uint8_t i = 0; i < columns; i++) {
      char code[5];
      char state;
      // This is the actual generation of cell IDs
      sprintf(code, "%c%.2d%d\0", type, number, i);
      // And here is generation of the according <td id=...> HTML code
      if ((states[number] >> i) & 0x01 != 0) state = '#'; else state = ' ';
      output += "<td id=\"" + String(code) + "\">" + String(state) +"</td>" ;
    }
    output += "</tr>";
    number++;
  }
  output += "\n";
  file.close();
  return output;
}


// Generate HTML to display tables which visualize statistics of received frames.
// We can always change the list of frames we're interested in so they need to be created dynamically.
String generateFramesTable(uint8_t numCells, const uint8_t * codes, uint32_t * counter)
{
  String tableRow = "<tr><th>Code: </th>";
  for (uint8_t i = 0; i < numCells; i++) {
    char code[5];
    sprintf(code, "0x%.2X", codes[i]);
    code[4] = '\0';
    tableRow += "<th>" + String(code) + "</th>";
  }
  tableRow += "</tr>\n<tr><th>Count: </th>";
  for (uint8_t i = 0; i < numCells; i++) tableRow += "<td>" + String(counter[i]) + "</td>";
  tableRow += "</tr>\n";
  return tableRow;
}


// Generate list of inputs/zones/outputs which are stored in the config
// Only for diagnostic purposes
String generateNumbersTable(String filename, String header) {
  String numberStr = "";
  String name = "";
  String output = "";
  output += "<tr><th colspan=\"2\">" + header + "</th>" ;
  File file = LittleFS.open(filename, "r");
  if (!file) return "File not found";
  while (file.available()) {
    // Read corresponding config file line by line and split each line according to expected pattern as described above
    numberStr = file.readStringUntil(':');
    name = file.readStringUntil('\n');
    name.trim();
    output += "\n<tr><th>" + numberStr + "</th><td>" + name + "</td></tr>";
  }
  output += "\n";
  file.close();
  return output;
}



// Dispatch a notification to all connected clients via WebSocket.
// Each notification only updates a single input/zone/object but we always send update of all bits of this object
// Reason is we know which object has changed but at this point we don't know which particular bit
// Perhaps there's room for optimizetion here
// IDs of updated cells are compliant with IDs generated by generateTable function and are processed by JavaScript at the other end
void refreshStates(uint8_t num, uint8_t numFrameCodes, char type, uint8_t state) {
  for (uint8_t i = 0; i < numFrameCodes; i++) {
    char status;
    if (((state >> i ) & 0x01) != 0) status = '#'; else status = ' ';
    char notification[7];
    sprintf(notification, "%c%.2d%d:%c\0", type, num, i, status);
    notifyClients(String(notification));
  }
}


// When a state of a zone changes we send WhatsApp notification
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
// Entry time and exit time change too dynamically, it usually wouldn't make sens in my case
void notifyWhatsApp(uint8_t i, uint8_t state, uint8_t oldstate) {
  String message = "";
  String numberStr = "";
  String name = "";
  uint8_t number = 0;
  uint8_t change = state ^ oldstate;
  File file = LittleFS.open("/integra_zones.cfg", "r");
  while (file.available()) {
    numberStr = file.readStringUntil(':');
    number = atoi(numberStr.c_str());
    name = file.readStringUntil('\n');
    if (i == number) {
      if ((change & 0x01) != 0) {
        message += "Strefa " + name;
        if (state & 0x01 != 0) message += " uzbrojona.\n"; else message += " rozbrojona.\n";
      }
      if (((change & 0x50) || (state & 0x50)) != 0) {
        message += "Alarm w strefie " + name + "\n";
      }
      if (((change & 0xA0) || (state & 0xA0)) != 0) {
        message += "Alarm pozarowy w strefie " + name + "\n";
      }
      sendWhatsAppMessage(message);
      break;
    }
  }
}