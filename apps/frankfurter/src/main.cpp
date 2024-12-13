
#include <Arduino.h>
#include <otaserver.h>
#include <kgfx.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

Preferences preferences;
OTAServer otaserver;
KGFX ui;

HTTPClient http;
JsonDocument json;
const int refreshTimeInSeconds = 1800;
unsigned long lastTime = 0;
String baseUrl = "https://api.frankfurter.dev/v1";
String baseCurrency;
String targetCurrency;
String since;
int sinceDays;
int lloop;
const int screenWidth = 240;
TFT_eSprite textSpr = ui.createSprite(screenWidth, 80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 1800);

void setup() {
  Serial.begin(460800);
  Serial.println("Starting app");

  otaserver.connectWiFi(); // DO NOT EDIT.
  otaserver.run(); // DO NOT EDIT

  preferences.begin("app", true);
  baseCurrency = preferences.getString("base_currency", "EUR");
  targetCurrency = preferences.getString("target_currency", "USD");
  since = preferences.getString("since", "90d");
  preferences.end();

  if (since == "30d" ){
    sinceDays = 30;
  } else if (since == "90d") {
    sinceDays = 90;
  } else if (since == "180d") {
    sinceDays = 180;
  } else if (since == "365d") {
    sinceDays = 365;
  }

  ui.init();
  ui.clear();

  lloop = 0;
  ui.createChartSpriteLarge(screenWidth, 160);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print ( "." );
  }
  timeClient.begin();
}

String timeToString(time_t time) {
  struct tm *tm = localtime(&time);
  char dateStr[11];
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tm);

  return String(dateStr);
}

String daysAgo(time_t time, int daysToRemove) {
  time -= daysToRemove * 24 * 60 * 60;

  return timeToString(time);
}

void loop() {
  if((WiFi.status() == WL_CONNECTED)) {
    otaserver.handle(); // DO NOT EDIT

    timeClient.update();

    if (((millis() - lastTime) > refreshTimeInSeconds*1000) || lastTime == (unsigned long)(0)) {


      lloop++;

      lastTime = millis();

      time_t time = timeClient.getEpochTime();
      String currentDate = timeToString(time);

      Serial.println(currentDate);

      String sinceDate = daysAgo(time, sinceDays);

      String url = baseUrl + "/" + sinceDate + "..?base=" + baseCurrency + "&symbols=" + targetCurrency;
      Serial.println(url);
      http.begin(url);
      int httpResponseCode = http.GET();

      if (httpResponseCode < 0) {
        ui.drawText(textSpr, "HTTP Error: API not-reachable", Arial_14_Bold, TFT_RED, 0, 0);
        return;
      }

      if (httpResponseCode != HTTP_CODE_OK) {
        ui.drawText(textSpr, "HTTP Error", Arial_14_Bold, TFT_RED, 0, 0);
        return;
      }

      String payload = http.getString();

      DeserializationError error = deserializeJson(json, payload);
      if (error) {
        ui.drawText(textSpr, "Cannot parse JSON", Arial_14_Bold, TFT_RED, 0, 0);
        return;
      }

      http.end();

      std::vector<float> arr = {};
      String lastValue;

      for (JsonObject::iterator it = json["rates"].as<JsonObject>().begin(); it != json["rates"].as<JsonObject>().end(); ++it) {
        String date = it->key().c_str();
        if (json["rates"][date][targetCurrency].is<float>()) {
          lastValue = json["rates"][date][targetCurrency].as<String>();
          Serial.println(date + ": " + lastValue);
          arr.push_back(lastValue.toFloat());
        } else {
          Serial.println(date + ": " + lastValue);
          arr.push_back(lastValue.toFloat());
        }
      }

      String title = baseCurrency + "/" + targetCurrency;
      ui.drawText(textSpr, title.c_str(), Arial_14_Bold, TFT_WHITE, 0, 0);
      ui.drawText(textSpr, currentDate.c_str(), Arial_14, TFT_WHITE, (screenWidth - currentDate.length()*10), 0);
      ui.drawText(textSpr, (since).c_str(), Arial_14, TFT_WHITE, 0, 40);
      ui.drawText(textSpr, String(lloop).c_str(), Arial_14, TFT_WHITE, 60, 40);
      ui.drawText(textSpr, lastValue.c_str(), Arial_28_Bold, TFT_GREEN, (screenWidth - lastValue.length()*20), 40);

      ui.drawChartLarge(arr, TFT_GREEN, 80, 160);
      // Free resources
      json.clear();
    }
  }

  delay(1);
}
