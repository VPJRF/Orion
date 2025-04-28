#include <Arduino.h>
#include <WiFiNINA.h>
#include <ArduinoBLE.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobot_DF1201S.h>
#include "light.h"
#include "sound.h"
#include "scene.h"
#include <map>

// BLE service and characteristics
BLEService testService("0000180c-0000-1000-8000-00805f9b34fb");
BLEStringCharacteristic ssidCharacteristic("00002a57-0000-1000-8000-00805f9b34fb", BLEWrite, 128);
BLEStringCharacteristic passwordCharacteristic("00002a58-0000-1000-8000-00805f9b34fb", BLEWrite, 128);
BLEStringCharacteristic logCharacteristic("00002a59-0000-1000-8000-00805f9b34fb", BLERead | BLENotify, 512);

// Wi-Fi server
WiFiServer server(80);
bool wifiConnected = false;

// NeoPixel setup
#define LED_PIN 6
#define NUM_LEDS 24
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// DFRobot DF1201S setup
DFRobot_DF1201S DF1201S;
#define DF1201S_SERIAL Serial1
#define DF1201S_BAUD 115200
bool df1201sInitialized = false;

// === Global variables for test interaction model ===
String currentTest = "";
int currentStep = 0;
String testLog = "";
std::map<String, String> testResults;

String getTestLabel(const String& test) {
  if (test == "A1") return "Intensity";
  if (test == "A2") return "Color";
  if (test == "A3") return "Focus";
  if (test == "A4") return "Time";
  if (test == "A5") return "Alarm";
  if (test == "B1") return "Volume";
  if (test == "B2") return "Speed";
  if (test == "B3") return "Voice";
  if (test == "B4") return "Message";
  return test;
}

String getTestValue(const String& test, int step) {
  if (test == "A1") {
    // Only reference values from light.cpp
    extern const int intensitiesA1[5];
    return String(intensitiesA1[step % 5]) + "/255";
  }
  if (test == "A2") {
    extern const char* colorTempsA2[5];
    return String(colorTempsA2[step % 5]);
  }
  if (test == "A3") {
    extern const int focusModesA3[5];
    return String(focusModesA3[step % 5]) + "/24 leds";
  }
  if (test == "A4") {
    extern const char* timeModesA4[5];
    return String(timeModesA4[step % 5]);
  }
  if (test == "A5") {
    extern const char* alarmModesA5[5];
    return String(alarmModesA5[step % 5]);
  }
  if (test == "B1") {
    extern const int volumesB1[5];
    return String(volumesB1[step % 5]) + "/30";
  }
  if (test == "B2") {
    extern const char* speedsB2[3];
    return String(speedsB2[step % 3]);
  }
  if (test == "B3") {
    extern const char* voicesB3[5];
    return String(voicesB3[step % 5]);
  }
  if (test == "B4") {
    extern const char* messagesB4[5];
    return String(messagesB4[step % 5]);
  }
  return String(step);
}

// === Helper functions for test interaction model ===
void runCurrentStep() {
  if (currentTest == "A1") {
    lightA1(strip, currentStep);
  } else if (currentTest == "A2") {
    lightA2(strip, currentStep);
  } else if (currentTest == "A3") {
    lightA3(strip, currentStep);
  } else if (currentTest == "A4") {
    lightA4(strip, currentStep);
  } else if (currentTest == "A5") {
    lightA5(strip, currentStep);
  } else if (currentTest == "B1") {
    soundB1(currentStep);
  } else if (currentTest == "B2") {
    soundB2(currentStep);
  } else if (currentTest == "B3") {
    soundB3(currentStep);
  } else if (currentTest == "B4") {
    soundB4(currentStep);
  }
}

void startTest(const String& test) {
  currentTest = test;
  currentStep = 0;
  runCurrentStep();
}

void cycleTest() {
  currentStep++;
  runCurrentStep();
}

void setupBLE() {
  if (!BLE.begin()) {
    Serial.println("BLE failed to start");
    while (1);
  }
  BLE.setLocalName("Arduino_Orion_1");
  BLE.setAdvertisedService(testService);
  testService.addCharacteristic(ssidCharacteristic);
  testService.addCharacteristic(passwordCharacteristic);
  testService.addCharacteristic(logCharacteristic);
  BLE.addService(testService);
  BLE.advertise();
  Serial.println("BLE Advertising started");
}

void sendPlainTextResponse(WiFiClient &client, const String &body) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println(body);
}

void sendNotFoundResponse(WiFiClient &client, const String &body) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println(body);
}

void connectToWiFi(const String &ssid, const String &password) {
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);

  BLE.stopAdvertise();
  BLE.end();
  delay(1000);

  WiFi.end();
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), password.c_str());

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    server.begin();
    wifiConnected = true;
  } else {
    Serial.println("Wi-Fi connection failed. Restarting BLE...");
    setupBLE();
  }
}

void setup() {
  Serial.begin(115200);
  setupBLE();

  strip.begin();
  strip.show();

  // Initialize DF1201S sound module
  DF1201S_SERIAL.begin(DF1201S_BAUD);
  while (!DF1201S.begin(DF1201S_SERIAL)) {
    Serial.println("DF1201S init failed, check wiring!");
    delay(1000);
  }
  DF1201S.switchFunction(DF1201S.MUSIC);
  delay(2000);
  DF1201S.setVol(5);
  DF1201S.setPlayMode(DF1201S.SINGLE);
  Serial.println("DF1201S module initialized.");
}

