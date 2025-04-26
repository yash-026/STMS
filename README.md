# Smart Traffic Management System

## Overview
The Smart Traffic Management System is an IoT-based solution that collects real-time traffic data using multiple sensors and manages traffic flow intelligently. The system uses ESP32 microcontrollers to publish sensor data via MQTT protocol, which is then processed by subscribers to make intelligent traffic management decisions.

## Components

### Hardware
- ESP32 microcontrollers (publishers and subscribers)
- Ultrasonic sensors (vehicle detection)
- Camera modules (visual traffic monitoring)
- RFID modules (vehicle identification)
- Smoke detector modules (air quality monitoring)
- Traffic light controllers

### Software
- MQTT broker (Mosquitto recommended)
- ESP32 Arduino firmware
- Data processing application (subscriber)

## System Architecture

```
[Sensors] → [ESP32 Publishers] → [MQTT Broker] → [Subscribers (Computer/ESP32)]
                                                        ↓
                                              [Traffic Management Actions]
```

## Features
- Real-time traffic density monitoring
- Vehicle classification and counting
- Authorized vehicle identification
- Emergency vehicle priority routing
- Air quality and hazard detection
- Adaptive traffic light control
- Congestion detection and mitigation

## Installation

### Prerequisites
- Arduino IDE with ESP32 board support
- MQTT broker setup
- Required libraries: PubSubClient, WiFi, ESP32Cam (if using camera modules)

### Setup Instructions
1. Clone this repository
2. Configure WiFi and MQTT broker settings in `config.h`
3. Upload publisher firmware to sensor ESP32 devices
4. Upload subscriber firmware to traffic light ESP32 controllers
5. Start the MQTT broker service
6. Run the central monitoring application
7. Add .env file

### .env file
Add the following .env file and update your credentials as required
```
# Server Configuration
PORT=3000

# Database Configuration
DB_USER=postgres
DB_HOST=localhost
DB_NAME=traffic_management
DB_PASSWORD=password
DB_PORT=5432

# MQTT Configuration
MQTT_SERVER=localhost
MQTT_PORT=1883
MQTT_USERNAME=username
MQTT_PASSWORD=password
```
Before running using npm start, run source .env

## Usage
The system operates autonomously after setup. The central monitoring application provides a dashboard for traffic operators to:
- View real-time traffic conditions
- Access historical traffic data
- Override automated decisions when necessary
- Configure system parameters

## Contributing
Contributions to this project are welcome. Please follow these steps:
1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments
- Thanks to all contributors who have invested their time in developing this system
- Special thanks to the IoT community for inspiration and support
