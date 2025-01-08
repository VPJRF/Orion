#include <IRremote.h>
#include <FastLED.h>

// IR codes
#define IR_RECEIVE_PIN 2
#define REMOTE_4 0xBC435583
#define REMOTE_7 0x619E5583

// LED strip
#define LED_PIN 9
#define NUM_LEDS 20
#define LED_BRIGHTNESS 150
CRGB leds[NUM_LEDS];
#define LED_COLOR CRGB(238, 153, 17)

// Flags
bool polestarActive = false;
float currentBrightness = 0.0;

// Helper functions

bool isStopSignalReceived() {
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.decodedRawData == REMOTE_7) {
      Serial.println("STOP signal received (REMOTE_7)");
      stopAllEffects();
      return true;
    }
    IrReceiver.resume();
  }
  return false;
}

void stopAllEffects() {
  Serial.println("Stopping all effects and fading out.");
  fadeOutEffect();
  polestarActive = false;
  FastLED.clear();
  FastLED.show();
  currentBrightness = 0.0;
  delay(10);
}

void fadeOutEffect() {
  Serial.println("Fading out LEDs slowly.");
  for (int fadeStep = 0; fadeStep <= 255; fadeStep += 2) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(2);
    }
    FastLED.show();
    delay(15); // Slower fade-out
  }
  FastLED.clear();
  FastLED.show();
}

// Actuator functions

void Polestar() {
  Serial.println("Starting Polestar effect.");
  polestarActive = true;
  float brightness = 0.0;

  for (int step = 0; step <= 1000 && polestarActive; step++) {
    brightness = pow((float)step / 1000, 2.2);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = LED_COLOR;
      leds[i].fadeToBlackBy(255 - (brightness * 255));
    }
    FastLED.show();
    delay(3); // speed

    if (isStopSignalReceived()) {
      polestarActive = false;
      Serial.println("Fade-in interrupted by STOP signal.");
      return;
    }
  }

  Serial.println("Polestar effect complete.");
}

// Initial setup
void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  Serial.println(F("IR Remote Ready - Waiting for Input"));
}

// IR receiver
void loop() {
  if (IrReceiver.decode()) {
    Serial.print("IR code received: ");
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
    switch (IrReceiver.decodedIRData.decodedRawData) {
      case REMOTE_4:
        Serial.println("REMOTE_4 detected - Starting Polestar.");
        Polestar();
        break;
      case REMOTE_7:
        Serial.println("REMOTE_7 detected - Stopping all effects.");
        stopAllEffects();
        break;
      default:
        Serial.println("Unknown IR code.");
        break;
    }
    IrReceiver.resume();
  }
}
