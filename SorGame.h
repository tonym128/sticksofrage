#ifndef SORG_GAME_H
#define SORG_GAME_H

#include "Platform.h"
#include "game_data.h"
#include "Engine.h"
#include "logo.h"
#include "stick.h"

#define MAX_ENEMIES 4
#define LEVEL_END_X TO_FP(2500)
#define LANE_MIN_Y TO_FP(80)
#define LANE_MAX_Y TO_FP(112)

class SorGame {
public:
    void setup();
    void loop();

protected:
    void drawBackground();
    void triggerHit(Skeleton &attacker, Skeleton &defender);
    void updatePlayer();
    void updateEnemies();
    void resetStage();
    void updateFight();
    void drawFight();
    void drawMenu();
    void drawCharSelect();
    void drawStageIntro();
    void drawStageClear();
    void drawEnding();

    void updateInputBuffer();
    bool checkCombo(const uint8_t* sequence, uint8_t length);

    Arduboy2 arduboy;
    GameState currentState = STATE_TITLE;
    Camera camera = { TO_FP(0), TO_FP(48), 100 };
    
    Skeleton player;
    Skeleton enemies[MAX_ENEMIES];
    bool enemyActive[MAX_ENEMIES];
    
    InputBuffer playerBuffer;

    uint8_t shakeTimer = 0, freezeTimer = 0;
    uint8_t selectedChar = 0;
    uint8_t currentStage = 0; // 0 to 4
    int32_t stageProgress = 0;
    int32_t maxCameraX = 0;
    
    uint8_t enemiesDefeated = 0;
    uint8_t totalEnemiesToSpawn = 0;
    uint8_t enemiesSpawned = 0;
    bool bossSpawned = false;
    bool bossDefeated = false;

    uint8_t menuIdx = 0;
    uint16_t delayTimer = 0;
};

#endif