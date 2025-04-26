#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <MQUnifiedsensor.h>

// RFID pins
#define RST_PIN         27
#define SS_PIN          5
#define timeSeconds     5

// MQ2 sensor parameters
#define BOARD "ESP-32"
#define PIN_ANALOG 34  // Analog pin connected to the MQ2 sensor
#define TYPE "MQ-2"    // MQ2 sensor type
#define VOLTAGE_RESOLUTION 3.3 // ESP32 ADC voltage resolution
#define ADC_BIT_RESOLUTION 12  // ESP32 ADC bit resolution (12 bits)
#define RATIO_MQ2_CLEANAIR 9.83 // RS / R0 = 9.83 in clean air for MQ2
#define SMOKE_THRESHOLD 0.10 // ppm

// WiFi Credentials
const char* ssid = "Aquamarine_B_165_2.4G";
const char* password = "aquamarineb1964";

// MQTT Broker
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* emergency_topic = "traffic/emergency";
const char* smoke_topic = "sensor/smoke";

// LED pins
const int led = 2;
const int statusLed = 26;

// RFID instance
MFRC522 mfrc522(SS_PIN, RST_PIN);

// MQ2 sensor instance
MQUnifiedsensor MQ2(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, PIN_ANALOG, TYPE);

// Interrupt and state variables
const int rfidinterrupt = 13;
unsigned long lastTrigger = 0;
boolean startTimer = false;
boolean tagdetection = false;
boolean test = false;

// Timing variables for MQ2 sensor readings
unsigned long lastMQ2Reading = 0;
const unsigned long mq2ReadInterval = 2000; // Read MQ2 every 2 seconds

// WiFi and MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Interrupt handler - sets the LED high and starts the timer
void IRAM_ATTR rfidtagdetection() {
  digitalWrite(led, HIGH);
  Serial.println("TURN EMERGENCY SIGNAL ON..........");
  test = !test;
  startTimer = true;
  lastTrigger = millis();
  tagdetection = true;
  Serial.println(test);
}

void setup() {
  Serial.begin(115200);
  
  // Setup pins
  pinMode(rfidinterrupt, INPUT);
  attachInterrupt(digitalPinToInterrupt(rfidinterrupt), rfidtagdetection, CHANGE);
  pinMode(led, OUTPUT);
  pinMode(statusLed, OUTPUT);
  digitalWrite(led, LOW);
  digitalWrite(statusLed, LOW);
  
  // Initialize SPI and RFID
  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();
  Serial.println(F("RFID Emergency Vehicle Publisher"));
  Serial.println(F("Scan RFID card to trigger emergency signal"));
  
  // Initialize MQ2 sensor
  Serial.println("Initializing MQ2 smoke sensor...");
  MQ2.init();
  
  // Calibrate MQ2 sensor in clean air
  Serial.println("Calibrating MQ2 sensor...");
  Serial.println("Please make sure the sensor is in clean air.");
  float calcR0 = 0;
  
  for(int i = 1; i <= 10; i++) {
    MQ2.update();
    calcR0 += MQ2.calibrate(RATIO_MQ2_CLEANAIR);
    Serial.print(".");
    delay(1000);
  }
  
  MQ2.setR0(calcR0/10);
  Serial.println("\nCalibration completed.");
  Serial.print("R0 = ");
  Serial.println(calcR0/10);
  
  if(isinf(calcR0)) {
    Serial.println("Warning: Connection issue, R0 is infinite (Open circuit detected)");
    while(1);
  }
  if(calcR0 == 0) {
    Serial.println("Warning: Connection issue, R0 is zero (Analog pin short circuit to ground)");
    while(1);
  }
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  connectMQTT();
  
  Serial.println("System ready. Monitoring for RFID tags and smoke...");
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect("ESP32_Sensor_Publisher")) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 2 seconds...");
      delay(2000);
    }
  }
}

void loop() {
  // Ensure MQTT connection
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
  
  // Handle LED timer for RFID
  if (startTimer && (millis() - lastTrigger > (timeSeconds * 1000))) {
    Serial.println("LED timer expired - turning off LED");
    digitalWrite(led, LOW);
    startTimer = false;
    tagdetection = false;
    
    // Publish normal state when emergency timer expires
    if (client.connected()) {
      client.publish(emergency_topic, "NORMAL");
      Serial.println("Published: NORMAL to topic: " + String(emergency_topic));
    }
  }
  
  digitalWrite(statusLed, test);
  
  // Check for MQ2 sensor readings at the defined interval
  if (millis() - lastMQ2Reading >= mq2ReadInterval) {
    lastMQ2Reading = millis();
    
    // Read MQ2 sensor
    MQ2.update();
    MQ2.setA(36.753); 
    MQ2.setB(-3.109);
    float smoke = MQ2.readSensor();
    
    // Print the smoke value
    Serial.print("Smoke: ");
    Serial.print(smoke);
    Serial.println(" ppm");
    
    // Publish smoke reading to MQTT
    if (client.connected()) {
      char smokeStr[10];
      dtostrf(smoke, 4, 2, smokeStr);
      client.publish(smoke_topic, smokeStr);
      Serial.println("Published smoke reading: " + String(smokeStr) + " to topic: " + String(smoke_topic));
    }
    
    // Check if smoke level is above threshold
    if (smoke > SMOKE_THRESHOLD) {
      Serial.println("WARNING: Smoke detected!");
      digitalWrite(led, HIGH); // Turn LED on
      
      // Publish smoke alert to MQTT
      if (client.connected()) {
        client.publish(smoke_topic, "ALERT: Smoke detected");
        Serial.println("Published smoke alert to topic: " + String(smoke_topic));
      }
    } else if (!startTimer) {
      // Only turn off LED if not triggered by RFID
      digitalWrite(led, LOW); // Turn LED off
    }
  }
  
  // RFID card detection
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Show UID on serial monitor
    Serial.print("UID tag: ");
    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println();
    content.toUpperCase();
    
    // Publish RFID tag to MQTT
    if (client.connected()) {
      String rfidMsg = "RFID Tag: " + content.substring(1);
      client.publish(emergency_topic, rfidMsg.c_str());
    }
    
    if (content.substring(1) == "FC F8 C8 01") {
      Serial.println("Authorized emergency vehicle");
      digitalWrite(statusLed, HIGH);
      
      // Publish emergency signal to MQTT
      if (client.connected()) {
        client.publish(emergency_topic, "EMERGENCY");
        Serial.println("Published: EMERGENCY to topic: " + String(emergency_topic));
      }
      
      // Only trigger emergency function if not already active
      if (!tagdetection) {
        Serial.println("Emergency RFID card detected");
        rfidtagdetection(); // Trigger the interrupt function
      }
    } else {
      Serial.println("Unknown RFID tag - access denied");
      delay(1000);
    }
  }
}