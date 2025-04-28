#ifndef LIGHT_H
#define LIGHT_H

#include <Adafruit_NeoPixel.h>

// Extern parameter arrays
extern const int intensitiesA1[5];
extern const int focusModesA3[5];

// Declare LED functions
void lightA0(Adafruit_NeoPixel &ledRing, int step = 0);
void lightA1(Adafruit_NeoPixel &ledRing, int step = 0);
void lightA2(Adafruit_NeoPixel &ledRing, int step = 0);
void lightA3(Adafruit_NeoPixel &ledRing, int step = 0);
void lightA4(Adafruit_NeoPixel &ledRing, int step = 0);
void lightA5(Adafruit_NeoPixel &ledRing, int step = 0);

#endif