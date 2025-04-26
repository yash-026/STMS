#include <WiFi.h>
#include <PubSubClient.h>

// WiFi Credentials
const char* ssid = "Aquamarine_B_163_2.4G";
const char* password = "aquamarineb1964";

// MQTT Broker
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* density_topic = "traffic/density";
const char* emergency_topic = "traffic/emergency";
const char* vehicle_count_topic = "vehicle_counter/counter11";  // New topic for vehicle count
const char* mqtt_client_id = "ESP32_Traffic_Controller";  // Unique client ID

// Traffic Light Pins
const int RED_PIN = 5;     // Red LED pin
const int YELLOW_PIN = 18;  // Yellow LED pin
const int GREEN_PIN = 19;   // Green LED pin

// Traffic timing durations (in ms)
const int LOW_RED_TIME = 8000;     // 8 seconds for low traffic
const int MEDIUM_RED_TIME = 4000;  // 4 seconds for medium traffic
const int HIGH_RED_TIME = 2000;    // 2 seconds for high traffic
const int YELLOW_TIME = 2000;      // Yellow light duration (2 seconds)
const int GREEN_TIME = 5000;       // Green light duration (5 seconds)
const int EMERGENCY_TIME = 10000;  // Emergency red light duration (10 seconds)

// Current traffic level and state
String currentTraffic = "Low";  // Default to low traffic
int currentVehicleCount = 0;    // Store the latest vehicle count - added this
boolean emergencyActive = false;
unsigned long emergencyStartTime = 0;
unsigned long lastReconnectAttempt = 0;
const long reconnectInterval = 5000;  // Reconnect every 5 seconds

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long previousMillis = 0;
int currentState = 0;  // 0: Red, 1: Green, 2: Yellow

void setup() {
  Serial.begin(115200);
  delay(100);  // Short delay for serial to initialize
  
  Serial.println("\n=== ESP32 Traffic Light Controller ===");
  
  // Setup LED pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  
  // Initial state - Red light on
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  Serial.println("Initial state: RED");
  
  // Connect to WiFi
  setupWiFi();

  // Setup MQTT client
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  Serial.println("Setup complete. Beginning normal operation.");
}

void setupWiFi() {
  Serial.println("Connecting to WiFi network: " + String(ssid));
  
  // Disconnect if already connected
  WiFi.disconnect();
  delay(100);
  
  WiFi.begin(ssid, password);
  
  int connectionAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && connectionAttempts < 20) {
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed after multiple attempts.");
    // Will retry in loop()
  }
}

