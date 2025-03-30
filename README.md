The project was created with 2 main functionality scopes:
1. To serve as a transparent TCP <-> UART interface. Combined with virtual COM port installed on PC it allows to wirelessly connect to remote devices using serial port. It's like a wireless serial converter.
   * A simple webpage is provided to setup WiFi connectivity and configure UART port parameters,
   * Supported baud rates from 1200 to 115200, all common RS232 modes,
   * Device can work in both in AP and Station modes,   
2. To provide integration with a popular Satel Integra alarm system. List of currently supported features include:
   * Device can be connected to the RS232 port located in Integra manipulator and use the official simplified integration protocol from Satel to:
      - Remotely arm, disarm, clear alarm in selected zones via webpage,
      - Provide live view of configured inputs, zones and outputs via webpage,
      - Device can also send notification over WhatsApp whenever zone status changes (armed, disarmed, alarm triggered etc.).
      - It provides statistical overview of data transferred via the TCP <-> UART bridge and data received from Integra.
   * It also allows to connect to Integra central unit without cable to use DLOADX remotely.
   * Device was tested with Integra 32 central unit and INT-KLCD manipulator, but it should also work with other Integra series central units.
     
Hardware setup is simple and cheap. Requires only ESP32, some level converter from TTL to RS232 (preferably MAX232) and a 5V power source.
ESP32 can handle up to 3 UARTS and a single MAX232 provides 2 bidirectional channels so it can handle 2 channels without additional hardware.
So we can connect to RS232 on central unit (for DLOADX) and RS232 on KLCD (for integration) at the same time and use it simultaneously.

Setup of is simple. 
Device will initially start in AP mode. You need to connect to AP, open http://192.168.1.1, enter settings and configure your WLAN.
After next restart device will connect to WLAN in Client mode. You need to identify its IP, go to its webpage and continue configuration.
WiFi and WhatsApp credentials, TCP sockets and UART parameters are configured via webpage.
Configuration of inputs, zones and outputs in the alarm system is done via .cfg files located in ./data directory.
You don't need to configure all inputs/zones/outputs you have in your system, just the ones you want to monitor via the device.

THE DEVICE NEVER STORES ANY PASSWORDS OF THE ALARM SYSTEM!
Passwords to the alarm system are entered via webpage and after clicking "Submit" dispatched to Integra as part of data frame.
They are not stored anywhere in the device. Only WLAN and WhatsApp credentials are stored in the EEPROM.
