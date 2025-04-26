#include <WiFi.h>
#include <PubSubClient.h>
#include <HCSR04.h>

// WiFi Credentials
const char* ssid = "Aquamarine_B_165_2.4G";
const char* password = "aquamarineb1964";

// MQTT Broker
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* topic = "traffic/density";

// Initialize sensors
UltraSonicDistanceSensor sensor1(5, 18);
UltraSonicDistanceSensor sensor2(19, 21);
UltraSonicDistanceSensor sensor3(22, 23);

// Error handling constant
const float SENSOR_ERROR = -1.0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Connect to MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client_Publisher")) {
      Serial.println("Connected to MQTT!");
    } else {
      Serial.println("Failed... retrying");
      delay(2000);
    }
  }
}

// Function to handle sensor reading with error detection
float getSensorReading(UltraSonicDistanceSensor& sensor) {
  float distance = sensor.measureDistanceCm();
  
  // Handle error value (-1)
  if (distance == SENSOR_ERROR) {
    Serial.println("Sensor error detected! Using previous valid reading or default value.");
    return 100.0; // Default to a high value (no obstacle) when error occurs
  }
  
  return distance;
}

void loop() {
  // Maintain MQTT connection
  if (!client.connected()) {
    client.connect("ESP32Client_Publisher");
  }
  client.loop();  // Keep the MQTT connection alive

  // Read distance from all sensors with error handling
  float d1 = getSensorReading(sensor1);
  float d2 = getSensorReading(sensor2);
  float d3 = getSensorReading(sensor3);

  // Debug: print sensor readings
  Serial.print("Sensor readings: ");
  Serial.print(d1); Serial.print(" cm, ");
  Serial.print(d2); Serial.print(" cm, ");
  Serial.print(d3); Serial.println(" cm");

  // Determine traffic density
  int blockedPairs = 0;
  if (d1 < 15 && d2 > 15 && d3 > 15) blockedPairs = 1;   // Row 1
  else if (d1 < 15 && d2 < 15 && d3 > 15) blockedPairs = 2;   // Row 2
  else if (d1 < 15 && d2 < 15 && d3 < 15) blockedPairs = 3;   // Row 3

  String trafficLevel;
  if (blockedPairs <= 1) {
    trafficLevel = "Low";
  } else if (blockedPairs == 2) {
    trafficLevel = "Medium";
  } else {
    trafficLevel = "High";
  }

  Serial.println("Traffic Level: " + trafficLevel);
  client.publish(topic, trafficLevel.c_str());
  
  delay(3000);  // Publish every 3 seconds
}