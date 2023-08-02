#include "ESPOTADASH.h"

const char* ssid = "Your_SSID";
const char* password = "Your_WiFi_Password";
const char* hostName = "ESP8266"; // You can modify this to your desired host name
const char* serverAddress = "http://Your_Server_IP:3000"; // Replace with your Node.js server address

unsigned long heartbeatInterval = 10000; // Modify the heartbeat interval (e.g., 10 seconds)
unsigned long registrationInterval = 30000; // Modify the registration interval (e.g., 30 seconds)
const char* firmwareVersion = "1.0.0"; // Modify the firmware version

ESPOTADASH ota(ssid, password, hostName, serverAddress, heartbeatInterval, registrationInterval, firmwareVersion);

void setup() {
  ota.begin();
}

void loop() {
  ota.loop();
}