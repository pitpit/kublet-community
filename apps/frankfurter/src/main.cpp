
#include <Arduino.h>
#include <otaserver.h>
#include <kgfx.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

Preferences preferences;
OTAServer otaserver;
KGFX ui;
JsonDocument json;
HTTPClient http;
String baseUrl = "https://api.frankfurter.dev/v1";
String baseCurrency;
String targetCurrency;
int refreshTimeInSeconds = 300;
unsigned long lastTime = 0;

TFT_eSprite textSpr = ui.createSprite(240, 150);

void setup() {
  Serial.begin(460800);
  Serial.println("Starting app");

  otaserver.connectWiFi(); // DO NOT EDIT.
  otaserver.run(); // DO NOT EDIT

  preferences.begin("app", true);
  baseCurrency = preferences.getString("base_currency", "EUR");
  targetCurrency = preferences.getString("target_currency", "USD");
  preferences.end();

  ui.init();
  ui.clear();
}

void loop() {
  if((WiFi.status() == WL_CONNECTED)) {
    otaserver.handle(); // DO NOT EDIT

    if (((millis() - lastTime) > refreshTimeInSeconds*1000) || lastTime == (unsigned long)(0)) {

      lastTime = millis();

      http.begin(baseUrl + "/latest?base=" + baseCurrency + "&symbols=" + targetCurrency);
      int httpResponseCode = http.GET();

      if (httpResponseCode < 0) {
        ui.drawText(textSpr, "HTTP Error: Probably incorrect or not-reachable API URL.", Arial_14_Bold, TFT_RED, 0, 0);
        return;
      }

      if (httpResponseCode != HTTP_CODE_OK) {
        ui.drawText(textSpr, "HTTP Error.", Arial_14_Bold, TFT_RED, 0, 0);
        return;
      }

      String payload = http.getString();
      DeserializationError error = deserializeJson(json, payload);
      if (error) {
        ui.drawText(textSpr, "Cannot parse JSON payload", Arial_14_Bold, TFT_RED, 0, 0);
        return;
      }

      String title = baseCurrency + "/" + targetCurrency;
      ui.drawText(textSpr, title.c_str(), Arial_14_Bold, TFT_YELLOW, 0, 0);

      const String rate = json["rates"]["USD"];
      ui.drawText(textSpr, rate.c_str(), Arial_28_Bold, TFT_GREEN, 0, 40);

      // Free resources
      http.end();
    }
  }

  delay(1);
}
