#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <U8g2lib.h>
#include <WiFiMulti.h>
#include "config.h"

WiFiMulti wifiMulti;

// u-blox 7M GPS module connected to ESP32 UART2.
#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600
TinyGPSPlus gps;
HardwareSerial GPSSerial(2);

// 128x64 SH1106 OLED connected over I2C.
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /*reset=*/U8X8_PIN_NONE);

// Last reverse-geocoded address shown on the display.
char addressLine[64] = "Acquiring address...";
uint32_t lastQuery = 0;

void connectWiFi() {
  WiFi.mode(WIFI_STA);

  for (size_t i = 0; i < WIFI_NETWORK_COUNT; ++i) {
    wifiMulti.addAP(WIFI_NETWORKS[i].ssid, WIFI_NETWORKS[i].password);
  }

  Serial.println("Connecting to WiFi...");

  // Keep trying until one of the configured networks is available.
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void queryNominatim(double lat, double lon) {
  // Use a compact URL while keeping enough precision for a readable address.
  String url = "https://nominatim.openstreetmap.org/reverse?format=jsonv2";
  url += "&lat=" + String(lat, 5) + "&lon=" + String(lon, 5);

  HTTPClient http;
  http.begin(url);
  http.addHeader("User-Agent",
                 "esp32-gps-demo/1.0 (contact: replace-with-your-email@example.com)");

  int code = http.GET();
  if (code != 200) {
    snprintf(addressLine, sizeof(addressLine), "HTTP %d", code);
    http.end();
    return;
  }

  // The JSON response is small enough for a fixed-size document buffer.
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    strncpy(addressLine, "JSON error", sizeof(addressLine));
    return;
  }

  // Show a short and readable address on the display.
  const char* road = doc["address"]["road"] | doc["name"] | "";
  const char* city = doc["address"]["city"] | doc["address"]["town"] |
                     doc["address"]["village"] | "";
  if (*road || *city) {
    snprintf(addressLine, sizeof(addressLine), "%s %s", road, city);
  } else {
    strncpy(addressLine, "Address unknown", sizeof(addressLine));
  }
}

// Draw a string on multiple lines, keeping words intact where possible.
void printwords(const char *msg, int xloc, int yloc) {
  int dspwidth = oled.getDisplayWidth();
  int strwidth = 0;
  char glyph[2];
  glyph[1] = 0;

  for (const char *ptr = msg, *lastblank = NULL; *ptr; ++ptr) {
    while (xloc == 0 && *msg == ' ') {
      if (ptr == msg++) {
        ++ptr;
      }
    }

    glyph[0] = *ptr;
    strwidth += oled.getStrWidth(glyph);
    if (*ptr == ' ') {
      lastblank = ptr;
    } else {
      ++strwidth;
    }

    if (xloc + strwidth > dspwidth) {
      int starting_xloc = xloc;
      while (msg < (lastblank ? lastblank : ptr)) {
        glyph[0] = *msg++;
        xloc += oled.drawStr(xloc, yloc, glyph);
      }

      strwidth -= xloc - starting_xloc;
      yloc += oled.getMaxCharHeight();
      xloc = 0;
      lastblank = NULL;
    }
  }

  while (*msg) {
    glyph[0] = *msg++;
    xloc += oled.drawStr(xloc, yloc, glyph);
  }
}

void drawScreen(double lat, double lon, uint8_t sats, double hdop) {
  oled.clearBuffer();
  char buf[48];

  oled.setCursor(0, 12);
  snprintf(buf, sizeof(buf), "Lat: %.5f", lat);
  oled.print(buf);

  oled.setCursor(0, 24);
  snprintf(buf, sizeof(buf), "Lon: %.5f", lon);
  oled.print(buf);

  oled.setCursor(0, 36);
  snprintf(buf, sizeof(buf), "Sat:%2d HDOP:%.1f", sats, hdop);
  oled.print(buf);

  printwords(addressLine, 0, 48);
  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);

  oled.begin();
  oled.setFont(u8g2_font_profont11_tf);
  oled.enableUTF8Print();
  oled.drawStr(0, 12, "Connecting WiFi...");
  oled.sendBuffer();

  connectWiFi();

  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  oled.clearBuffer();
  oled.drawStr(0, 12, "Looking for GPS...");
  oled.sendBuffer();
}

void loop() {
  // Feed GPS bytes into the parser as they arrive.
  while (GPSSerial.available()) {
    gps.encode(GPSSerial.read());
  }

  // Refresh the display only when a new GPS position is available.
  if (gps.location.isUpdated()) {
    double lat = gps.location.lat();
    double lon = gps.location.lng();

    // Respect the public Nominatim service by limiting request frequency.
    uint32_t now = millis();
    if (now - lastQuery > 15000 && WiFi.status() == WL_CONNECTED) {
      lastQuery = now;
      queryNominatim(lat, lon);
    }

    drawScreen(lat, lon, gps.satellites.value(), gps.hdop.hdop());

    Serial.printf("%.5f, %.5f | %s | Sat:%d HDOP:%.1f\n",
                  lat, lon, addressLine, gps.satellites.value(),
                  gps.hdop.hdop());
  }
}
