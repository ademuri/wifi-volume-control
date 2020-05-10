# 1 "/tmp/tmpwt6B6w"
#include <Arduino.h>
# 1 "/home/adam/hardware/wifi-volume-control/src/src.ino"
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <iomanip>
#include <string>
#include <debounce-filter.h>
#include <lwip/etharp.h>

#include "constants.h"
#include "html.h"
#include "mcp4661.h"
#include "steps.h"

AsyncWebServer server(80);
Mcp4661 pot( D2, D1);
DebounceFilter *upButton;
DebounceFilter *downButton;

uint16_t volume = 0;
uint16_t new_volume = 0;
void gratuitous_arp();
String processor(const String& var);
void setup();
void loop();
#line 23 "/home/adam/hardware/wifi-volume-control/src/src.ino"
void gratuitous_arp() {
  netif *n = netif_list;
  while (n) {
    etharp_gratuitous(n);
    n = n->next;
  }
}

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


  wifi_set_sleep_type(NONE_SLEEP_T);

  pinMode(D7, INPUT_PULLUP);
  pinMode(D6, INPUT_PULLUP);
  upButton = new DebounceFilter([=]() { return !digitalRead(D7); });
  downButton = new DebounceFilter([=]() { return !digitalRead(D6); });

  pot.begin();
  delay(5);
  uint16_t volume_0 = pot.read_register(Mcp4661::kNonVolatileWiper0);
  uint16_t volume_1 = pot.read_register(Mcp4661::kNonVolatileWiper1);
  if (volume_0 != volume_1) {
    Serial.printf("Warning: read volumes differ. %u vs %u\n", volume_0, volume_1);
    volume_0 = kSteps.size();
    volume_1 = volume_0;




    pot.write_register(Mcp4661::kNonVolatileWiper0, volume_0);
    delay(5);
    pot.write_register(Mcp4661::kNonVolatileWiper1, volume_1);
    delay(5);
  }
  Serial.printf("Read inital volume: %u\n", volume_0);

  if (volume_0 != 0) {
    for (unsigned int i = 0; i < kSteps.size(); i++) {



      if (volume <= kSteps[i]) {
        volume = i;
        break;
      }
    }
  }
  new_volume = volume;

  Serial.print("Connecting to wifi: ");
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
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
      } else {
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
  ArduinoOTA.begin( false);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/volume", HTTP_GET, [](AsyncWebServerRequest *request) {
    std::ostringstream ostream;
    ostream << volume;
    request->send_P(200, "text/plain", ostream.str().c_str());
  });


  server.on("/volume", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->params() == 0) {
      Serial.println("No params in request");
      request->send(400);
      return;
    }


    AsyncWebParameter* param = request->getParam(0);
    if (!param->isPost()) {
      Serial.println("Invalid param type");
      request->send(400);
      return;
    }

    new_volume = String(param->value()).toInt();
    if (new_volume >= kSteps.size()) {
      new_volume = 0;
    }
    request->send(200);
  });

  server.begin();
  Serial.println("Started server.");

  if (MDNS.begin("volume-control", WiFi.localIP(), 1)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up mDNS responder!");
  }
}

int32_t write_nonvolatile_at = -1;
void loop() {
  ArduinoOTA.handle();
  MDNS.update();
  upButton->Run();
  downButton->Run();

  if (upButton->Rose() && new_volume < kSteps.size() - 1) {
    new_volume++;
  }
  if (downButton->Rose() && new_volume > 0) {
    new_volume--;
  }

  if (new_volume != volume) {
    Serial.printf("Writing new volume: %u (%u)\n", new_volume, kSteps[new_volume]);
    pot.write_register(Mcp4661::kVolatileWiper0, kSteps[new_volume]);
    pot.write_register(Mcp4661::kVolatileWiper1, kSteps[new_volume]);

    volume = new_volume;
    write_nonvolatile_at = millis() + 10 * 1000;
  }

  if (write_nonvolatile_at > 0 && millis() > write_nonvolatile_at) {
    Serial.println("Writing volume to non-volatile");
    pot.write_register(Mcp4661::kNonVolatileWiper0, kSteps[new_volume]);
    delay(5);
    pot.write_register(Mcp4661::kNonVolatileWiper1, kSteps[new_volume]);
    delay(5);
    write_nonvolatile_at = -1;
  }

  delay(10);
}