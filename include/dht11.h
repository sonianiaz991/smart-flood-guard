/*
 * ============================================
 *  DHT11 Sensor Library (Simple Functions)
 *  No classes — just plain functions
 * ============================================
 *
 *  Wiring:
 *    DHT11 VCC  → 3.3V
 *    DHT11 DATA → GPIO 4 (default, change below)
 *    DHT11 GND  → GND
 *
 *  Requires: DHT sensor library by Adafruit
 *            Adafruit Unified Sensor library
 * ============================================
 */

#ifndef DHT11_H
#define DHT11_H

#include <DHT.h>

// ---------- Pin & Sensor Config ----------
#define DHT_PIN   4        // GPIO pin connected to DHT11 data pin
#define DHT_TYPE  DHT11    // Sensor type

// ---------- Global DHT object ----------
DHT dht(DHT_PIN, DHT_TYPE);

// ---------- Track min/max over time ----------
float minTemperature = 999.0;
float maxTemperature = -999.0;
float minHumidity    = 999.0;
float maxHumidity    = -999.0;

/* Initialize the DHT11 sensor — call once in setup() */
void dht11_init() {
  dht.begin();
}

/* Read temperature in Celsius. Returns NaN on failure. */
float read_temperature() {
  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    if (temp < minTemperature) minTemperature = temp;
    if (temp > maxTemperature) maxTemperature = temp;
  }
  return temp;
}

/* Read humidity in %. Returns NaN on failure. */
float read_humidity() {
  float hum = dht.readHumidity();
  if (!isnan(hum)) {
    if (hum < minHumidity) minHumidity = hum;
    if (hum > maxHumidity) maxHumidity = hum;
  }
  return hum;
}

/* Get the minimum temperature recorded so far */
float get_min_temperature() {
  return minTemperature;
}

/* Get the maximum temperature recorded so far */
float get_max_temperature() {
  return maxTemperature;
}

/* Get the minimum humidity recorded so far */
float get_min_humidity() {
  return minHumidity;
}

/* Get the maximum humidity recorded so far */
float get_max_humidity() {
  return maxHumidity;
}

/* Compute heat index in Celsius (feels-like temperature) */
float get_heat_index(float temp, float hum) {
  if (isnan(temp) || isnan(hum)) return NAN;
  return dht.computeHeatIndex(temp, hum, false); // false = Celsius
}

/* Print a quick sensor summary to Serial */
void print_dht_summary(float temp, float hum) {
  Serial.println("--- DHT11 Reading ---");
  Serial.print("Temperature : "); Serial.print(temp); Serial.println(" C");
  Serial.print("Humidity    : "); Serial.print(hum);  Serial.println(" %");
  Serial.print("Heat Index  : "); Serial.print(get_heat_index(temp, hum)); Serial.println(" C");
  Serial.print("Temp Min/Max: "); Serial.print(minTemperature); Serial.print(" / "); Serial.println(maxTemperature);
  Serial.print("Hum  Min/Max: "); Serial.print(minHumidity);    Serial.print(" / "); Serial.println(maxHumidity);
  Serial.println("---------------------");
}

#endif
