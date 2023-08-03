#ifndef ESPOTADASH_H
#define ESPOTADASH_H

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>
#endif

class ESPOTADASH {
public:
  ESPOTADASH(const char* ssid, const char* password, const char* hostName, const char* serverAddress, unsigned long heartbeatInterval, unsigned long registrationInterval, const char* firmwareVersion);
  void begin();
  void loop();

private:
  const char* ssid;
  const char* password;
  const char* hostName;
  const char* serverAddress;
  const char* firmwareVersion;
  unsigned long lastHeartbeatTime = 0;
  unsigned long heartbeatInterval;
  unsigned long lastRegistrationTime = 0;
  unsigned long registrationInterval;
  bool isFirmwareFlashing = false;

  void registerToNodeJS(const char* hostName, const char* firmwareVersion, const char* macAddress, int wifiSignalStrength);
  void sendHeartbeat();
  void reRegisterDevice();
  void sendFirmwareFlashingInitiated();
  String urlEncode(const String& str);
};

#endif