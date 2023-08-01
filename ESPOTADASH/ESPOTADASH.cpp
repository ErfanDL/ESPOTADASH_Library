#include "ESPOTADASH.h"

ESPOTADASH::ESPOTADASH(const char* ssid, const char* password, const char* hostName, const char* serverAddress, unsigned long heartbeatInterval, unsigned long registrationInterval, const char* firmwareVersion) {
  this->ssid = ssid;
  this->password = password;
  this->hostName = hostName;
  this->serverAddress = serverAddress;
  this->heartbeatInterval = heartbeatInterval;
  this->registrationInterval = registrationInterval;
  this->firmwareVersion = firmwareVersion;
}

void ESPOTADASH::begin() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
  }
  // Register with the Node.js server on startup
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.hostname(hostName);
    Serial.print("Registering host name: ");
    Serial.println(hostName);
    registerToNodeJS(hostName, firmwareVersion, WiFi.macAddress().c_str(), WiFi.RSSI());
    lastRegistrationTime = millis(); // Set the last registration time to the current time
  }
}

void ESPOTADASH::loop() {
  // Check if firmware flashing is ongoing
  if (isFirmwareFlashing) {
    // The firmware flashing process is handled by ESP8266httpUpdate or ESP32httpUpdate automatically.
    // No additional action is required here.
  } else {
    // If not flashing, check if a firmware update is available and start the update process
    if (WiFi.status() == WL_CONNECTED) {
      unsigned long currentMillis = millis();
      // Check if it's time to send a heartbeat
      if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
        sendHeartbeat();
        lastHeartbeatTime = currentMillis; // Update the last heartbeat time
      }
      // Check if it's time to re-register with the server
      if (currentMillis - lastRegistrationTime >= registrationInterval) {
        reRegisterDevice();
        lastRegistrationTime = currentMillis; // Update the last registration time
      }
      WiFiClient client;
      HTTPClient http;
      String updateURL = (String(serverAddress) + "/updateStatus?hostName=" + hostName);
      http.begin(client, updateURL);
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        if (response == "Update Available") {
          Serial.println("Firmware update available. Starting OTA update...");
		  sendFirmwareFlashingInitiated();
          isFirmwareFlashing = true; // Set the flag to indicate that firmware flashing is ongoing
          #if defined(ESP8266)
          t_httpUpdate_return ret = ESPhttpUpdate.update(client, String(serverAddress) + "/upload/" + hostName + "_firmware.bin");
          #elif defined(ESP32)
          t_httpUpdate_return ret = ESP32shttpUpdate.update(String(serverAddress) + "/upload/" + hostName + "_firmware.bin", firmwareVersion);
          #endif
          switch (ret) {
            case HTTP_UPDATE_FAILED:
              break;
            case HTTP_UPDATE_NO_UPDATES:
              break;
            case HTTP_UPDATE_OK:
              Serial.println("OTA Update Completed!");
              ESP.restart();
              break;
          }
        } else {
        }
      } else {
      }
      http.end();
    }
  }
}

void ESPOTADASH::registerToNodeJS(const char* hostName, const char* firmwareVersion, const char* macAddress, int wifiSignalStrength) {
  // Connect to the Node.js server using HTTP POST request
  WiFiClient client;
  HTTPClient http;
  // Construct the registration URL with the host name and firmware version parameters
  String registrationURL = String(serverAddress) + "/register";
  http.begin(client, registrationURL);
  // Set the host name, firmware version, MAC address, Wi-Fi signal strength, and IP address as the request data
  String postData = String(hostName) + "\n" + firmwareVersion + "\n" + macAddress + "\n" + wifiSignalStrength + "\n" + WiFi.localIP().toString();
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST(postData);
  if (httpCode > 0) {
    Serial.printf("Registered with Node.js server. HTTP code: %d\n", httpCode);
  } else {
    Serial.printf("Failed to connect to Node.js server. HTTP error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void ESPOTADASH::sendHeartbeat() {
  // Connect to the Node.js server using HTTP POST request
  WiFiClient client;
  HTTPClient http;
  // Construct the heartbeat URL with the host name parameter
  String heartbeatURL = String(serverAddress) + "/heartbeat";
  http.begin(client, heartbeatURL);
  // Set the host name as the request data
  String postData = String(hostName);
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST(postData);
  if (httpCode > 0) {
    Serial.printf("Heartbeat sent to Node.js server. HTTP code: %d\n", httpCode);
  } else {
    Serial.printf("Failed to send heartbeat to Node.js server. HTTP error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void ESPOTADASH::reRegisterDevice() {
  // Re-register with the Node.js server on a periodic interval
  registerToNodeJS(hostName, firmwareVersion, WiFi.macAddress().c_str(), WiFi.RSSI());
}

void ESPOTADASH::sendFirmwareFlashingInitiated() {
    // Connect to the Node.js server using HTTP POST request
    WiFiClient client;
    HTTPClient http;

    // Construct the URL for sending the firmware flashing initiated message
    String firmwareInitiatedURL = String(serverAddress) + "/firmwareInitiated?hostName=" + hostName; // Include the hostname as a query parameter

    http.begin(client, firmwareInitiatedURL);

    // Set the request data (you can include additional data if needed)
    String postData = "Hello"; // Modify this line to send the "Hello" message or any other data
    http.addHeader("Content-Type", "text/plain");
    int httpCode = http.POST(postData);

    if (httpCode > 0) {
        Serial.printf("Firmware flashing initiated message sent to Node.js server. HTTP code: %d\n", httpCode);
    } else {
        Serial.printf("Failed to send firmware flashing initiated message to Node.js server. HTTP error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

