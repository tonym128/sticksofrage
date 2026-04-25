#ifndef ENGINE_H
#define ENGINE_H

#include "Platform.h"
#include "game_data.h"

class Engine {
public:
    static int16_t getSin(uint8_t angle);
    static int16_t getCos(uint8_t angle);

    static void updateSkeleton(Skeleton &s, const Pose &target, uint16_t frameCount, uint8_t poseIdx);
    static void drawSkeleton(Arduboy2 &arduboy, Skeleton &s, const struct Camera &camera, uint8_t shakeTimer, int16_t screenOffsetX = 64, int16_t screenOffsetY = 32);
    static void initSkeleton(Skeleton &s, uint8_t cIdx, int32_t x, bool faceLeft);
    static uint8_t getSize(uint8_t cIdx);

    static void drawScaledLine(Arduboy2 &arduboy, int32_t x1, int32_t y1, int32_t x2, int32_t y2, const struct Camera &camera, uint8_t shakeTimer, int16_t screenOffsetX = 64, int16_t screenOffsetY = 32);
    static void drawScaledCircle(Arduboy2 &arduboy, int32_t x, int32_t y, int8_t r, const struct Camera &camera, uint8_t shakeTimer, int16_t screenOffsetX = 64, int16_t screenOffsetY = 32);
    static void drawFace(Arduboy2 &arduboy, int16_t x, int16_t y, FaceData& f, bool flip, int16_t zoom);

    static void drawBitmap(Arduboy2 &arduboy, int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color = WHITE);
    static void drawBitmapMirror(Arduboy2 &arduboy, int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color = WHITE);
};

struct Camera {
    int32_t x, y;
    int16_t zoom;
};

#define IS_DUCKING(s) ((s) == CS_DUCK || (s) == CS_DUCK_PUNCH_STARTUP || (s) == CS_DUCK_PUNCH_ACTIVE || (s) == CS_DUCK_PUNCH_RECOVERY || (s) == CS_DUCK_KICK_STARTUP || (s) == CS_DUCK_KICK_ACTIVE || (s) == CS_DUCK_KICK_RECOVERY)

#endif
