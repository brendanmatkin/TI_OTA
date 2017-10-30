#include "TI_OTA.h"
//#include "Arduino.h"

//void TI_OTA::begin(const char** _deviceName) {
void TI_OTA::begin(String* _deviceName) {
  deviceName = _deviceName->c_str();
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(deviceName);
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");
  ArduinoOTA.onStart([&]() {
    Serial.println("Start OTA");
  });

  ArduinoOTA.onEnd([&]() {
    Serial.println("\nEnd OTA");
    for (int i = 0; i < 3; i++) {              // number of LED blinks (3 bright)
      delay(100);
      if (OTA_onboardLED) digitalWrite(LED_BUILTIN, LOW);          // turn LED on
      delay(100);
      if (OTA_onboardLED) digitalWrite(LED_BUILTIN, HIGH);         // turn LED off
    }
  });

  ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total) {
    if (OTA_onboardLED) digitalWrite(LED_BUILTIN, LOW);    // LED on
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    if (OTA_onboardLED) digitalWrite(LED_BUILTIN, HIGH);   // led off (serial call takes a few ms to run)
  });

  ArduinoOTA.onError([&](ota_error_t error) {
    if (OTA_onboardLED) digitalWrite(LED_BUILTIN, LOW);       // turn LED on
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    delay(1000);                          // leave LED on for a second
    if (OTA_onboardLED) digitalWrite(LED_BUILTIN, HIGH);      // turn LED off
  });

  ArduinoOTA.begin();
}


void TI_OTA::loop() {
  ArduinoOTA.handle();
}


void TI_OTA::checkServer(String fwUrlBase, String fwName, const int FW_VERSION) {
  /* get latest version number: */
  //String mac = getMAC();
  String fwURL = String(fwUrlBase);
  //fwURL.concat(mac);
  fwURL.concat(fwName);
  String fwVersionURL = fwURL;
  fwVersionURL.concat(".version");    // version number file (use this number to check if update available)
  if (OTA_SERIAL_DEBUG) {
    Serial.println( "\nChecking for firmware updates." );
    Serial.print( "FW Name: " );
    Serial.println( fwName );
    Serial.print( "Firmware version URL: " );
    Serial.println( fwVersionURL );
  }

  HTTPClient httpClient;
  httpClient.begin(fwVersionURL);     // might have to use .begin(uri, port);
  int httpCode = httpClient.GET();
  if (httpCode == 200) {              // OK
    /* check available firmware version: */
    String newFWVersion = httpClient.getString();
    if (OTA_SERIAL_DEBUG) {
      Serial.print("Current firmware version: ");
      Serial.println(FW_VERSION);
      Serial.print("Available firmware version: ");
      Serial.println(newFWVersion);
    }
    int newVersion = newFWVersion.toInt();

    /* see if available firmware is newer: */
    if (newVersion > FW_VERSION) {
      if (OTA_SERIAL_DEBUG) Serial.println("Preparing to update...");
      String fwImageURL = fwURL;
      fwImageURL.concat(".bin");
      t_httpUpdate_return ret = ESPhttpUpdate.update(fwImageURL);   // OTA updater
      /* in case the updater fails: */
      switch(ret) {
        case HTTP_UPDATE_FAILED:
          if (OTA_SERIAL_DEBUG) Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          if (OTA_SERIAL_DEBUG) Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;
      }
    }
    else {
      if (OTA_SERIAL_DEBUG) Serial.println("Already using latest version");
    }
  }
  else {
    if (OTA_SERIAL_DEBUG) Serial.print("Firmware version check failed, got HTTP response code: ");
    if (httpCode > 0 && OTA_SERIAL_DEBUG) Serial.println(httpCode);
    else if (OTA_SERIAL_DEBUG) Serial.println(httpClient.errorToString(httpCode).c_str());
  }
  httpClient.end();
}
