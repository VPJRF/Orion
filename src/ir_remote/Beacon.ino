#include <IRremote.h>
#include <FastLED.h>

// ====================
// CONFIGURATIONS
// ====================

// IR Remote Setup
#define IR_RECEIVE_PIN 2
#define REMOTE_1 0xC03F5583     // NEC code for button 1
#define REMOTE_3 0xDC235583     // NEC code for button 3
#define REMOTE_4 0xBC435583     // NEC code for button 4
#define REMOTE_5 0xBE415583     // NEC code for button 5
#define REMOTE_6 0x9F605583     // NEC code for button 6
#define REMOTE_9 0x708F5583     // NEC code for button 9
#define REMOTE_ESC 0x7B845583   // NEC code for ESC button

// RGB LED Setup (Single LED)
#define RED_PIN 5
#define GREEN_PIN 10
#define BLUE_PIN 6
const int rgbPins[] = {RED_PIN, GREEN_PIN, BLUE_PIN};
const int rgbValues[] = {200, 50, 0};
const int maxBrightness = 255;

// RGB Addressable LEDs
#define LED_PIN 9
#define NUM_LEDS 10
#define LED_BRIGHTNESS 255
CRGB leds[NUM_LEDS];
CRGB nextLeds[NUM_LEDS];
CRGB currentLeds[NUM_LEDS];

// State Control
bool pillowActive = false;
bool skyActive = false;
bool alarmActive = false;
bool singleRGBActive = false;
bool stopSunrise = false;
bool fadeOutSunrise = false;
bool breathingActive = false;
float currentBrightness = 0.0;

const unsigned long delayInterval = 10;
const unsigned long fadeStepInterval = 50;
int currentFadeStep = 0;
unsigned long lastFadeMillis = 0;

// ====================
// FUNCTIONS
// ====================

// ===== Helper Functions =====
void fadeDownToBlack() {
  float fadeBrightness = currentBrightness;
  while (fadeBrightness > 0.01) {
    fadeBrightness -= 0.01;
    for (int i = 0; i < 3; i++) {
      analogWrite(rgbPins[i], (rgbValues[i] * fadeBrightness));
    }
    delay(delayInterval);
  }
  for (int i = 0; i < 3; i++) analogWrite(rgbPins[i], 0);
  Serial.println("Single RGB LED Faded Out");
}

void fadeUpToBlack() {
  for (int fadeStep = 0; fadeStep <= 255; fadeStep += 5) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(5);
    }
    FastLED.show();
    delay(20);
  }
  FastLED.clear();
  FastLED.show();
  Serial.println("LED Strip Faded Out");
}

void stopAllEffects() {
  if (singleRGBActive) fadeDownToBlack();
  if (skyActive || alarmActive || fadeOutSunrise || breathingActive) fadeUpToBlack();
  singleRGBActive = skyActive = alarmActive = fadeOutSunrise = breathingActive = false;
  stopSunrise = true;
}

// ===== Meditation effect =====
void Breathing() {
  Serial.println("Starting Breathing Exercise...");
  breathingActive = true;

  const int totalCycleTime = 10000;
  const int fadeInTime = 2000;
  const int fadeOutTime = 2000;
  const int holdTime = (totalCycleTime - fadeInTime - fadeOutTime) / 2;
  const int maxBrightness = 255;
  const int minBrightness = 90;

  bool firstCycle = true;

  while (breathingActive) {
    // Fade In
    int startBrightness = firstCycle ? 0 : minBrightness;
    for (int fade = startBrightness; fade <= maxBrightness; fade++) {
      fill_solid(leds, NUM_LEDS, CRGB::OrangeRed);
      FastLED.setBrightness(fade);
      FastLED.show();
      delay(fadeInTime / (maxBrightness - startBrightness));

      if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.decodedRawData == REMOTE_ESC) {
          stopAllEffects();
          return;
        }
        IrReceiver.resume();
      }
    }

    // Hold at Full Brightness
    for (int i = 0; i < holdTime / 10; i++) {
      delay(10);
      if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.decodedRawData == REMOTE_ESC) {
          stopAllEffects();
          return;
        }
        IrReceiver.resume();
      }
    }

    // Fade Out
    for (int fade = maxBrightness; fade >= minBrightness; fade--) {
      fill_solid(leds, NUM_LEDS, CRGB::OrangeRed);
      FastLED.setBrightness(fade);
      FastLED.show();
      delay(fadeOutTime / (maxBrightness - minBrightness));

      if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.decodedRawData == REMOTE_ESC) {
          stopAllEffects();
          return;
        }
        IrReceiver.resume();
      }
    }

    // Hold at Low Brightness
    for (int i = 0; i < holdTime / 10; i++) {
      delay(10);
      if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.decodedRawData == REMOTE_ESC) {
          stopAllEffects();
          return;
        }
        IrReceiver.resume();
      }
    }

    firstCycle = false;
  }

  Serial.println("Breathing Exercise Complete");
}

