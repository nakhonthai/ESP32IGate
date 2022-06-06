#include <PPPoS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "TTGO_TCall.h"

#define PPP_APN   "internet"
#define PPP_USER  ""
#define PPP_PASS  ""

PPPoS ppp;

void setup()
{
  Serial.begin(115200);
  delay(100);

  setupModem();

  Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  Serial1.setTimeout(10);
  Serial1.setRxBufferSize(2048);
  ppp.begin(&Serial1);

  Serial.print("Connecting PPPoS");
  ppp.connect(PPP_APN, PPP_USER, PPP_PASS);
  while (!ppp.status()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("OK");

  // Check plain TCP connection
  requestJSON("http://httpbin.org/anything?client=ESP32_PPPoS");

  // Check secure SSL/TLS connection
  requestJSON("https://www.howsmyssl.com/a/check");

  ppp.end();
}

void loop() {
  /* do nothing */
  delay(1000);
}

void requestJSON(String url)
{
  Serial.print("Requesting "); Serial.println(url);
  HTTPClient http;

  if (!http.begin(url)) {
    Serial.println("Cannot initiate request");
    return;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Status code: ");
    Serial.println(httpCode);
    if (httpCode < 0) {
      return;
    }
  }

  // Parse JSON
  DynamicJsonDocument doc(1024*10);
  DeserializationError error = deserializeJson(doc, http.getStream());
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.print(error.f_str());
  } else {
    serializeJsonPretty(doc, Serial);
  }

  http.end();
  Serial.println();
}
