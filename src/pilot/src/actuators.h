#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <DFRobot_DF1201S.h>

extern const char* deviceName;
extern bool lastLEDState;
extern bool message;
extern bool feedback;
extern bool feedbackActive;
extern DFRobot_DF1201S DF1201S;

void handleActuatorCommand(const JsonDocument& doc, Adafruit_NeoPixel& ring, int LED_RING_COUNT);
void updateLEDFade(Adafruit_NeoPixel& ring, int LED_RING_COUNT);
void updateFeedbackAnimation(Adafruit_NeoPixel& ring, int LED_RING_COUNT);