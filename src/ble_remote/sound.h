#ifndef SOUND_H
#define SOUND_H

#include <DFRobot_DF1201S.h>

// Extern parameter arrays
extern const int volumesB1[5];

// Declare speaker functions with cycling support
void soundB1(int step = 0);
void soundB2(int step = 0);
void soundB3(int step = 0);
void soundB4(int step = 0);

#endif