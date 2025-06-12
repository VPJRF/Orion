#include <Arduino.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Arduino_LSM6DS3.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobot_DF1201S.h>
#include "actuators.h"

DFRobot_DF1201S DF1201S; // Only here!

// LOCATIE = bed/kamer/toilet ----------------
// const char* deviceName = "bed"; //S-COM5
// const char* deviceName = "kamer"; //C-COM4
// const char* deviceName = "toilet"; //O-COM3
const char* deviceName = DEVICE_NAME;
//--------------------------------------------

const char* ssid = "iPhone";
const char* password = "Lindes_hotsp0t";
const char* mqtt_server = "172.20.10.13";
const int mqtt_port = 1883;

const int PIR_PIN = 5;
const int LED_RING_PIN = 6;
const int LED_RING_COUNT = 24;
const float gyroThreshold = 4.0; // Lower threshold for higher sensitivity

float last_gx = 0, last_gy = 0, last_gz = 0;
float last_ax = 0, last_ay = 0, last_az = 0;
bool lastLEDState = false;
bool message = false; // BOX state
bool feedback = false;

Adafruit_NeoPixel ring(LED_RING_COUNT, LED_RING_PIN, NEO_GRB + NEO_KHZ800);

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setBoxState(bool state) {
  message = state;
  // Add any hardware actions here if needed (e.g., digitalWrite to a pin)
}

// WiFi connection
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
}

// MQTT connection and subscription
void connectToMQTT() {
  static bool wasConnected = false;
  while (!client.connected()) {
    if (!wasConnected) {
      Serial.print("Connecting to MQTT...");
    }
    if (client.connect(deviceName)) {
      if (!wasConnected) {
        Serial.println(" connected.");
      }
      String commandTopic = "/commands/" + String(deviceName);
      client.subscribe(commandTopic.c_str());
      wasConnected = true;
    } else {
      if (!wasConnected) {
        Serial.print(" failed, rc=");
        Serial.print(client.state());
        Serial.println();
      }
      delay(1000);
    }
  }
  wasConnected = true;
}

// Publish sensor and LED state as JSON to /orion/states/<deviceName>
void sendMQTT(bool pir, bool imu, bool led, bool message, bool feedback) {
  StaticJsonDocument<256> doc;
  doc["device"] = deviceName;
  doc["PIR"] = pir;
  doc["IMU"] = imu;
  doc["LED"] = led;
  doc["BOX"] = message;
  doc["FBK"] = feedback;

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  String stateTopic = "/orion/states/" + String(deviceName);
  client.publish(stateTopic.c_str(), buffer, n);
}

// Handle incoming MQTT commands
void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("JSON deserialization failed: ");
    Serial.println(error.c_str());
    return;
  }

  Serial.print("MQTT command received on topic: ");
  Serial.println(topic);
  serializeJson(doc, Serial);
  Serial.println();

  handleActuatorCommand(doc, ring, LED_RING_COUNT);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);

  ring.begin();
  ring.setBrightness(50);
  ring.show();

  connectToWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  if (!IMU.begin()) {
    Serial.println("IMU init failed!");
    while (1);
  }

  // Only initialize DFPlayer if this is the toilet device
  if (deviceName && strcmp(deviceName, "toilet") == 0) {
    Serial1.begin(115200);
    while (!DF1201S.begin(Serial1)) {
      Serial.println("DF1201S init failed, check wiring!");
      delay(1000);
    }
    DF1201S.switchFunction(DF1201S.MUSIC);
    delay(2000);
    DF1201S.setVol(15);
    DF1201S.setPlayMode(DF1201S.SINGLE);
    Serial.println("DFPlayer PRO online.");
  }

  connectToMQTT();
  Serial.println("Setup complete.");
}

static float ref_gx = 0, ref_gy = 0, ref_gz = 0;
static unsigned long lastRefUpdate = 0;

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  // Only update one LED effect at a time!
  if (feedbackActive) {
    updateFeedbackAnimation(ring, LED_RING_COUNT);
  } else {
    updateLEDFade(ring, LED_RING_COUNT);
  }

  // Read PIR sensor
  bool pirState = digitalRead(PIR_PIN);

  // Read gyroscope and check for significant movement over 500ms
  float gx = 0, gy = 0, gz = 0;
  bool imuTriggered = false;
  static bool firstGyroRead = true;

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gx, gy, gz);

    // Update reference gyro values every 500 ms
    if (firstGyroRead || millis() - lastRefUpdate >= 500) {
      ref_gx = gx;
      ref_gy = gy;
      ref_gz = gz;
      lastRefUpdate = millis();
      firstGyroRead = false;
    }

    // Compare current reading to reference from 500ms ago
    if (!firstGyroRead) {
      if (abs(gx - ref_gx) > gyroThreshold ||
          abs(gy - ref_gy) > gyroThreshold ||
          abs(gz - ref_gz) > gyroThreshold) {
        imuTriggered = true;
      }
    }
  }

  // Only send MQTT and print every 2000 ms
  static unsigned long lastSend = 0;
  if (millis() - lastSend >= 2000) {
    // Print all sensor and state values
    Serial.print(deviceName);
    Serial.print(" | PIR: ");
    Serial.print(pirState);
    Serial.print(" | IMU: ");
    Serial.print(imuTriggered);
    Serial.print(" | LED: ");
    Serial.print(lastLEDState);
    Serial.print(" | BOX: ");
    Serial.print(message);
    Serial.print(" | FBK: ");
    Serial.println(feedback);

    // Print raw gyro values every 2 seconds
    // Serial.print("Gyro: gx="); Serial.print(gx);
    // Serial.print(" gy="); Serial.print(gy);
    // Serial.print(" gz="); Serial.println(gz);

    sendMQTT(pirState, imuTriggered, lastLEDState, message, feedback);
    lastSend = millis();
  }

  delay(10); // Fast loop for smooth fades
}