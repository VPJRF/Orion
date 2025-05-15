#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#define PIN        6
#define NUM_LEDS   24
#define POT_PIN    A0

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

float smoothedInput = 0.0;
float smoothingFactor = 0.05;
const float gammaVal = 2.2;

void setAllWarmWhite(uint8_t brightness) {
  uint8_t r = brightness;
  uint8_t g = static_cast<uint8_t>(brightness * 0.75);
  uint8_t b = static_cast<uint8_t>(brightness * 0.25);

  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show();  // Initialize all LEDs to off
}

void loop() {
  int potValue = analogRead(POT_PIN);
  float normalizedInput = potValue / 1023.0;

  // Smooth the input
  smoothedInput = (smoothingFactor * normalizedInput) + ((1.0 - smoothingFactor) * smoothedInput);

  // Apply gamma correction
  float gammaCorrected = pow(smoothedInput, gammaVal);

  // Map to brightness range (10–255)
  int correctedBrightness = round(gammaCorrected * (255 - 10) + 10);

  Serial.print("Pot: ");
  Serial.print(potValue);
  Serial.print("  Smoothed: ");
  Serial.print(smoothedInput, 3);
  Serial.print("  Gamma: ");
  Serial.print(gammaCorrected, 3);
  Serial.print("  Brightness: ");
  Serial.println(correctedBrightness);

  setAllWarmWhite(correctedBrightness);
  delay(30);
}