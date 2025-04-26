require('dotenv').config();
const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const mqtt = require('mqtt');
const { Pool } = require('pg');
const path = require('path');
const moment = require('moment');

// Initialize Express app
const app = express();
const server = http.createServer(app);
const io = socketIo(server);

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));

// Database connection
const pool = new Pool({
  user: process.env.DB_USER,
  host: process.env.DB_HOST,
  database: process.env.DB_NAME,
  password: process.env.DB_PASSWORD,
  port: process.env.DB_PORT,
});

// Ensure database connection
pool.connect((err) => {
  if (err) {
    console.error('Error connecting to the database:', err);
    process.exit(1);
  } else {
    console.log('Connected to the database');
  }
});

// Initialize database tables
async function initDatabase() {
  try {
    await pool.query(`
      CREATE TABLE IF NOT EXISTS traffic_data (
        id SERIAL PRIMARY KEY,
        intersection_id VARCHAR(50) NOT NULL,
        traffic_level VARCHAR(20) NOT NULL,
        vehicles_count INTEGER,
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
      )
    `);
    await pool.query(`
      CREATE TABLE IF NOT EXISTS priority_vehicles (
        id SERIAL PRIMARY KEY,
        is_detected BOOLEAN NOT NULL,
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
      )
    `);
    await pool.query(`
      CREATE TABLE IF NOT EXISTS alerts (
        id SERIAL PRIMARY KEY,
        intersection_id VARCHAR(50) NOT NULL,
        alert_type VARCHAR(50) NOT NULL,
        details TEXT,
        is_active BOOLEAN DEFAULT TRUE,
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
      )
    `);
    console.log('Database tables initialized');
  } catch (err) {
    console.error('Error initializing database:', err);
  }
}

// MQTT client setup
const mqttClient = mqtt.connect(`mqtt://${process.env.MQTT_SERVER}:${process.env.MQTT_PORT}`);

// MQTT subscription topics
const TOPICS = ['traffic/density', 'vehicle_counter/counter11', 'traffic/emergency', 'traffic/fire'];

// Connect to MQTT broker
mqttClient.on('connect', () => {
  console.log('Connected to MQTT broker');
  TOPICS.forEach((topic) => {
    mqttClient.subscribe(topic, (err) => {
      if (!err) {
        console.log(`Subscribed to ${topic}`);
      } else {
        console.error(`Error subscribing to ${topic}:`, err);
      }
    });
  });
});

// Latest traffic data
let latestData = {
  trafficLevel: 'Low',
  vehicleCount: 0,
  priorityVehicles: false,
  priorityTimestamp: null,
  fireAlert: false,
  smokeLevel: 0,
  lastUpdated: moment().format('YYYY-MM-DD HH:mm:ss'),
};

// Handle incoming MQTT messages
mqttClient.on('message', async (topic, message) => {
  const payload = message.toString();
  console.log(`Received message on ${topic}: ${payload}`);
  try {
    if (topic === 'traffic/density') {
      if (['Low', 'Medium', 'High'].includes(payload)) {
        latestData.trafficLevel = payload;
        await pool.query(
          'INSERT INTO traffic_data (intersection_id, traffic_level) VALUES ($1, $2)',
          ['intersection-main', payload]
        );
      } else {
        console.error('Invalid traffic density payload:', payload);
      }
    } else if (topic === 'vehicle_counter/counter11') {
      const count = parseInt(payload);
      if (!isNaN(count)) {
        latestData.vehicleCount = count;
        await pool.query(
          'UPDATE traffic_data SET vehicles_count = $1 WHERE id = (SELECT id FROM traffic_data ORDER BY timestamp DESC LIMIT 1)',
          [count]
        );
      }
    } else if (topic === 'traffic/emergency') {
      const isDetected = payload.toLowerCase() === 'true';
      latestData.priorityVehicles = isDetected;
      if (isDetected) {
        latestData.priorityTimestamp = moment().format('HH:mm:ss');
        await pool.query(
          'INSERT INTO priority_vehicles (is_detected, timestamp) VALUES ($1, $2)',
          [isDetected, latestData.priorityTimestamp]
        );
      }
    } else if (topic === 'traffic/fire') {
      const fireData = JSON.parse(payload);
      latestData.fireAlert = fireData.detected;
      latestData.smokeLevel = fireData.level;
      if (fireData.detected) {
        await pool.query(
          'INSERT INTO alerts (intersection_id, alert_type, details) VALUES ($1, $2, $3)',
          ['intersection-main', 'FIRE', `Smoke level: ${fireData.level}`]
        );
      }
    }
    latestData.lastUpdated = moment().format('YYYY-MM-DD HH:mm:ss');
    io.emit('trafficUpdate', latestData);
  } catch (err) {
    console.error('Error processing message:', err);
  }
});

