#include "scene.h"

void buttonFeedback(const String &button) {
  Serial.print("Button pressed: ");
  Serial.println(button);
}

void sceneC1() {
  Serial.println("Scene C1 activated: Bed");
  // Add specific actions for Scene C1 here
}

void sceneC2() {
  Serial.println("Scene C2 activated: Kamer");
  // Add specific actions for Scene C2 here
}

void sceneC3() {
  Serial.println("Scene C3 activated: Gang");
  // Add specific actions for Scene C3 here
}

void sceneC4() {
  Serial.println("Scene C4 activated: Toilet");
  // Add specific actions for Scene C4 here
}