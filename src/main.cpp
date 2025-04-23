#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

#define LED_RING_PIN 6
#define NUM_LEDS 24

BLEService potService("0000180c-0000-1000-8000-00805f9b34fb"); // Custom service UUID
BLEIntCharacteristic potCharacteristic("00002a56-0000-1000-8000-00805f9b34fb", BLERead | BLENotify); // Potentiometer characteristic
BLEByteCharacteristic buttonCharacteristic("00002a57-0000-1000-8000-00805f9b34fb", BLEWrite); // Button characteristic
const int potPin = A0; // Potentiometer connected to A0
const int ledPin = LED_BUILTIN; // Onboard LED

Adafruit_NeoPixel ledRing = Adafruit_NeoPixel(NUM_LEDS, LED_RING_PIN, NEO_GRB + NEO_KHZ800);

// Function declarations
void lightZone(int start, int end, int red, int green, int blue);
void clearRing();

void setup() {
  Serial.begin(115200);

  // Optional: Add a timeout for Serial Monitor initialization
  unsigned long startTime = millis();
  while (!Serial && millis() - startTime < 2000) {
    // Wait for Serial Monitor for up to 2 seconds
  }

  if (!BLE.begin()) {
    Serial.println("BLE failed to start");
    while (1);
  }

  BLE.setLocalName("Nano33-Test");
  BLE.setAdvertisedService(potService);
  potService.addCharacteristic(potCharacteristic);
  potService.addCharacteristic(buttonCharacteristic); // Add button characteristic
  BLE.addService(potService);

  BLE.advertise();
  Serial.println("BLE Advertising started");

  pinMode(ledPin, OUTPUT);
  ledRing.begin();
  ledRing.show(); // Initialize all LEDs to off
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(ledPin, HIGH); // Turn on LED when connected

    while (central.connected()) {
      int potValue = analogRead(potPin); // Read potentiometer value
      Serial.print("Potentiometer Value: ");
      Serial.println(potValue); // Print value to Serial Monitor
      potCharacteristic.writeValue(potValue); // Send value to browser

      // Check if a button command was received
      if (buttonCharacteristic.written()) {
        uint8_t buttonNumber = buttonCharacteristic.value();
        Serial.print("Button pressed: ");
        Serial.println(buttonNumber);

        // Light up different zones based on the button number
        if (buttonNumber == 1) {
          lightZone(0, 7, 255, 0, 0); // Light up LEDs 0-7 in red
        } else if (buttonNumber == 2) {
          lightZone(8, 15, 0, 255, 0); // Light up LEDs 8-15 in green
        } else if (buttonNumber == 3) {
          lightZone(16, 23, 0, 0, 255); // Light up LEDs 16-23 in blue
        }
      }

      delay(500); // Update every 500ms
    }

    Serial.println("Disconnected from central");
    digitalWrite(ledPin, LOW); // Turn off LED when disconnected
    clearRing(); // Turn off all LEDs
  }
}

// Function to light up a specific zone on the LED ring
void lightZone(int start, int end, int red, int green, int blue) {
  clearRing(); // Turn off all LEDs first
  for (int i = start; i <= end; i++) {
    ledRing.setPixelColor(i, ledRing.Color(red, green, blue));
  }
  ledRing.show();
}

// Function to clear all LEDs on the ring
void clearRing() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ledRing.setPixelColor(i, 0); // Turn off LED
  }
  ledRing.show();
}