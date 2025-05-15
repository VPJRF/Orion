#include "actuators.h"

enum FadeState { FADE_NONE, FADE_IN, FADE_OUT };
FadeState fadeState = FADE_NONE;
float currentIntensity = 0.0f; // 0.0 - 1.0
float targetIntensity = 0.0f;
unsigned long fadeStartTime = 0;
const unsigned long fadeDuration = 3000; // ms, adjust as needed

// Set these appropriately in your code
extern const char* deviceName; // "bed", "kamer", "toilet", etc.
extern bool lastLEDState;
extern bool feedback; // Add this at the top if not present

// Add global DFPlayer object and serial
HardwareSerial& dfSerial = Serial1; // Use Serial1 for RX/TX (Nano 33 IoT: TX=1, RX=0)

// Add at the top, after other globals:
bool feedbackActive = false;
unsigned long feedbackStartTime = 0;
const unsigned long feedbackDuration = 5 * 60 * 1000; // 5 minutes in ms

// Define the bed/feedback color and intensity only once
const float BED_FEEDBACK_INTENSITY = 10.0f / 255.0f;
const uint8_t BED_FEEDBACK_R = 255 * BED_FEEDBACK_INTENSITY;
const uint8_t BED_FEEDBACK_G = 255 * BED_FEEDBACK_INTENSITY;
const uint8_t BED_FEEDBACK_B = 255 * BED_FEEDBACK_INTENSITY;

void setRingColor(Adafruit_NeoPixel& ring, int LED_RING_COUNT, float intensity) {
    float maxIntensity = (deviceName && strcmp(deviceName, "bed") == 0) ? BED_FEEDBACK_INTENSITY : (100.0f / 255.0f);
    if (intensity > maxIntensity) intensity = maxIntensity;

    uint8_t r, g, b;
    if (deviceName && strcmp(deviceName, "bed") == 0) {
        // Always scale by intensity!
        r = 255 * intensity;
        g = 255 * intensity;
        b = 255 * intensity;
    } else {
        r = 255 * intensity;
        g = 180 * intensity;
        b = 60 * intensity;
    }

    for (int i = 0; i < LED_RING_COUNT; i++) {
        ring.setPixelColor(i, ring.Color(r, g, b));
    }
    ring.show();
}

void startLEDFade(Adafruit_NeoPixel& ring, int LED_RING_COUNT, bool fadeIn) {
    // Set a different but visible value for the bed
    float maxIntensity = (deviceName == "bed") ? (30.0f / 255.0f) : (100.0f / 255.0f);
    // If interrupted, start from currentIntensity
    targetIntensity = fadeIn ? maxIntensity : 0.0f;
    fadeState = fadeIn ? FADE_IN : FADE_OUT;
    fadeStartTime = millis();
}

void updateLEDFade(Adafruit_NeoPixel& ring, int LED_RING_COUNT) {
    if (fadeState == FADE_NONE) return;

    // Match the new value here as well
    float maxIntensity = (deviceName == "bed") ? (30.0f / 255.0f) : (100.0f / 255.0f);
    unsigned long elapsed = millis() - fadeStartTime;
    float t = min(1.0f, elapsed / (float)fadeDuration);

    float startIntensity = currentIntensity;
    float endIntensity = targetIntensity;

    // Linear interpolation from currentIntensity to targetIntensity
    currentIntensity = startIntensity + (endIntensity - startIntensity) * t;
    setRingColor(ring, LED_RING_COUNT, currentIntensity);

    if (t >= 1.0f || abs(currentIntensity - targetIntensity) < 0.001f) {
        currentIntensity = targetIntensity;
        setRingColor(ring, LED_RING_COUNT, currentIntensity);
        fadeState = FADE_NONE;
        lastLEDState = (currentIntensity > 0.0f);
    }
}

void updateFeedbackAnimation(Adafruit_NeoPixel& ring, int LED_RING_COUNT) {
    if (!feedbackActive) {
        // Turn off all LEDs
        for (int i = 0; i < LED_RING_COUNT; i++) {
            ring.setPixelColor(i, 0, 0, 0);
        }
        ring.show();
        return;
    }

    // Animation: fill the circle clockwise over 5 minutes
    unsigned long elapsed = millis() - feedbackStartTime;
    if (elapsed >= feedbackDuration) {
        feedbackActive = false;
        // Optionally, turn off all LEDs when done
        for (int i = 0; i < LED_RING_COUNT; i++) {
            ring.setPixelColor(i, 0, 0, 0);
        }
        ring.show();
        return;
    }

    // Calculate how many LEDs should be lit (start with 1, end with all)
    float progress = (float)elapsed / feedbackDuration;
    int ledsToLight = max(1, (int)(progress * LED_RING_COUNT + 0.5f));

    // Use the same color as the bed
    for (int i = 0; i < LED_RING_COUNT; i++) {
        if (i < ledsToLight) {
            ring.setPixelColor(i, BED_FEEDBACK_R, BED_FEEDBACK_G, BED_FEEDBACK_B);
        } else {
            ring.setPixelColor(i, 0, 0, 0);
        }
    }
    ring.show();
}

void handleActuatorCommand(const JsonDocument& doc, Adafruit_NeoPixel& ring, int LED_RING_COUNT) {
    // Handle LED command
    if (doc.containsKey("LED")) {
        int value = doc["LED"];
        if (value == 1) {
            Serial.println("LED command received (fade in)");
            startLEDFade(ring, LED_RING_COUNT, true);
        } else {
            Serial.println("LED command received (fade out)");
            startLEDFade(ring, LED_RING_COUNT, false);
        }
    }

    // Handle BOX command
    if (doc.containsKey("BOX")) {
        int value = doc["BOX"];
        message = value; // Update the global message variable
        Serial.print("BOX command received, value: ");
        Serial.println(value);
        if (value == 1) {
            Serial.println("Playing sound 1...");
            DF1201S.setVol(15);
            bool result = DF1201S.playFileNum(1);
            if (!result) {
                Serial.println("DFPlayer playFileNum failed!");
            }
        }
    }

    // Handle FBK command
    if (doc.containsKey("FBK")) {
        int value = doc["FBK"];
        if (value == 1) {
            Serial.println("FBK command received (start feedback)");
            feedbackActive = true;
            feedbackStartTime = millis();
            feedback = true; // <-- Set global feedback variable
        } else {
            Serial.println("FBK command received (stop feedback)");
            feedbackActive = false;
            feedback = false; // <-- Set global feedback variable
            // Turn off all LEDs
            for (int i = 0; i < LED_RING_COUNT; i++) {
                ring.setPixelColor(i, 0, 0, 0);
            }
            ring.show();
        }
    }
}