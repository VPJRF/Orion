#include <Arduino.h>
#include <DFRobot_DF1201S.h>

#define DF1201SSerial Serial1
#define POT_PIN A0

DFRobot_DF1201S DF1201S;
int lastVolume = -1;

void updateVolume() {
  int potValue = analogRead(POT_PIN);
  int volume = map(potValue, 0, 1023, 0, 30);

  if (abs(volume - lastVolume) >= 2) {
    DF1201S.setVol(volume);
    Serial.print("Volume set to: ");
    Serial.println(volume);
    lastVolume = volume;
  }
}

void setup() {
  Serial.begin(115200);
  DF1201SSerial.begin(115200);

  while (!DF1201S.begin(DF1201SSerial)) {
    Serial.println("Init failed, please check the wire connection!");
    delay(1000);
  }

  DF1201S.switchFunction(DF1201S.MUSIC);
  delay(2000);

  updateVolume();
  DF1201S.playFileNum(1);
}

void loop() {
  updateVolume();
  delay(200);
}