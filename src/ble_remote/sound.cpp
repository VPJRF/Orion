#include "sound.h"
#include <DFRobot_DF1201S.h>

// --- Parameter arrays for use in main.cpp ---
const int volumesB1[5] = {1, 5, 10, 20, 30};
const char* speedsB2[3] = {"x0,8", "x0,9", "x1,0"};
const char* voicesB3[5] = {"AI man", "AI vrouw", "Elias", "Voogd", "Bekende"};
const char* messagesB4[5] = {"Actie", "Geruststelling", "Tijdstip", "Kort", "Persoonlijk"};

extern DFRobot_DF1201S DF1201S;

void soundB1(int step) {
    DF1201S.setVol(volumesB1[step % 5]);
    DF1201S.playFileNum(1);
}

void soundB2(int step) {
    int files[3] = {2, 3, 4};
    DF1201S.playFileNum(files[step % 3]);
}

void soundB3(int step) {
    int files[5] = {5, 6, 7, 8, 9};
    DF1201S.playFileNum(files[step % 5]);
}

void soundB4(int step) {
    int files[5] = {10, 11, 12, 13, 14};
    DF1201S.playFileNum(files[step % 5]);
}