boolean connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;  // Can't connect to MQTT without WiFi
  }
  
  Serial.println("Attempting MQTT connection...");
  
  // Create a random client ID
  String clientId = mqtt_client_id;
  clientId += "-";
  clientId += String(random(0xffff), HEX);
  
  // Attempt to connect
  if (client.connect(clientId.c_str())) {
    Serial.println("Connected to MQTT broker!");
    
    // Subscribe to topics
    boolean subDensity = client.subscribe(density_topic);
    boolean subEmergency = client.subscribe(emergency_topic);
    boolean subVehicleCount = client.subscribe(vehicle_count_topic);  // Added subscription to vehicle count
    
    if (subDensity && subEmergency && subVehicleCount) {
      Serial.println("Successfully subscribed to all topics");
    } else {
      Serial.println("Failed to subscribe to some topics");
      if (!subDensity) Serial.println("- Failed to subscribe to density topic");
      if (!subEmergency) Serial.println("- Failed to subscribe to emergency topic");
      if (!subVehicleCount) Serial.println("- Failed to subscribe to vehicle count topic");
    }
    
    // Publish connection status
    client.publish("traffic/status", "ESP32 Traffic Controller Online");
    return true;
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.print(client.state());
    Serial.println(" will try again later");
    return false;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Create a properly terminated string from the payload
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';  // Null-terminate the string
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  // Debug: print each character's ASCII value
  Serial.print("ASCII values: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((int)payload[i]);
    Serial.print(" ");
  }
  Serial.println();
  
  // Handle traffic density updates
  if (strcmp(topic, density_topic) == 0) {
    // Case-insensitive comparison
    String msg = String(message);
    msg.toLowerCase();
    
    if (msg.indexOf("low") != -1) {
      currentTraffic = "Low";
      Serial.println("Traffic level set to: Low");
    } else if (msg.indexOf("medium") != -1) {
      currentTraffic = "Medium";
      Serial.println("Traffic level set to: Medium");
    } else if (msg.indexOf("high") != -1) {
      currentTraffic = "High";
      Serial.println("Traffic level set to: High");
    } else {
      Serial.println("Unknown traffic level received. Staying at current level: " + currentTraffic);
    }
  }
  
  // Handle emergency messages
  else if (strcmp(topic, emergency_topic) == 0) {
    String msg = String(message);
    msg.toUpperCase();
    
    if (msg.indexOf("EMERGENCY") != -1) {
      Serial.println("!!! EMERGENCY VEHICLE DETECTED - ACTIVATING RED LIGHT !!!");
      emergencyActive = true;
      emergencyStartTime = millis();
    } else if (msg.indexOf("NORMAL") != -1 || msg.indexOf("CLEAR") != -1) {
      emergencyActive = false;
      Serial.println("Emergency state cleared");
    }
  }
  
  // Handle vehicle count messages - new handler
  else if (strcmp(topic, vehicle_count_topic) == 0) {
    // Convert the payload to an integer vehicle count
    int newCount = atoi(message);
    
    // Update the vehicle count
    currentVehicleCount = newCount;
    
    // Log the updated count
    Serial.print("Current vehicle count: ");
    Serial.println(currentVehicleCount);
  }
}

void handleEmergencyMode() {
  // Force red light on for emergency
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  
  // Check if emergency time has elapsed
  if (millis() - emergencyStartTime >= EMERGENCY_TIME) {
    Serial.println("Emergency duration completed - returning to normal traffic light cycle");
    emergencyActive = false;
    previousMillis = millis();  // Reset the normal operation timer
  }
}

void updateTrafficLight() {
  unsigned long currentMillis = millis();
  int redDuration;
  
  // Determine red light duration based on traffic level
  if (currentTraffic == "Low") {
    redDuration = LOW_RED_TIME;
  } else if (currentTraffic == "Medium") {
    redDuration = MEDIUM_RED_TIME;
  } else {  // High traffic
    redDuration = HIGH_RED_TIME;
  }
  
  // Traffic light state machine
  switch (currentState) {
    case 0:  // Red light
      digitalWrite(RED_PIN, HIGH);
      digitalWrite(YELLOW_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      
      if (currentMillis - previousMillis >= redDuration) {
        previousMillis = currentMillis;
        currentState = 1;  // Change to green
        Serial.println("Changing to GREEN");
      }
      break;
      
    case 1:  // Green light
      digitalWrite(RED_PIN, LOW);
      digitalWrite(YELLOW_PIN, LOW);
      digitalWrite(GREEN_PIN, HIGH);
      
      if (currentMillis - previousMillis >= GREEN_TIME) {
        previousMillis = currentMillis;
        currentState = 2;  // Change to yellow
        Serial.println("Changing to YELLOW");
      }
      break;
      
    case 2:  // Yellow light
      digitalWrite(RED_PIN, LOW);
      digitalWrite(YELLOW_PIN, HIGH);
      digitalWrite(GREEN_PIN, LOW);
      
      if (currentMillis - previousMillis >= YELLOW_TIME) {
        previousMillis = currentMillis;
        currentState = 0;  // Change to red
        Serial.println("Changing to RED for " + currentTraffic + " traffic (" + String(redDuration/1000) + "s) - Vehicle count: " + String(currentVehicleCount));
      }
      break;
  }
}

void checkConnections() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    setupWiFi();
  }
  
  // Check MQTT connection
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (connectMQTT()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    client.loop();
  }
}

void loop() {
  // Check and maintain connections
  checkConnections();
  
  // Priority handling: Emergency mode overrides normal operation
  if (emergencyActive) {
    handleEmergencyMode();
  } else {
    // Normal traffic light operation
    updateTrafficLight();
  }
}
