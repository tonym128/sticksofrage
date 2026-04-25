#include "Engine.h"

int16_t Engine::getSin(uint8_t angle) {
    uint8_t idx = angle & 0x3F;
    uint8_t quad = (angle >> 6) & 0x03;
    int16_t val;
    if (quad == 0) val = pgm_read_word(&SIN_TABLE[idx]);
    else if (quad == 1) val = pgm_read_word(&SIN_TABLE[63 - idx]);
    else if (quad == 2) val = -pgm_read_word(&SIN_TABLE[idx]);
    else val = -pgm_read_word(&SIN_TABLE[63 - idx]);
    return val;
}
int16_t Engine::getCos(uint8_t angle) { return getSin(angle + 64); }

void Engine::drawScaledLine(Arduboy2 &arduboy, int32_t x1, int32_t y1, int32_t x2, int32_t y2, const Camera &camera, uint8_t shakeTimer, int16_t screenOffsetX, int16_t screenOffsetY) {
    int32_t z = camera.zoom; int16_t ox = (shakeTimer > 0) ? random(-2, 3) : 0, oy = (shakeTimer > 0) ? random(-2, 3) : 0;
    int16_t sx1 = (int16_t)((((x1 - camera.x) * z) / 100) >> FP_SHIFT) + (screenOffsetX + ox);
    int16_t sy1 = (int16_t)((((y1 - camera.y) * z) / 100) >> FP_SHIFT) + (screenOffsetY + oy);
    int16_t sx2 = (int16_t)((((x2 - camera.x) * z) / 100) >> FP_SHIFT) + (screenOffsetX + ox);
    int16_t sy2 = (int16_t)((((y2 - camera.y) * z) / 100) >> FP_SHIFT) + (screenOffsetY + oy);
    arduboy.drawLine(sx1, sy1, sx2, sy2);
}

void Engine::drawScaledCircle(Arduboy2 &arduboy, int32_t x, int32_t y, int8_t r, const Camera &camera, uint8_t shakeTimer, int16_t screenOffsetX, int16_t screenOffsetY) {
    int32_t z = camera.zoom; int16_t ox = (shakeTimer > 0) ? random(-2, 3) : 0, oy = (shakeTimer > 0) ? random(-2, 3) : 0;
    int16_t sx = (int16_t)((((x - camera.x) * z) / 100) >> FP_SHIFT) + (screenOffsetX + ox);
    int16_t sy = (int16_t)((((y - camera.y) * z) / 100) >> FP_SHIFT) + (screenOffsetY + oy);
    int8_t sr = (r * z) / 100; if (sr < 1) sr = 1;
    arduboy.drawCircle(sx, sy, sr);
}

void Engine::drawFace(Arduboy2 &arduboy, int16_t x, int16_t y, FaceData& f, bool flip, int16_t zoom) {
    int8_t s = (zoom > 120) ? 1 : 0;
    if (f.headShape == 0) arduboy.drawCircle(x, y, 4+s); else if (f.headShape == 1) arduboy.drawRect(x-(3+s), y-(3+s), 7+s*2, 7+s*2); else arduboy.drawCircle(x, y, 3+s);
    if (f.hairStyle == 1) { for(int i=-(3+s); i<=(3+s); i+=2) arduboy.drawLine(x+i, y-(3+s), x+i, y-(5+s*2)); }
    else if (f.hairStyle == 2) { arduboy.drawFastHLine(x-(3+s), y-(2+s), 7+s*2); }
    else if (f.hairStyle == 4) { arduboy.drawFastVLine(x-(4+s), y-2, 6+s); arduboy.drawFastVLine(x+(4+s), y-2, 6+s); }
    int8_t side = flip ? -1 : 1;
    if (f.eyeType == 0) { arduboy.drawPixel(x-2*side, y-1); arduboy.drawPixel(x+2*side, y-1); }
    else if (f.eyeType == 2) { arduboy.drawLine(x-2, y-2, x-1, y-1); arduboy.drawLine(x+1, y-1, x+2, y-2); }
    if (f.mouthType == 0) arduboy.drawFastHLine(x-1, y+2, 3);
}

void Engine::updateSkeleton(Skeleton &s, const Pose &target, uint16_t frameCount, uint8_t poseIdx) {
    s.breathingPhase += 4; int8_t breath = (getSin(s.breathingPhase) >> 6);
    int32_t hipOffset = TO_FP(s.bones[4].length); 
    for (uint8_t i = 0; i < MAX_BONES; i++) {
        if (s.bones[i].length == 0 && i > 0) break;
        if (i < 6) { 
            uint8_t targetAngle = target.angles[i]; 
            if (poseIdx == 0) { // IDLE
                if (i == 0) targetAngle += breath; 
                if (i == 2 || i == 3) targetAngle -= (breath * 2); 
                if (i == 4 || i == 5) targetAngle += (breath / 2); 
            }
            int16_t diff = (int16_t)targetAngle - s.currentAngles[i]; 
            if (abs(diff) < 2) s.currentAngles[i] = targetAngle; else s.currentAngles[i] += (diff / 4); 
        }
        int32_t startX, startY; uint8_t angle = s.currentAngles[i]; if (s.facingLeft) angle = 128 - angle;
        if (s.bones[i].parent == -1) { 
            startX = s.x; startY = s.y - hipOffset; 
            if (IS_DUCKING(s.state)) startY += TO_FP(4); 
            if (poseIdx >= 1 && poseIdx <= 4) { // WALK
                if ((frameCount / 4) % 4 == 0) startY -= TO_FP(1); 
            }
            if (i == 4) startX += s.facingLeft ? TO_FP(2) : TO_FP(-2); 
            if (i == 5) startX -= s.facingLeft ? TO_FP(2) : TO_FP(-2); 
        }
        else { 
            startX = s.worldX[s.bones[i].parent]; startY = s.worldY[s.bones[i].parent]; 
            if (i == 2) startX += s.facingLeft ? TO_FP(2) : TO_FP(-2); 
            if (i == 3) startX -= s.facingLeft ? TO_FP(2) : TO_FP(-2); 
        }
        s.worldX[i] = startX + TO_FP(((int32_t)getCos(angle) * s.bones[i].length) >> 8); s.worldY[i] = startY + TO_FP(((int32_t)getSin(angle) * s.bones[i].length) >> 8);
    }
}

