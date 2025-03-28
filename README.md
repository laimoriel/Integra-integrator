The project was created with 2 main areas of functionality:
1. To serve as a transparent TCP socket <-> UART interface. Combined with virtual COM port installed on PC it allows to wirelessly connect to remote devices using serial port. It's like a wireless serial converter.
   * A simple webpage is provided to setup WiFi connectivity and configure UART port parameters,
   * Supported baud rates from 1200 to 115200 and all common RS232 modes,
   * Device can work in both in AP and Station modes,   
2. To provide integration with a popular Satel Integra alarm system. List of currently supported features include:
   * Device can be connected to the RS232 port located in Integra manipulator and use the official simplified integration protocol from Satel to:
      - Remotely arm, disarm and clear alarm in selected zones via webpage,
      - Provide live view of configured inputs, zones and outputs via webpage,
      - Device can also send notification over WhatsApp whenever zone status is changed (armed, disarmed, alarm triggered etc.).
      - It also provides nice statistical overview of data transferred via the TCP <-> UART bridge and data frames received from Integra.
   * It also allows to remotely use DLOADx to connect to Integra central unit remotely without cable.
   * Device was tested with Integra 32 central unit and INT-KLCD manipulator, but it should also work with other Integra series central units.
     
Hardware setup is simple and cheap. Requires only ESP32, some level converter from TTL to RS232 (possibly MAX232) and a 5V power source.

Setup of the device is quite simple. 
By default device will initially start in AP mode. You need to connect to the AP, open http://192.168.1.1 in browser, enter settings webpage and configure credentials of your WLAN.
After restart device will start in Client mode. You need to identify according IP, enter its webpage and continue configuration.
WiFi and WhatsApp credentials, TCP sockets and UART parameters are configured via webpage.
Configuration of inputs, zones and outputs in the alarm system is done via .cfg files located in ./data directory.
You don't need to configure all inputs/zones/outputs you have in your system, just the ones you want to monitor via the device.

THE DEVICE NEVER STORES ANY PASSWORDS TO THE ALARM SYSTEM! ONCE ENTERED AND SUBMITTED THEY ARE ONLY DISPATCHED VIA UART TO THE ALARM SYSTEM.
