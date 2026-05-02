"""
============================================
 Database Connection — PostgreSQL (Neon)
============================================
 Uses a connection pool to avoid opening a new
 connection on every request (Neon cold starts = slow).
============================================
"""

import psycopg2
from psycopg2 import pool

# Neon PostgreSQL connection string
DATABASE_URL = (
    "postgresql://neondb_owner:npg_NPl1savUnR6V@"
    "ep-holy-cake-a46rb8oc-pooler.us-east-1.aws.neon.tech/"
    "neondb?sslmode=require"
)

# Connection pool — reuses connections instead of opening new ones each time
# min 1 connection, max 5 connections
db_pool = None


def init_db():
    """Create connection pool and sensor_data table."""
    global db_pool
    db_pool = pool.SimpleConnectionPool(1, 5, DATABASE_URL)

    conn = db_pool.getconn()
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS sensor_data (
            id            SERIAL PRIMARY KEY,
            temperature   FLOAT        NOT NULL,
            humidity      FLOAT        NOT NULL,
            water_level   INTEGER      NOT NULL,
            alert_status  VARCHAR(3)   NOT NULL,
            led_status    VARCHAR(3)   NOT NULL,
            timestamp     TIMESTAMP    NOT NULL DEFAULT NOW()
        );
    """)
    conn.commit()
    cur.close()
    db_pool.putconn(conn)
    print("Database initialized — connection pool ready.")


def get_connection():
    """Get a connection from the pool."""
    return db_pool.getconn()


def return_connection(conn):
    """Return a connection back to the pool."""
    db_pool.putconn(conn)
