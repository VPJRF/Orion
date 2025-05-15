#include <map>
#include <Arduino.h>
extern std::map<String, String> testResults;
#include "light.h"
#include "math.h"
#include <Adafruit_NeoPixel.h>

// --- Parameter arrays for use in main.cpp ---
const int intensitiesA1[5] = {5, 10, 40, 100, 255};
const char* colorTempsA2[5] = {"3500K", "3000K", "2500K", "2000K", "1500K"};
const int focusModesA3[5] = {1, 3, 5, 7, 9};
const char* timeModesA4[5] = {"Pixel", "Halo", "Gradient", "Clock", "Sunrise"};
const char* alarmModesA5[5] = {"Fill", "Blink", "Pulse", "Bright", "Rotate"};

// --- Shared color temperature RGB values ---
static const int colorTemps[5][3] = {
    {255, 196, 137},   // 3500K
    {255, 180, 107},   // 3000K
    {255, 161, 72},    // 2500K
    {255, 138, 18},    // 2000K
    {255, 109, 0}      // 1500K
};

// --- Common Colors for Light Modes ---
// Use only 3 colors for all light modes
const int dayCycleColors[3][3] = {
  {0, 0, 30},     // Night: dark blue
  {255, 120, 30}, // Sunrise: orange
  {135, 206, 235} // Daytime: light blue
};

// --- Light functions ---

void lightA0(Adafruit_NeoPixel &ledRing, int step) {
    // Light up all LEDs in red with a simple animation
    for (int i = 0; i < ledRing.numPixels(); i++) {
        ledRing.setPixelColor(i, ledRing.Color(255, 0, 0));
        ledRing.show();
        delay(100);
    }
    delay(1000);
    ledRing.clear();
    ledRing.show();
}

void lightA1(Adafruit_NeoPixel &ledRing, int step) {
    int brightness = intensitiesA1[step % 5];
    for (int i = 0; i < ledRing.numPixels(); i++) {
        ledRing.setPixelColor(i, brightness, brightness, brightness);
    }
    ledRing.show();
}

void lightA2(Adafruit_NeoPixel &ledRing, int step) {
    int idx = step % 5;

    // Default to first brightness
    int brightness = intensitiesA1[0];

    // Try to get last picked brightness from testResults
    auto it = testResults.find("Intensity");
    if (it != testResults.end()) {
        String val = it->second;
        int sep = val.indexOf('/');
        if (sep > 0) {
            brightness = val.substring(0, sep).toInt();
        }
    }

    for (int i = 0; i < ledRing.numPixels(); i++) {
        ledRing.setPixelColor(
            i,
            (colorTemps[idx][0] * brightness) / 255,
            (colorTemps[idx][1] * brightness) / 255,
            (colorTemps[idx][2] * brightness) / 255
        );
    }
    ledRing.show();
}

void lightA3(Adafruit_NeoPixel &ledRing, int step) {
    const int center = 15;
    int colorIdx = 0; // Default to first color temp

    // Use the color temperature index from last A2 pick, if available
    auto itColor = testResults.find("Color");
    if (itColor != testResults.end()) {
        String val = itColor->second;
        for (int i = 0; i < 5; ++i) {
            if (val == colorTempsA2[i]) {
                colorIdx = i;
                break;
            }
        }
    } else {
        colorIdx = step % 5;
    }

    // Use the brightness from last A2 pick, if available
    int baseBrightness = 128; // Default
    auto itIntensity = testResults.find("Intensity");
    if (itIntensity != testResults.end()) {
        String val = itIntensity->second;
        int sep = val.indexOf('/');
        if (sep > 0) {
            baseBrightness = val.substring(0, sep).toInt();
        }
    }

    // Use focusModesA3 to set the amount of pixels (must be odd)
    int numPixels = focusModesA3[step % 5];
    int maxOffset = numPixels / 2;

    ledRing.clear();

    for (int offset = -maxOffset; offset <= maxOffset; ++offset) {
        int pos = center + offset;
        if (pos < 0 || pos >= ledRing.numPixels()) continue;

        int brightness;
        if (numPixels == 1) {
            brightness = 10; // Special case: single pixel, brightness 10
        } else {
            // Linear falloff: center = baseBrightness, edges = close to 0
            float norm = float(abs(offset)) / float(maxOffset);
            brightness = int(baseBrightness * (1.0f - norm));
            if (brightness < 2) brightness = 2; // Minimum brightness
        }

        ledRing.setPixelColor(
            pos,
            (colorTemps[colorIdx][0] * brightness) / 255,
            (colorTemps[colorIdx][1] * brightness) / 255,
            (colorTemps[colorIdx][2] * brightness) / 255
        );
    }
    ledRing.show();
}

