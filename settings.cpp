#include "config.h"

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
  extern uint8_t port_mirroring;
  if (name == "port_1") {
    port_1 = value.toInt(); 
    EEPROM.write(0, port_1);
    EEPROM.write(1, port_1 >> 8);
    EEPROM.commit();
  }
  else if (name == "port_2") {
    port_2 = value.toInt(); 
    EEPROM.write(2, port_2);
    EEPROM.write(3, port_2 >> 8);
    EEPROM.commit();
  }
  else if (name == "bps_1") {
    bps_1 = value.toInt(); 
    EEPROM.write(4, bps_1);
    EEPROM.write(5, bps_1 >> 8);
    EEPROM.commit();
    uartInit();
  }
  else if (name == "bps_2") {
    bps_2 = value.toInt(); 
    EEPROM.write(6, bps_2);
    EEPROM.write(7, bps_2 >> 8);
    EEPROM.commit();
    uartInit();
  }
  else if (name == "mode_1") {
    mode_1 = value.toInt();
    EEPROM.write(8, mode_1);
    EEPROM.commit();
    uartInit();
  }
  else if (name == "mode_2") {
    mode_2 = value.toInt();
    EEPROM.write(9, mode_2);
    EEPROM.commit();
    uartInit();
  }
  else if (name == "mirror_1") {
    if (value == "1") port_mirroring |= 0x01;
    else port_mirroring &= 0xFE;
    EEPROM.write(10, port_mirroring);
    EEPROM.commit();
  }
  else if (name == "mirror_2") {
    if (value == "1") port_mirroring |= 0x02;
    else port_mirroring &= 0xFD;
    EEPROM.write(10, port_mirroring);
    EEPROM.commit();
  }
  else if (name == "ssid") {
    ssidClient = value;
    EEPROM.writeString(EEPROM_SSID_OFFSET, ssidClient);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + 1, passClient);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + 1, phoneNumber);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 1, apiKey);
    EEPROM.commit();
  }
  else if (name == "pass") {
    passClient = value;
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + 1, passClient);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + 2, phoneNumber);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 3, apiKey);
    EEPROM.commit();
  }
  else if (name == "phone") {
    phoneNumber = value;
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + 2, phoneNumber);
    EEPROM.writeString(EEPROM_SSID_OFFSET + ssidClient.length() + passClient.length() + phoneNumber.length() + 3, apiKey);
    EEPROM.commit();
  }
  else if (name == "apikey") {
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
  extern uint8_t port_mirroring;
  if (var == "SSID") return ssidClient;
  else if (var == "PORT1") return String(port_1);
  else if (var == "PORT2") return String(port_2);
  else if (var == "BPSTABLE1") return generateSelectForm("/uart_baudrates.cfg", bps_1);
  else if (var == "BPSTABLE2") return generateSelectForm("/uart_baudrates.cfg", bps_2);
  else if (var == "MODETABLE1") return generateSelectForm("/uart_modes.cfg", mode_1);
  else if (var == "MODETABLE2") return generateSelectForm("/uart_modes.cfg", mode_2);
  else if (var == "MIRROR1") return (port_mirroring & 0x01) ? "checked" : "";
  else if (var == "MIRROR2") return (port_mirroring & 0x02) ? "checked" : "";
  else if (var == "PHONE") return String(phoneNumber);
  else if (var == "UPTIME") return upTime();
  else return "n/a";
}


String generateSelectForm(String filename, uint32_t value) {
  String str1 = "";
  String str2 = "";
  uint32_t number = 0;
  String output = "";
  File file = LittleFS.open(filename, "r");
  if (!file) return "File not found";
  while (file.available()) {
    // Read corresponding config file line by line and split each line according to expected pattern as described above
    str1 = file.readStringUntil(':');
    number = str1.toInt();
    str2 = file.readStringUntil('\n');
    str2.trim();
    String selected = "";
    if (number == value) selected = "selected";
    output += "\n<option value=\"" + str1 + "\"" + selected + ">" + str2 + "</option>";
  }
  output += "\n";
  file.close();
  return output;
}