// ===== Beacon light =====
void Pillow() {
  pillowActive = true;
  singleRGBActive = true;
  currentBrightness = 0.0;
  unsigned long previousMillis = millis();

  while (currentBrightness <= 1.0 && pillowActive) {
    if (millis() - previousMillis >= delayInterval) {
      previousMillis = millis();
      currentBrightness = min(1.0, currentBrightness + 0.01);
      for (int i = 0; i < 3; i++) {
        analogWrite(rgbPins[i], (rgbValues[i] * currentBrightness));
      }
    }

    if (IrReceiver.decode()) {
      uint32_t irData = IrReceiver.decodedIRData.decodedRawData;
      if (irData == REMOTE_ESC) {
        stopAllEffects();
        return;
      }
      IrReceiver.resume();
    }
  }
  pillowActive = false;
  Serial.println("Fade-In Complete");
}

// ===== Sea Projection =====
void Sky() {
  Serial.println("Starting Sea Projection...");
  skyActive = true;
  currentFadeStep = 0;

  // Ensure LEDs start from off
  FastLED.clear();
  FastLED.show();

  // Set currentLeds to DarkBlue
  for (int i = 0; i < NUM_LEDS; i++) {
    currentLeds[i] = CRGB::DarkBlue;
  }

  // Fade In to DarkBlue
  for (int brightness = 0; brightness <= LED_BRIGHTNESS; brightness++) {
    fill_solid(leds, NUM_LEDS, CRGB::DarkBlue);
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(10);
  }
  Serial.println("Sea Projection Fade-In to DarkBlue Complete");

  // Run the projection effect
  while (skyActive) {
    if (millis() - lastFadeMillis >= fadeStepInterval) {
      lastFadeMillis = millis();
      for (int i = 0; i < NUM_LEDS; i++) {
        currentLeds[i].r += (nextLeds[i].r - currentLeds[i].r) / 8;
        currentLeds[i].g += (nextLeds[i].g - currentLeds[i].g) / 8;
        currentLeds[i].b += (nextLeds[i].b - currentLeds[i].b) / 8;
        leds[i] = currentLeds[i];
      }
      FastLED.show();

      if (++currentFadeStep >= 16) {
        currentFadeStep = 0;
        for (int i = 0; i < NUM_LEDS; i++) {
          int colorChoice = random(0, 4);
          nextLeds[i] = (colorChoice == 0) ? CRGB::Black :
                        (colorChoice == 1) ? CRGB::MediumBlue :
                        (colorChoice == 2) ? CRGB::DarkBlue : CRGB::DeepSkyBlue;
        }
      }
    }

    // Check for ESC signal
    if (IrReceiver.decode()) {
      if (IrReceiver.decodedIRData.decodedRawData == REMOTE_ESC) {
        stopAllEffects();
        return;
      }
      IrReceiver.resume();
    }
  }
  Serial.println("Sea Projection Stopped");
}

// ===== Sunrise Effect =====
void Sunrise() {
  Serial.println("Starting Sunrise Effect...");
  stopSunrise = false;
  fadeOutSunrise = false;
  float brightness = 0.0;
  for (int step = 0; step <= 1000 && !stopSunrise; step++) {
    brightness = pow((float)step / 1000, 2.2);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::White;
      leds[i].fadeToBlackBy(255 - (brightness * 255));
    }
    FastLED.show();
    delay(5);

    if (IrReceiver.decode()) {
      if (IrReceiver.decodedIRData.decodedRawData == REMOTE_ESC) {
        fadeOutSunrise = true;
        stopAllEffects();
        return;
      }
      IrReceiver.resume();
    }
  }
  Serial.println("Sunrise Complete");
  fadeOutSunrise = true;
}

// ===== Alarm Effect =====
void Alarm() {
  Serial.println("Starting Alarm Effect...");
  alarmActive = true;
  float brightness = 0.0;
  bool increasing = true;
  while (alarmActive) {
    brightness += (increasing) ? 0.003 : -0.003;
    increasing = (brightness >= 1.0 || brightness <= 0.0) ? !increasing : increasing;
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Red;
      leds[i].fadeToBlackBy(255 - (brightness * 255));
    }
    FastLED.show();
    delay(3);

    if (IrReceiver.decode()) {
      if (IrReceiver.decodedIRData.decodedRawData == REMOTE_ESC) {
        stopAllEffects();
        return;
      }
      IrReceiver.resume();
    }
  }
}

// ====================
// SETUP AND LOOP
// ====================
void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  Serial.println(F("IR Remote Ready - Waiting for Input"));
}

void loop() {
  if (IrReceiver.decode()) {
    switch (IrReceiver.decodedIRData.decodedRawData) {
      case REMOTE_1: Pillow(); break;
      case REMOTE_3: Breathing(); break;
      case REMOTE_5: Sky(); break;
      case REMOTE_6: Sunrise(); break;
      case REMOTE_9: Alarm(); break;
      case REMOTE_ESC: stopAllEffects(); break;
    }
    IrReceiver.resume();
  }
}