void lightA4(Adafruit_NeoPixel &ledRing, int step) {
  int mode = step % 5;

  if (mode == 0) { // Pixel mode
      for (int i = 0; i < 3; i++) { // Only 3 colors
          ledRing.clear();
          ledRing.setPixelColor(15, dayCycleColors[i][0], dayCycleColors[i][1], dayCycleColors[i][2]);
          ledRing.show();
          delay(2000);
      }

      ledRing.clear();
      ledRing.show();
  }

  if (mode == 1) { // Halo mode
    for (int i = 0; i < 3; i++) { // Only 3 colors
        ledRing.clear();
        for (int p = 0; p < ledRing.numPixels(); p++) {
            ledRing.setPixelColor(p,
                dayCycleColors[i][0] > 0 ? 4 : 0,
                dayCycleColors[i][1] > 0 ? 2 : 0,
                dayCycleColors[i][2] > 0 ? 4 : 0
            );
        }
        ledRing.show();
        delay(1000);
    }
    ledRing.clear();
    ledRing.show();
  }

  if (mode == 2) { // Gradient mode
    int center = 15;
    const int steps = 48;
    const int maxSpread = ledRing.numPixels() / 2;

    for (int frame = 0; frame <= steps; frame++) {
        ledRing.clear();

        float spread = (float(frame) / steps) * maxSpread;

        // Use only 3 colors for gradient
        float colorPhase = (float(frame) / steps) * 2.0f; // 0..2
        int colorIdx = int(colorPhase);
        float colorFrac = colorPhase - colorIdx;
        if (colorIdx > 1) { colorIdx = 1; colorFrac = 1.0f; }

        int r = int(dayCycleColors[colorIdx][0] * (1.0f - colorFrac) + dayCycleColors[colorIdx + 1][0] * colorFrac);
        int g = int(dayCycleColors[colorIdx][1] * (1.0f - colorFrac) + dayCycleColors[colorIdx + 1][1] * colorFrac);
        int b = int(dayCycleColors[colorIdx][2] * (1.0f - colorFrac) + dayCycleColors[colorIdx + 1][2] * colorFrac);

        for (int offset = 0; offset <= int(spread); offset++) {
            float fade = 1.0f - (float(offset) / (spread == 0 ? 1 : spread));
            int rr = int(r * fade);
            int gg = int(g * fade);
            int bb = int(b * fade);

            int posPlus = (center + offset) % ledRing.numPixels();
            int posMinus = (center - offset + ledRing.numPixels()) % ledRing.numPixels();
            ledRing.setPixelColor(posPlus, rr, gg, bb);
            ledRing.setPixelColor(posMinus, rr, gg, bb);
        }

        ledRing.show();
        delay(100);
    }
  }

  if (mode == 3) { // Clock mode
      ledRing.clear();

      int totalPixels = ledRing.numPixels();
      int centerPixel = 4;

      // Animate a virtual night quickly, starting and ending at centerPixel
      int animationSteps = totalPixels; // One step per pixel for smoothness
      int delayMs = 200;                 // Delay between frames

      for (int animStep = 0; animStep < animationSteps; animStep++) {
          ledRing.clear();

          // Calculate progress pixel, wrapping around the ring, starting at centerPixel
          int progressPixel = (centerPixel + animStep) % totalPixels;

          for (int i = 0; i < totalPixels; i++) {
              if (i == centerPixel) {
                  ledRing.setPixelColor(i, 10, 10, 10);
              } else if (
                  (progressPixel >= centerPixel && i > centerPixel && i <= progressPixel) ||
                  (progressPixel < centerPixel && (i > centerPixel || i <= progressPixel))
              ) {
                  ledRing.setPixelColor(i, 1, 1, 1);
              }
          }

          ledRing.setPixelColor(progressPixel, 10, 10, 10);

          ledRing.show();
          delay(delayMs);
      }

      // Final frame: all pixels except center are lit
      ledRing.clear();
      for (int i = 0; i < totalPixels; i++) {
          if (i == centerPixel) {
              ledRing.setPixelColor(i, 10, 10, 10);
          } else {
              ledRing.setPixelColor(i, 1, 1, 1);
          }
      }
      ledRing.show();
      delay(delayMs);
  }

  if (mode == 4) { // Sunrise mode
    int lastBrightness = -1;
    // Use a less yellow color for sunrise, e.g. more orange: (255, 120, 30)
    const int sunriseColor[3] = {255, 120, 30};
    for (float t = 0; t <= 1.0f; ) {
        float eased = t * t;
        int brightness = int(eased * 255);
        if (brightness > 255) brightness = 255;
        if (brightness != lastBrightness) {
            int r = (sunriseColor[0] * brightness) / 255;
            int g = (sunriseColor[1] * brightness) / 255;
            int b = (sunriseColor[2] * brightness) / 255;
            for (int i = 0; i < ledRing.numPixels(); i++) {
                ledRing.setPixelColor(i, r, g, b);
            }
            ledRing.show();
            delay(50);
            lastBrightness = brightness;
        }
        t += 0.01 + 0.04 * t;
    }
    // Hold full brightness for 2 seconds
    int r = sunriseColor[0];
    int g = sunriseColor[1];
    int b = sunriseColor[2];
    for (int i = 0; i < ledRing.numPixels(); i++) {
        ledRing.setPixelColor(i, r, g, b);
    }
    ledRing.show();
    delay(2000);
}
}

