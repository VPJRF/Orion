#include "light.h"

void ledFunctionA1(Adafruit_NeoPixel &ledRing) {
  // Example: Light up LEDs 0-7 in red
  for (int i = 0; i <= 7; i++) {
    ledRing.setPixelColor(i, ledRing.Color(255, 0, 0));
  }
  ledRing.show();
}

void ledFunctionA2(Adafruit_NeoPixel &ledRing) {
  // Example: Light up LEDs 8-15 in green
  for (int i = 8; i <= 15; i++) {
    ledRing.setPixelColor(i, ledRing.Color(0, 255, 0));
  }
  ledRing.show();
}

void ledFunctionA3(Adafruit_NeoPixel &ledRing) {
  // Example: Light up LEDs 16-23 in blue
  for (int i = 16; i <= 23; i++) {
    ledRing.setPixelColor(i, ledRing.Color(0, 0, 255));
  }
  ledRing.show();
}