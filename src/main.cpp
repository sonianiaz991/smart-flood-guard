/*
 * ============================================================
 *  Flood Monitoring System — ESP32 Main Sketch
 * ============================================================
 *
 *  Sensors & Actuators:
 *    - DHT11          → Temperature & Humidity   (GPIO 4)
 *    - Water Level    → Analog input             (GPIO 34)
 *    - Red LED        → Flood alert indicator    (GPIO 2)  <<<--- RED LED PIN
 *    - Buzzer         → Audible flood alert      (GPIO 15)
 *
 *  Data strategy:
 *    - Read sensors every 1.5 seconds (never blocks)
 *    - Buffer readings locally on ESP
 *    - Send buffered batch to API every 10 seconds
 *    - If send fails, data stays in buffer for next attempt
 *    - No data loss even if backend is slow
 * ============================================================
 */

#include <Arduino.h>
#include <HTTPClient.h>
#include "dht11.h"
#include "wifi_handler.h"

// ============ PIN DEFINITIONS ============

//  *** RED LED PIN — GPIO 2 (onboard LED on most ESP32 boards) ***
#define RED_LED_PIN       2

#define BUZZER_PIN        15    // Buzzer output pin
#define WATER_LEVEL_PIN   34    // Analog input for water level sensor

// ============ THRESHOLDS ============

// Water level threshold (0–4095 on ESP32 12-bit ADC)
#define WATER_LEVEL_THRESHOLD  1500

// ============ API CONFIG ============

#define API_URL       "http://192.168.137.1:8000/api/data/batch"
#define HTTP_TIMEOUT  5000   // 5 second timeout for HTTP requests

// ============ TIMING ============
#define READ_INTERVAL  1500    // Read sensors every 1.5 seconds
#define SEND_INTERVAL  3000    // Send batch to API every 3 seconds

// ============ DATA BUFFER ============
// Store up to 10 readings (enough for ~15 seconds if sends fail)
#define MAX_BUFFER  10

struct SensorReading {
  float temperature;
  float humidity;
  int   waterLevel;
  bool  alertOn;
};

SensorReading buffer[MAX_BUFFER];
int bufferCount = 0;

unsigned long lastReadTime = 0;
unsigned long lastSendTime = 0;

// Forward declarations
void read_and_buffer();
void send_buffered_data();

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  delay(2000);  // Give ESP32 time to stabilize
  Serial.println("\n=== Flood Monitoring System ===\n");

  // Initialize pins
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize DHT11 sensor
  dht11_init();

  // Connect to WiFi
  wifi_connect();

  Serial.println("System ready!\n");
}

// ============ MAIN LOOP (non-blocking) ============
void loop() {
  unsigned long now = millis();

  // --- Read sensors every 1.5 seconds ---
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;
    read_and_buffer();
  }

  // --- Send buffered data every 10 seconds ---
  if (now - lastSendTime >= SEND_INTERVAL && bufferCount > 0) {
    lastSendTime = now;
    wifi_ensure_connected();
    if (WiFi.status() == WL_CONNECTED) {
      send_buffered_data();
    } else {
      Serial.println("WiFi not connected — keeping data in buffer.");
    }
  }
}

// ============ READ SENSORS & STORE IN BUFFER ============
void read_and_buffer() {
  float temperature = read_temperature();
  float humidity    = read_humidity();
  int   waterLevel  = analogRead(WATER_LEVEL_PIN);

  // Determine alert
  bool alertOn = (waterLevel > WATER_LEVEL_THRESHOLD);

  // Control LED and Buzzer (always real-time, never delayed)
  digitalWrite(RED_LED_PIN, alertOn ? HIGH : LOW);
  digitalWrite(BUZZER_PIN,  alertOn ? HIGH : LOW);

  // Print to Serial
  Serial.println("========== Sensor Data ==========");
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("WARNING: DHT11 read failed!");
  } else {
    print_dht_summary(temperature, humidity);
  }
  Serial.print("Water Level : "); Serial.print(waterLevel); Serial.println(" / 4095");
  Serial.print("Alert       : "); Serial.println(alertOn ? "ON" : "OFF");
  Serial.print("Buffer      : "); Serial.print(bufferCount + 1); Serial.print("/"); Serial.println(MAX_BUFFER);
  Serial.println("=================================\n");

  // Add to buffer (drop oldest if full)
  if (bufferCount >= MAX_BUFFER) {
    Serial.println("WARNING: Buffer full — dropping oldest reading.");
    for (int i = 0; i < MAX_BUFFER - 1; i++) {
      buffer[i] = buffer[i + 1];
    }
    bufferCount = MAX_BUFFER - 1;
  }

  buffer[bufferCount].temperature = temperature;
  buffer[bufferCount].humidity    = humidity;
  buffer[bufferCount].waterLevel  = waterLevel;
  buffer[bufferCount].alertOn     = alertOn;
  bufferCount++;
}

// ============ SEND ALL BUFFERED DATA AS JSON ARRAY ============
void send_buffered_data() {
  Serial.print("Sending batch of "); Serial.print(bufferCount); Serial.println(" readings...");

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(HTTP_TIMEOUT);

  // Build JSON array
  String json = "[";
  for (int i = 0; i < bufferCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"temperature\":"    + String(buffer[i].temperature, 1) + ",";
    json += "\"humidity\":"       + String(buffer[i].humidity, 1)    + ",";
    json += "\"water_level\":"    + String(buffer[i].waterLevel)     + ",";
    json += "\"alert_status\":\"" + String(buffer[i].alertOn ? "ON" : "OFF") + "\",";
    json += "\"led_status\":\""   + String(buffer[i].alertOn ? "ON" : "OFF") + "\"";
    json += "}";
  }
  json += "]";

  Serial.println(json);

  int httpCode = http.POST(json);

  if (httpCode > 0) {
    Serial.print("Response: "); Serial.print(httpCode);
    Serial.print(" — "); Serial.println(http.getString());
    // Only clear buffer on success
    bufferCount = 0;
  } else {
    Serial.print("SEND FAILED: "); Serial.println(http.errorToString(httpCode));
    Serial.println("Data kept in buffer — will retry next cycle.");
  }

  http.end();
  Serial.println();
}