void lightA5(Adafruit_NeoPixel &ledRing, int step) {
    int mode = step % 5;

    if (mode == 0) { // Fill
        int center = 15;
        int maxSpread = ledRing.numPixels() / 2;
        for (int spread = 0; spread <= maxSpread; spread++) {
            ledRing.clear();
            for (int offset = 0; offset <= spread; offset++) {
                int posPlus = (center + offset) % ledRing.numPixels();
                int posMinus = (center - offset + ledRing.numPixels()) % ledRing.numPixels();
                ledRing.setPixelColor(posPlus, 255, 0, 0);
                ledRing.setPixelColor(posMinus, 255, 0, 0);
            }
            ledRing.show();
            delay(200);
        }
    }

    if (mode == 1) { // Blink
        for (int i = 0; i < 20; i++) {
            int brightness = (i % 2 == 0) ? 100 : 0;
            for (int p = 0; p < ledRing.numPixels(); p++) {
                ledRing.setPixelColor(p, brightness, 0, 0);
            }
            ledRing.show();
            delay(300);
        }
    }

    if (mode == 2) { // Pulse
        for (int t = 0; t < 200; t++) {
            float sineValue = (sin(t * 0.15) + 1) / 2;
            int brightness = (int)(10 + sineValue * (100 - 10));
            for (int i = 0; i < ledRing.numPixels(); i++) {
                ledRing.setPixelColor(i, brightness, 0, 0);
            }
            ledRing.show();
            delay(30);
        }
    }

    if (mode == 3) { // Bright
        for (int t = 0; t < 50; t++) {
            int brightness = (t * 5) % 256;
            for (int i = 0; i < ledRing.numPixels(); i++) {
                ledRing.setPixelColor(i, brightness, 0, 0);
            }
            ledRing.show();
            delay(100);
        }
    }

    if (mode == 4) { // Rotate
        for (int t = 0; t < ledRing.numPixels()*4; t++) {
            int centerPixel = t;
            ledRing.clear();
            for (int offset = -4; offset <= 4; offset++) {
                int pos = (centerPixel + offset + ledRing.numPixels()) % ledRing.numPixels();
                int brightness = 5 + (4 - abs(offset)) * (95 / 4);
                ledRing.setPixelColor(pos, brightness, 0, 0);
            }
            ledRing.show();
            delay(50);
        }
    }
}