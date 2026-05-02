"""
============================================
 Smart Flood Guard — FastAPI Backend
============================================
 Receives sensor data from ESP32, stores it
 in PostgreSQL, and serves the web dashboard.

 Run:
   pip install fastapi uvicorn psycopg2-binary jinja2
   uvicorn main:app --host 0.0.0.0 --port 8000
============================================
"""

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel
from typing import List
from db import get_connection, return_connection, init_db

# ---------- FastAPI App ----------
app = FastAPI(title="Smart Flood Guard API")

# ---------- Jinja2 Templates ----------
templates = Jinja2Templates(directory="templates")


# ---------- Request Body Schema ----------
class SensorData(BaseModel):
    temperature: float
    humidity: float
    water_level: int
    alert_status: str   # "ON" or "OFF"
    led_status: str     # "ON" or "OFF"


# ---------- Initialize DB on startup ----------
@app.on_event("startup")
def startup():
    init_db()


# =============================================
#  PAGE ROUTES — Serve HTML templates
# =============================================

@app.get("/", response_class=HTMLResponse)
def page_dashboard(request: Request):
    """Serve the main dashboard page."""
    return templates.TemplateResponse("dashboard.html", {"request": request})


@app.get("/about", response_class=HTMLResponse)
def page_about(request: Request):
    """Serve the about page."""
    return templates.TemplateResponse("about.html", {"request": request})


@app.get("/base", response_class=HTMLResponse)
def page_base(request: Request):
    """Serve the base page."""
    return templates.TemplateResponse("base.html", {"request": request})

# =============================================
#  DATA API — ESP32 sends data here
# =============================================

@app.post("/api/data")
def receive_data(data: SensorData):
    """Receive a single sensor reading from ESP32."""
    try:
        conn = get_connection()
        cur = conn.cursor()
        cur.execute(
            """
            INSERT INTO sensor_data (temperature, humidity, water_level, alert_status, led_status)
            VALUES (%s, %s, %s, %s, %s)
            RETURNING id, timestamp;
            """,
            (data.temperature, data.humidity, data.water_level, data.alert_status, data.led_status)
        )
        row = cur.fetchone()
        conn.commit()
        cur.close()
        return_connection(conn)

        return {"status": "success", "id": row[0], "timestamp": str(row[1])}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.post("/api/data/batch")
def receive_batch(readings: List[SensorData]):
    """Receive a batch of sensor readings from ESP32."""
    if not readings:
        raise HTTPException(status_code=400, detail="Empty batch")

    try:
        conn = get_connection()
        cur = conn.cursor()

        values = []
        params = []
        for r in readings:
            values.append("(%s, %s, %s, %s, %s)")
            params.extend([r.temperature, r.humidity, r.water_level, r.alert_status, r.led_status])

        query = (
            "INSERT INTO sensor_data (temperature, humidity, water_level, alert_status, led_status) "
            "VALUES " + ", ".join(values) + " RETURNING id;"
        )
        cur.execute(query, params)
        ids = [row[0] for row in cur.fetchall()]
        conn.commit()
        cur.close()
        return_connection(conn)

        return {"status": "success", "inserted": len(ids), "ids": ids}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


# =============================================
#  READ API — Frontend fetches data here
# =============================================

@app.get("/api/latest")
def get_latest():
    """Get the most recent sensor reading."""
    try:
        conn = get_connection()
        cur = conn.cursor()
        cur.execute("""
            SELECT id, temperature, humidity, water_level, alert_status, led_status, timestamp
            FROM sensor_data ORDER BY timestamp DESC LIMIT 1;
        """)
        row = cur.fetchone()
        cur.close()
        return_connection(conn)

        if not row:
            return {"data": None}

        return {"data": {
            "id": row[0], "temperature": row[1], "humidity": row[2],
            "water_level": row[3], "alert_status": row[4], "led_status": row[5],
            "timestamp": str(row[6])
        }}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.get("/api/history")
def get_history(limit: int = 50):
    """Get recent sensor readings for charts and table. Max 200."""
    limit = min(limit, 200)
    try:
        conn = get_connection()
        cur = conn.cursor()
        cur.execute("""
            SELECT id, temperature, humidity, water_level, alert_status, led_status, timestamp
            FROM sensor_data ORDER BY timestamp DESC LIMIT %s;
        """, (limit,))
        rows = cur.fetchall()
        cur.close()
        return_connection(conn)

        data = []
        for r in rows:
            data.append({
                "id": r[0], "temperature": r[1], "humidity": r[2],
                "water_level": r[3], "alert_status": r[4], "led_status": r[5],
                "timestamp": str(r[6])
            })

        # Return in chronological order (oldest first) for charts
        data.reverse()
        return {"data": data, "count": len(data)}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.get("/api/stats")
def get_stats():
    """Get summary statistics from all sensor data."""
    try:
        conn = get_connection()
        cur = conn.cursor()
        cur.execute("""
            SELECT
                COUNT(*) as total_readings,
                ROUND(AVG(temperature)::numeric, 1) as avg_temp,
                ROUND(MIN(temperature)::numeric, 1) as min_temp,
                ROUND(MAX(temperature)::numeric, 1) as max_temp,
                ROUND(AVG(humidity)::numeric, 1) as avg_hum,
                ROUND(MIN(humidity)::numeric, 1) as min_hum,
                ROUND(MAX(humidity)::numeric, 1) as max_hum,
                ROUND(AVG(water_level)::numeric, 0) as avg_water,
                MIN(water_level) as min_water,
                MAX(water_level) as max_water,
                SUM(CASE WHEN alert_status = 'ON' THEN 1 ELSE 0 END) as total_alerts
            FROM sensor_data;
        """)
        row = cur.fetchone()
        cur.close()
        return_connection(conn)

        if not row or row[0] == 0:
            return {"data": None}

        return {"data": {
            "total_readings": row[0],
            "avg_temp": float(row[1]), "min_temp": float(row[2]), "max_temp": float(row[3]),
            "avg_hum": float(row[4]), "min_hum": float(row[5]), "max_hum": float(row[6]),
            "avg_water": float(row[7]), "min_water": row[8], "max_water": row[9],
            "total_alerts": row[10]
        }}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.get("/api/alerts")
def get_alerts(limit: int = 20):
    """Get recent alert events (where alert_status = ON)."""
    limit = min(limit, 100)
    try:
        conn = get_connection()
        cur = conn.cursor()
        cur.execute("""
            SELECT id, temperature, humidity, water_level, alert_status, led_status, timestamp
            FROM sensor_data WHERE alert_status = 'ON'
            ORDER BY timestamp DESC LIMIT %s;
        """, (limit,))
        rows = cur.fetchall()
        cur.close()
        return_connection(conn)

        data = []
        for r in rows:
            data.append({
                "id": r[0], "temperature": r[1], "humidity": r[2],
                "water_level": r[3], "alert_status": r[4], "led_status": r[5],
                "timestamp": str(r[6])
            })

        return {"data": data, "count": len(data)}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


# ---------- Keep old routes working for ESP backward compatibility ----------
@app.post("/data")
def receive_data_legacy(data: SensorData):
    return receive_data(data)

@app.post("/data/batch")
def receive_batch_legacy(readings: List[SensorData]):
    return receive_batch(readings)
