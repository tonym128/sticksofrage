#ifndef POSES_H
#define POSES_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "Platform.h"
#endif

const Pose poses[] PROGMEM = {
    {{194, 190, 81, 35, 75, 41}},   // 0: IDLE
    {{198, 190, 25, 100, 40, 85}},   // 1: W1
    {{194, 190, 64, 64, 64, 64}},   // 2: W2
    {{190, 190, 100, 25, 85, 40}},   // 3: W3
    {{194, 190, 64, 64, 64, 64}},   // 4: W4
    {{192, 192, 234, 38, 98, 46}},   // 5: BLOCK
    {{220, 183, 88, 252, 64, 34}},   // 6: PUNCH
    {{192, 210, 80, 42, 84, 242}},   // 7: KICK
    {{192, 2, 54, 56, 74, 52}},   // 8: DUCK
    {{172, 168, 22, 30, 46, 30}},   // 9: HIT
    {{192, 2, 62, 2, 64, 48}},   // 10: DPUN
    {{192, 2, 74, 54, 56, 255}}   // 11: DKIC
};

const char* const animNames[] = { "IDLE", "W1", "W2", "W3", "W4", "BLOCK", "PUNCH", "KICK", "DUCK", "HIT", "DPUN", "DKIC" };

#endif