void sendLogOverBLE() {
  logCharacteristic.writeValue(testLog);
}

void savePickedSetting() {
  String label = getTestLabel(currentTest);
  String value = getTestValue(currentTest, currentStep);

  // Overwrite or insert the result
  testResults[label] = value;

  // Rebuild the log string
  testLog = "";
  for (auto const& entry : testResults) {
    testLog += entry.first + ": " + entry.second + "\n";
  }

  currentTest = "";
  currentStep = 0;

  Serial.println("\nLogged: \n" + testLog);

  sendLogOverBLE();
}

void loop() {
  // BLE: Receive Wi-Fi credentials and handle button presses
  if (!wifiConnected) {
    BLEDevice central = BLE.central();
    if (central) {
      Serial.print("Connected to central: ");
      Serial.println(central.address());

      // Lazy init DF1201S after BLE connection
      if (!df1201sInitialized) {
        Serial.println("Initializing DF1201S...");
        DF1201S_SERIAL.begin(DF1201S_BAUD);
        while (!DF1201S.begin(DF1201S_SERIAL)) {
          Serial.println("DF1201S init failed, check wiring!");
          delay(1000);
        }
        DF1201S.switchFunction(DF1201S.MUSIC);
        delay(2000);
        DF1201S.setVol(5);
        df1201sInitialized = true;
        Serial.println("DF1201S initialized.");
      }

      String ssid = "", password = "";
      while (central.connected()) {
        // Handle button presses and SSID
        if (ssidCharacteristic.written()) {
          String value = ssidCharacteristic.value();
          Serial.print("Received BLE value: ");
          Serial.println(value);

          // If value matches a button code, handle as button press
          if (value == "cycle" || value == "pick" ||
              (value.length() == 2 && (
                value.charAt(0) == 'A' ||
                value.charAt(0) == 'B' ||
                value.charAt(0) == 'C'))) {
            if (value == "cycle") {
              if (currentTest != "") {
                cycleTest();
              } else {
                Serial.println("No test active. Ignoring cycle.");
              }
            } else if (value == "pick") {
              if (currentTest != "") {
                savePickedSetting();
                sendLogOverBLE();
              } else {
                Serial.println("No test active. Ignoring pick.");
              }
            } else {
              // Handle A/B/C button codes
              char prefix = value.charAt(0), code = value.charAt(1);
              switch (prefix) {
                case 'A':
                  switch (code) {
                    case '1': startTest("A1"); break;
                    case '2': startTest("A2"); break;
                    case '3': startTest("A3"); break;
                    case '4': startTest("A4"); break;
                    case '5': startTest("A5"); break;
                    default: startTest(value); break;
                  }
                  break;
                case 'B':
                  switch (code) {
                    case '1': startTest("B1"); break;
                    case '2': startTest("B2"); break;
                    case '3': startTest("B3"); break;
                    case '4': startTest("B4"); break;
                    default: startTest(value); break;
                  }
                  break;
                case 'C':
                  switch (code) {
                    case '1': sceneC1(); break;
                    case '2': sceneC2(); break;
                    case '3': sceneC3(); break;
                    case '4': sceneC4(); break;
                    default: startTest(value); break;
                  }
                  break;
                default:
                  startTest(value);
                  break;
              }
            }
          } else {
            // Otherwise, treat as SSID
            ssid = value;
          }
        }
        // Handle password
        if (passwordCharacteristic.written()) {
          password = passwordCharacteristic.value();
        }

        // Attempt Wi-Fi connection if both are set
        if (!ssid.isEmpty() && !password.isEmpty()) {
          Serial.println("Attempting Wi-Fi connection...");
          connectToWiFi(ssid, password);
          ssid = ""; password = "";
          break;
        }
      }
      Serial.println("Disconnected from central");
    }
  }

  // HTTP server: handle requests if Wi-Fi is connected
  if (wifiConnected) {
    WiFiClient client = server.available();
    if (client) {
      String request = client.readStringUntil('\r');
      client.flush();

      if (request.indexOf("/change_scene") != -1) {
          lightA0(strip);
          sendPlainTextResponse(client, "Scene changed successfully!");
      } else if (request.indexOf("/disconnect_wifi") != -1) {
          Serial.println("Disconnecting Wi-Fi...");
          sendPlainTextResponse(client, "Wi-Fi disconnected and BLE restarted.");
          client.stop();
          delay(500);
          WiFi.disconnect();
          wifiConnected = false;
          setupBLE();
      } else if (request.indexOf("/wifi_status") != -1) {
          sendPlainTextResponse(client, WiFi.status() == WL_CONNECTED ? "Wi-Fi connected" : "Wi-Fi not connected");
          client.stop();
      } else if (request.indexOf("/get_log") != -1) {
          sendPlainTextResponse(client, testLog);
      } else {
          sendNotFoundResponse(client, "Invalid request");
      }
      client.stop();
    }
  }
}