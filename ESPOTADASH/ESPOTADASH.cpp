#include "ESPOTADASH.h"

ESPOTADASH::ESPOTADASH(const char* ssid, const char* password, const char* hostName, const char* serverAddress, unsigned long heartbeatInterval, unsigned long registrationInterval, unsigned long commandCheckInterval, unsigned long updateInterval, const char* firmwareVersion) {
  this->ssid = ssid;
  this->password = password;
  this->hostName = hostName;
  this->serverAddress = serverAddress;
  this->heartbeatInterval = heartbeatInterval;
  this->commandCheckInterval = commandCheckInterval;
  this->registrationInterval = registrationInterval;
  this->updateInterval = updateInterval;
  this->firmwareVersion = firmwareVersion;
}

void ESPOTADASH::begin() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Add a delay here to allow some time for Wi-Fi connection to initialize
  delay(2000);

  // Register with the Node.js server on startup
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.hostname(hostName);
    Serial.print("Registering host name: ");
    Serial.println(hostName);
    registerToNodeJS(hostName, firmwareVersion, WiFi.macAddress().c_str(), WiFi.RSSI());
    lastRegistrationTime = millis(); // Set the last registration time to the current time
  }
}

// Helper function to manually encode the hostName
String ESPOTADASH::urlEncode(const String& str) {
  String encodedString = "";
  char c;
  char bufHex[3];
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedString += c;
    } else if (c == ' ') {
      encodedString += "%20";
    } else {
      sprintf(bufHex, "%%%02X", c);
      encodedString += bufHex;
    }
  }
  return encodedString;
}

void ESPOTADASH::loop() {
  static enum class State {
    CONNECT_WIFI,
    CHECK_WIFI,
    FIRMWARE_UPDATE,
    IDLE
  } currentState = State::CONNECT_WIFI;

  static unsigned long lastCheckTime = 0;
  const unsigned long checkInterval = 5000; // Check Wi-Fi every 5 seconds

  unsigned long currentMillis = millis(); // Get the current time

  switch (currentState) {
    case State::CONNECT_WIFI:
      if (WiFi.status() != WL_CONNECTED) {
        if (currentMillis - lastCheckTime >= checkInterval) {
          lastCheckTime = currentMillis;
          Serial.println("Connecting to WiFi...");
          WiFi.begin(ssid, password);
        }
      } else {
        currentState = State::CHECK_WIFI;
      }
      break;

    case State::CHECK_WIFI:
      if (WiFi.status() == WL_CONNECTED) {
        // Wi-Fi connected, proceed with other operations
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
        if (currentMillis - lastCommandTime >= commandCheckInterval) {
          String command = getCommandFromServer();
          if (!command.isEmpty()) {
            // Process the received command
            processReceivedCommand(command);
          }
          lastCommandTime = currentMillis; // Update the last command time
        }
        // Inside the `case State::CHECK_WIFI:` block, replace the existing if statement with this
        if (currentMillis - lastUpdateTime >= updateInterval) {
          lastUpdateTime = currentMillis; // Update the last update time
          WiFiClient client;
          HTTPClient http;
          String updateURL = (String(serverAddress) + "/updateStatus?hostName=" + urlEncode(hostName)); // Use the urlEncode() function here
          http.begin(client, updateURL);
          int httpCode = http.GET();
          if (httpCode == HTTP_CODE_OK) {
            String response = http.getString();
            if (response == "Update Available") {
              Serial.println("Firmware update available. Starting OTA update...");
              sendFirmwareFlashingInitiated();
              isFirmwareFlashing = true; // Set the flag to indicate that firmware flashing is ongoing
              #if defined(ESP8266)
              ESPhttpUpdate.setLedPin(2, LOW);
              t_httpUpdate_return ret = ESPhttpUpdate.update(client, String(serverAddress) + "/upload/" + urlEncode(hostName) + "_firmware.bin"); // Use urlEncode() function here
              #elif defined(ESP32)
              t_httpUpdate_return ret = ESP32shttpUpdate.update(String(serverAddress) + "/upload/" + urlEncode(hostName) + "_firmware.bin", firmwareVersion); // Use urlEncode() function here
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
            }

            http.end();
          }
        }
      } else {
        // Wi-Fi connection lost, attempt reconnection
        currentState = State::CONNECT_WIFI;
        lastCheckTime = currentMillis; // Reset the last check time
      }
      break;

    case State::FIRMWARE_UPDATE:
      // The firmware flashing process is handled by ESP8266httpUpdate or ESP32httpUpdate automatically.
      // No additional action is required here.
      currentState = State::IDLE;
      break;

    case State::IDLE:
      // Do nothing in the IDLE state
      break;
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
    String firmwareInitiatedURL = String(serverAddress) + "/firmwareInitiated?hostName=" + urlEncode(hostName); // Include the hostname as a query parameter

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

String ESPOTADASH::getCommandFromServer() {
  // Connect to the Node.js server using HTTP GET request
  WiFiClient client;
  HTTPClient http;

  // Construct the URL to request the command
  String commandURL = String(serverAddress) + "/getCommand?hostName=" + urlEncode(hostName);

  http.begin(client, commandURL);

  // Set the content type to "text/plain" for the HTTP GET request
  http.addHeader("Content-Type", "text/plain");

  // Send the HTTP GET request
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    //Serial.print("Received command from server: ");
    //Serial.println(response);

    // Close the connection
    http.end();

    // Add a small delay to allow the server to respond
    delay(100);

    // Return the received command
    return response;
  } else {
    Serial.printf("HTTP error code: %d\n", httpCode);
    Serial.printf("Failed to fetch command from the server. HTTP error: %s\n", http.errorToString(httpCode).c_str());

    // Close the connection in case of an error
    http.end();
  }

  return ""; // Return an empty string if no command received or there was an error
}