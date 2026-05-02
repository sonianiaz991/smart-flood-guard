-- ============================================
--  Flood Monitoring System — Database Schema
-- ============================================
--  PostgreSQL (Neon) table for storing sensor data
--  sent from ESP32 via FastAPI backend.
-- ============================================

CREATE TABLE IF NOT EXISTS sensor_data (
    id            SERIAL PRIMARY KEY,
    temperature   FLOAT        NOT NULL,
    humidity      FLOAT        NOT NULL,
    water_level   INTEGER      NOT NULL,
    alert_status  VARCHAR(3)   NOT NULL CHECK (alert_status IN ('ON', 'OFF')),
    led_status    VARCHAR(3)   NOT NULL CHECK (led_status IN ('ON', 'OFF')),
    timestamp     TIMESTAMP    NOT NULL DEFAULT NOW()
);

-- Index on timestamp for faster queries by time range
CREATE INDEX IF NOT EXISTS idx_sensor_data_timestamp ON sensor_data (timestamp DESC);
