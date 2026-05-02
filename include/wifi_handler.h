/*
 * ============================================
 *  WiFi Handler — Simple connect / reconnect
 * ============================================
 */

#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <WiFi.h>

// WiFi credentials
#define WIFI_SSID "desktop"
#define WIFI_PASS "12345678"

/* Connect to WiFi — blocks until connected. Call once in setup(). */
void wifi_connect() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed! Continuing without WiFi...");
  }
}

/* Check connection and reconnect if dropped. Call at top of loop(). */
void wifi_ensure_connected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost — reconnecting...");
    wifi_connect();
  }
}

#endif
