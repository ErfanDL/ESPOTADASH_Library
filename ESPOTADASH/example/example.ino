#include "ESPOTADASH.h"

const char* ssid = "Your_SSID";
const char* password = "Your_WiFi_Password";
const char* hostName = "ESP Devices"; // You can modify this to your desired host name
const char* serverAddress = "http://Your_Server_IP:3000"; // Replace with your Node.js server address

unsigned long heartbeatInterval = 10000;     // Modify the heartbeat interval (e.g., 10 seconds)
unsigned long registrationInterval = 30000;  // Modify the registration interval (e.g., 30 seconds)
unsigned long commandCheckInterval = 10000;  // Modify the commandCheck interval (e.g., 10 seconds)
unsigned long updateInterval = 10000;        // Modify the Firmware check Update interval (e.g., 10 seconds)
const char* firmwareVersion = "1.0.0";       // Modify the firmware version

ESPOTADASH ota(ssid, password, hostName, serverAddress, heartbeatInterval, registrationInterval, commandCheckInterval, updateInterval, firmwareVersion);

void setup() {
  ota.begin();
}

void loop() {
  ota.loop();
}

// Implement the processReceivedCommand function here
void ESPOTADASH::processReceivedCommand(const String& command) {
  if (command == "action1") {
    // Perform action 1
    Serial.println("HELLO");
  } else if (command == "action2") {
    // Perform action 2
    Serial.println("ByeBye");
  }
  // Add more conditions for other actions as needed.
}