void Engine::drawSkeleton(Arduboy2 &arduboy, Skeleton &s, const Camera &camera, uint8_t shakeTimer, int16_t screenOffsetX, int16_t screenOffsetY) {
    CharacterData cd; memcpy_P(&cd, &roster[s.charIdx], sizeof(CharacterData));
    int32_t hipOffset = TO_FP(s.bones[4].length);
    for (uint8_t i = 0; i < MAX_BONES; i++) {
        if (s.bones[i].length == 0 && i > 0) break;
        int32_t startX, startY; if (s.bones[i].parent == -1) { startX = s.x; startY = s.y - hipOffset; if (IS_DUCKING(s.state)) startY += TO_FP(4); if (i == 4) startX += s.facingLeft ? TO_FP(2) : TO_FP(-2); if (i == 5) startX -= s.facingLeft ? TO_FP(2) : TO_FP(-2); }
        else { 
            startX = s.worldX[s.bones[i].parent]; startY = s.worldY[s.bones[i].parent]; 
            if (i == 2) startX += s.facingLeft ? TO_FP(2) : TO_FP(-2); 
            if (i == 3) startX -= s.facingLeft ? TO_FP(2) : TO_FP(-2); 
        }
        if (i == 1) { 
            int32_t z = camera.zoom; int16_t ox = (shakeTimer > 0) ? random(-2, 3) : 0, oy = (shakeTimer > 0) ? random(-2, 3) : 0;
            int16_t sx = (int16_t)((((s.worldX[i] - camera.x) * z) / 100) >> FP_SHIFT) + (screenOffsetX + ox);
            int16_t sy = (int16_t)((((s.worldY[i] - camera.y) * z) / 100) >> FP_SHIFT) + (screenOffsetY + oy);
            drawFace(arduboy, sx, sy, cd.face, s.facingLeft, camera.zoom); 
        } else drawScaledLine(arduboy, startX, startY, s.worldX[i], s.worldY[i], camera, shakeTimer, screenOffsetX, screenOffsetY);
    }
}

void Engine::initSkeleton(Skeleton &s, uint8_t cIdx, int32_t x, bool faceLeft) {
    s.charIdx = cIdx; s.x = x; s.y = GROUND_Y; s.vx = 0; s.vy = 0; s.facingLeft = faceLeft; s.isJumping = false; s.state = CS_IDLE; s.stateTimer = 0; s.health = 100; s.special = 0; s.aiState = AI_IDLE; s.aiTimer = 0; s.breathingPhase = random(0, 255);
    CharacterData data; memcpy_P(&data, &roster[cIdx], sizeof(CharacterData)); s.walkSpeed = data.walkSpeed;
    s.bones[0] = {-1, data.lengths[0], false, true}; s.bones[1] = {0, data.lengths[1], false, true}; s.bones[2] = {0, data.lengths[2], true, true}; s.bones[3] = {0, data.lengths[3], true, true}; s.bones[4] = {-1, data.lengths[4], true, true}; s.bones[5] = {-1, data.lengths[5], true, true};
    for(int i=6; i<MAX_BONES; i++) s.bones[i].length = 0;
    Pose p; memcpy_P(&p, &poses[0], sizeof(Pose)); for(int i=0; i<6; i++) s.currentAngles[i] = p.angles[i];
}

uint8_t Engine::getSize(uint8_t cIdx) {
    uint8_t total = 0;
    for(uint8_t i=0; i<6; i++) {
        total += pgm_read_byte(&roster[cIdx].lengths[i]);
    }
    return total;
}

void Engine::drawBitmap(Arduboy2 &arduboy, int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color) {
    arduboy.drawBitmap(x, y, bitmap, w, h, color);
}

void Engine::drawBitmapMirror(Arduboy2 &arduboy, int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color) {
    for (int16_t j = 0; j < h; j += 8) {
        for (int16_t i = 0; i < w; i++) {
            uint8_t byte = pgm_read_byte(bitmap + (j / 8) * w + i);
            for (int16_t k = 0; k < 8; k++) {
                if (byte & (1 << k)) {
                    arduboy.drawPixel(x + w - 1 - i, y + j + k, color);
                }
            }
        }
    }
}
