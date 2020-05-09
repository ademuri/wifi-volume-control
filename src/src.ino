#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <iomanip>
#include <string>

#include "constants.h"
#include "html.h"
#include "mcp4661.h"
#include "steps.h"

AsyncWebServer server(80);
Mcp4661 pot(/* SDA */ D2, /* SCL */ D1);

uint16_t volume = 0;
uint16_t new_volume = 0;

String processor(const String& var) {
  if(var == "NUM_STEPS") {
    return String(kSteps.size() - 1);
  }

  Serial.printf("Warning: unhandled template var '%s'\n", var.c_str());
  return String();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booted.");
  delay(500);
  Serial.println("");

  pot.begin();
  delay(5);
  uint16_t volume_0 = pot.read_register(Mcp4661::kNonVolatileWiper0);
  uint16_t volume_1 = pot.read_register(Mcp4661::kNonVolatileWiper1);
  if (volume_0 != volume_1) {
    Serial.printf("Warning: read volumes differ. %u vs %u\n", volume_0, volume_1);
    volume_0 = 0;
    volume_1 = 0;
    pot.write_register(Mcp4661::kNonVolatileWiper0, volume_0);
    pot.write_register(Mcp4661::kNonVolatileWiper1, volume_1);
  }
  Serial.printf("Read inital volume: %u\n", volume_0);
  // Backconvert from volume to stepped volume
  if (volume_0 != 0) {
    for (unsigned int i = 0; i < kSteps.size(); i++) {
      // kSteps is sorted ascending, so this will pick a close volume. This
      // isn't totally optimal, but the stored value should always be in
      // kSteps during normal operation.
      if (volume <= kSteps[i]) {
        volume = i;
        break;
      }
    }
  }
  new_volume = volume;

  Serial.print("Connecting to wifi: ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(kSsid, kPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Done.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(500);

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_SPIFFS
        type = "filesystem";
      }

      Serial.println("Start updating " + type);
    });
  ArduinoOTA
    .onEnd([]() {
      Serial.println("\nEnd");
    });
  ArduinoOTA
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  ArduinoOTA
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin(/* useMDNS */ false);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/volume", HTTP_GET, [](AsyncWebServerRequest *request) {
    std::ostringstream ostream;
    ostream << volume;
    request->send_P(200, "text/plain", ostream.str().c_str());
  });
  // Note: use POST, not PUT, so that we don't have to include the regex
  // library for URL matching, which is large
  server.on("/volume", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->params() == 0) {
      Serial.println("No params in request");
      request->send(400);
      return;
    }

    // Assume there is only one parameter
    AsyncWebParameter* param = request->getParam(0);
    if (!param->isPost()) {
      Serial.println("Invalid param type");
      request->send(400);
      return;
    }
    
    new_volume = String(param->value()).toInt();
    request->send(200);
  });

  server.begin();
  Serial.println("Started server.");

  if (MDNS.begin("volume-control")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }
}

uint32_t alive_at = 0;
void loop() {
  ArduinoOTA.handle();
  MDNS.update();
  delay(100);

  if (millis() > alive_at) {
    Serial.println(millis());
    alive_at = millis() + 5000;
  }
  if (new_volume != volume) {
    Serial.printf("Writing new volume: %u\n", kSteps[new_volume]);
    pot.write_register(Mcp4661::kNonVolatileWiper0, kSteps[new_volume]);
    pot.write_register(Mcp4661::kNonVolatileWiper1, kSteps[new_volume]);
    pot.write_register(Mcp4661::kVolatileWiper0, kSteps[new_volume]);
    pot.write_register(Mcp4661::kVolatileWiper1, kSteps[new_volume]);

    volume = new_volume;
  }
}