// Socket.IO connection
io.on('connection', (socket) => {
  console.log('New client connected');
  socket.emit('trafficUpdate', latestData);

  socket.on('getHistoricalData', async (period) => {
    try {
      const validPeriods = ['second', 'minute', 'hour'];
      if (!validPeriods.includes(period)) {
        socket.emit('error', 'Invalid period. Valid values are: second, minute, hour.');
        return;
      }
      let timeFilter = '';
      let groupBy = '';
      switch (period) {
        case 'second':
          timeFilter = "timestamp > NOW() - INTERVAL '1 second'";
          groupBy = "date_trunc('second', timestamp)";
          break;
        case 'minute':
          timeFilter = "timestamp > NOW() - INTERVAL '1 minute'";
          groupBy = "date_trunc('minute', timestamp)";
          break;
        case 'hour':
          timeFilter = "timestamp > NOW() - INTERVAL '1 hour'";
          groupBy = "date_trunc('hour', timestamp)";
          break;
      }
      const result = await pool.query(`
        SELECT traffic_level, COUNT(*) as count, ${groupBy} as timestamp
        FROM traffic_data
        WHERE ${timeFilter}
        GROUP BY traffic_level, timestamp
        ORDER BY timestamp ASC
      `);
      socket.emit('historicalData', result.rows);
    } catch (err) {
      console.error('Error retrieving historical data:', err);
      socket.emit('error', 'Failed to retrieve historical data');
    }
  });

  socket.on('disconnect', () => {
    console.log('Client disconnected');
  });
});

// Routes
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.get('/api/status', (req, res) => {
  res.json(latestData);
});

app.get('/api/history', async (req, res) => {
  try {
    const { period = 'minute' } = req.query;
    const validPeriods = ['second', 'minute', 'hour'];
    if (!validPeriods.includes(period)) {
      return res.status(400).json({ error: 'Invalid period. Valid values are: second, minute, hour.' });
    }
    let timeFilter = '';
    let groupBy = '';
    switch (period) {
      case 'second':
        timeFilter = "timestamp > NOW() - INTERVAL '1 second'";
        groupBy = "date_trunc('second', timestamp)";
        break;
      case 'minute':
        timeFilter = "timestamp > NOW() - INTERVAL '1 minute'";
        groupBy = "date_trunc('minute', timestamp)";
        break;
      case 'hour':
        timeFilter = "timestamp > NOW() - INTERVAL '1 hour'";
        groupBy = "date_trunc('hour', timestamp)";
        break;
    }
    const result = await pool.query(`
      SELECT traffic_level, COUNT(*) as count, ${groupBy} as timestamp
      FROM traffic_data
      WHERE ${timeFilter}
      GROUP BY traffic_level, timestamp
      ORDER BY timestamp ASC
    `);
    res.json(result.rows);
  } catch (err) {
    console.error('Error retrieving historical data:', err);
    res.status(500).json({ error: 'Failed to retrieve historical data' });
  }
});

// Graceful shutdown
process.on('SIGINT', async () => {
  console.log('Shutting down server...');
  await pool.end();
  mqttClient.end();
  process.exit(0);
});

// Start the server
const PORT = process.env.PORT || 3000;
server.listen(PORT, async () => {
  console.log(`Server running on port ${PORT}`);
  await initDatabase();
});