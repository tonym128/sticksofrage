#include "SorGame.h"
#include <algorithm>

void SorGame::setup() {
    arduboy.begin();
    arduboy.setFrameRate(60);
    playerBuffer.head = 0;
    memset(playerBuffer.buttons, 0, INPUT_BUFFER_SIZE);
    for(int i=0; i<MAX_ENEMIES; i++) enemyActive[i] = false;
}

void SorGame::resetStage() {
    Engine::initSkeleton(player, selectedChar, TO_FP(100), false);
    player.y = TO_FP(96);
    player.health = 100;
    
    stageProgress = 0;
    maxCameraX = 0;
    enemiesDefeated = 0;
    enemiesSpawned = 0;
    totalEnemiesToSpawn = 10 + (currentStage * 5);
    bossSpawned = false;
    bossDefeated = false;
    
    for(int i=0; i<MAX_ENEMIES; i++) enemyActive[i] = false;
    camera.x = 0;
    camera.y = TO_FP(48);
    camera.zoom = 100;
}

void SorGame::triggerHit(Skeleton &attacker, Skeleton &defender) {
    shakeTimer = 8; freezeTimer = 5;
    
    // Depth check! If they are on different Y planes, no hit
    if (labs(attacker.y - defender.y) > TO_FP(15)) return;

    int8_t dmg = 10;
    if (attacker.state == CS_PUNCH_ACTIVE) dmg = 8;
    else if (attacker.state == CS_KICK_ACTIVE) dmg = 12;

    uint8_t attackerSize = Engine::getSize(attacker.charIdx);
    dmg = (int16_t)dmg * attackerSize / 62;

    int16_t kb = TO_FP(3);
    if (attacker.x < defender.x) { defender.vx = kb; attacker.vx = -kb/2; } 
    else { defender.vx = -kb; attacker.vx = kb/2; }
    
    defender.health -= dmg; 
    defender.state = CS_HITSTUN; 
    defender.stateTimer = 15;
    
    if (defender.health < 0) defender.health = 0; 
}

void SorGame::updateInputBuffer() {
    playerBuffer.buttons[playerBuffer.head] = arduboy.buttonsState();
    playerBuffer.head = (playerBuffer.head + 1) % INPUT_BUFFER_SIZE;
}

bool SorGame::checkCombo(const uint8_t* sequence, uint8_t length) {
    return false; // No special moves needed for now, to simplify.
}

void SorGame::updatePlayer() {
    if (freezeTimer > 0) { freezeTimer--; return; }

    if (player.stateTimer > 0) player.stateTimer--;

    if (player.state == CS_HITSTUN) {
        player.x += player.vx;
        player.vx = (player.vx * 9) / 10;
        if (player.stateTimer == 0) player.state = CS_IDLE;
        return;
    }

    if (player.state >= CS_PUNCH_STARTUP && player.state <= CS_KICK_RECOVERY) {
        if (player.state == CS_PUNCH_STARTUP && player.stateTimer == 0) { player.state = CS_PUNCH_ACTIVE; player.stateTimer = 5; }
        else if (player.state == CS_PUNCH_ACTIVE && player.stateTimer == 0) { player.state = CS_PUNCH_RECOVERY; player.stateTimer = 10; }
        else if (player.state == CS_PUNCH_RECOVERY && player.stateTimer == 0) player.state = CS_IDLE;
        
        else if (player.state == CS_KICK_STARTUP && player.stateTimer == 0) { player.state = CS_KICK_ACTIVE; player.stateTimer = 8; }
        else if (player.state == CS_KICK_ACTIVE && player.stateTimer == 0) { player.state = CS_KICK_RECOVERY; player.stateTimer = 15; }
        else if (player.state == CS_KICK_RECOVERY && player.stateTimer == 0) player.state = CS_IDLE;
        
        // Check hits during active frames
        if (player.state == CS_PUNCH_ACTIVE || player.state == CS_KICK_ACTIVE) {
            for(int i=0; i<MAX_ENEMIES; i++) {
                if (!enemyActive[i] || enemies[i].state == CS_HITSTUN || enemies[i].health <= 0) continue;
                int32_t dx = FROM_FP(player.x - enemies[i].x);
                if (labs(dx) < 30 && labs(player.y - enemies[i].y) < TO_FP(15)) {
                    if ((player.facingLeft && dx > 0) || (!player.facingLeft && dx < 0)) {
                        triggerHit(player, enemies[i]);
                    }
                }
            }
        }
        return;
    }

    // Input handling
    player.vx = 0; player.vy = 0;
    bool moving = false;
    if (arduboy.pressed(LEFT_BUTTON)) { player.vx = -player.walkSpeed; player.facingLeft = true; moving = true; }
    else if (arduboy.pressed(RIGHT_BUTTON)) { player.vx = player.walkSpeed; player.facingLeft = false; moving = true; }
    
    if (arduboy.pressed(UP_BUTTON)) { player.vy = -player.walkSpeed; moving = true; }
    else if (arduboy.pressed(DOWN_BUTTON)) { player.vy = player.walkSpeed; moving = true; }

    if (arduboy.justPressed(A_BUTTON)) { player.state = CS_PUNCH_STARTUP; player.stateTimer = 5; moving = false; }
    else if (arduboy.justPressed(B_BUTTON)) { player.state = CS_KICK_STARTUP; player.stateTimer = 8; moving = false; }

    if (moving && player.state == CS_IDLE) player.state = CS_WALK;
    else if (!moving && player.state == CS_WALK) player.state = CS_IDLE;

    player.x += player.vx;
    player.y += player.vy;

    // Bounds check
    if (player.y < LANE_MIN_Y) player.y = LANE_MIN_Y;
    if (player.y > LANE_MAX_Y) player.y = LANE_MAX_Y;

    if (player.x < maxCameraX) player.x = maxCameraX; // Can't go back

    // Camera follow
    int32_t targetCamX = player.x - TO_FP(64);
    if (targetCamX > maxCameraX) {
        // Only advance if enemies are clear
        bool enemiesAlive = false;
        for(int i=0; i<MAX_ENEMIES; i++) if (enemyActive[i]) enemiesAlive = true;
        
        if (!enemiesAlive || bossSpawned) {
            maxCameraX = targetCamX;
            camera.x = targetCamX;
            stageProgress = maxCameraX;
        } else {
            player.x -= player.vx; // Block advancing
        }
    }
}
#include "SorGame.h"

void SorGame::updateEnemies() {
    if (freezeTimer > 0) return;

    // Spawning logic
    if (stageProgress >= LEVEL_END_X && !bossSpawned && enemiesDefeated >= totalEnemiesToSpawn) {
        // Spawn boss
        for(int i=0; i<MAX_ENEMIES; i++) {
            if (!enemyActive[i]) {
                Engine::initSkeleton(enemies[i], 9, player.x + TO_FP(100), true);
                enemies[i].y = player.y;
                enemies[i].health = 100; // Boss health
                enemyActive[i] = true;
                bossSpawned = true;
                break;
            }
        }
    } else if (enemiesSpawned < totalEnemiesToSpawn) {
        int activeCount = 0;
        for(int i=0; i<MAX_ENEMIES; i++) if(enemyActive[i]) activeCount++;
        
        if (activeCount < 3 && random(0, 100) < 2) { // Spawn chance
            for(int i=0; i<MAX_ENEMIES; i++) {
                if (!enemyActive[i]) {
                    uint8_t eIdx = random(3, 9); // Enemy characters 3 to 8
                    Engine::initSkeleton(enemies[i], eIdx, camera.x + TO_FP(140), true);
                    enemies[i].y = TO_FP(random(60, 120));
                    enemies[i].health = 30;
                    enemies[i].walkSpeed = TO_FP(1);
                    enemyActive[i] = true;
                    enemiesSpawned++;
                    break;
                }
            }
        }
    }

    // Enemy AI
    for(int i=0; i<MAX_ENEMIES; i++) {
        if (!enemyActive[i]) continue;
        
        Skeleton &e = enemies[i];
        
        if (e.stateTimer > 0) e.stateTimer--;

        if (e.health <= 0) {
            if (e.state != CS_HITSTUN) {
                e.state = CS_HITSTUN; e.stateTimer = 30; // Death animation
            }
            if (e.stateTimer == 0) {
                enemyActive[i] = false;
                if (bossSpawned && e.charIdx == 9) bossDefeated = true;
                else enemiesDefeated++;
            }
            continue;
        }

        if (e.state == CS_HITSTUN) {
            e.x += e.vx; e.vx = (e.vx * 9) / 10;
            if (e.stateTimer == 0) e.state = CS_IDLE;
            continue;
        }

        if (e.state >= CS_PUNCH_STARTUP && e.state <= CS_KICK_RECOVERY) {
            if (e.state == CS_PUNCH_STARTUP && e.stateTimer == 0) { e.state = CS_PUNCH_ACTIVE; e.stateTimer = 5; }
            else if (e.state == CS_PUNCH_ACTIVE && e.stateTimer == 0) { e.state = CS_PUNCH_RECOVERY; e.stateTimer = 10; }
            else if (e.state == CS_PUNCH_RECOVERY && e.stateTimer == 0) e.state = CS_IDLE;
            
            if (e.state == CS_PUNCH_ACTIVE) {
                int32_t dx = FROM_FP(e.x - player.x);
                if (labs(dx) < 30 && labs(e.y - player.y) < TO_FP(15)) {
                    if ((e.facingLeft && dx > 0) || (!e.facingLeft && dx < 0)) {
                        if (player.state != CS_HITSTUN) triggerHit(e, player);
                    }
                }
            }
            continue;
        }

        // Movement towards player
        int32_t dx = player.x - e.x;
        int32_t dy = player.y - e.y;
        
        e.vx = 0; e.vy = 0;
        bool moving = false;
        
        if (labs(dx) > TO_FP(25)) { e.vx = (dx > 0) ? e.walkSpeed : -e.walkSpeed; moving = true; }
        if (labs(dy) > TO_FP(5)) { e.vy = (dy > 0) ? e.walkSpeed : -e.walkSpeed; moving = true; }
        
        e.facingLeft = (dx < 0);
        
        // Attack
        if (labs(dx) < TO_FP(35) && labs(dy) < TO_FP(15) && random(0, 30) == 0) {
            e.state = CS_PUNCH_STARTUP; e.stateTimer = 8;
            moving = false; e.vx = 0; e.vy = 0;
        }
        
        if (moving && e.state == CS_IDLE) e.state = CS_WALK;
        else if (!moving && e.state == CS_WALK) e.state = CS_IDLE;
        
        e.x += e.vx; e.y += e.vy;
    }
}
#include "SorGame.h"

void SorGame::updateFight() {
    if (shakeTimer > 0) shakeTimer--;
    updatePlayer();
    updateEnemies();
    
    if (player.health == 0) {
        if (delayTimer == 0) delayTimer = 120;
        delayTimer--;
        if (delayTimer == 0) currentState = STATE_RESULTS;
    } else if (bossDefeated) {
        if (delayTimer == 0) delayTimer = 120;
        delayTimer--;
        if (delayTimer == 0) {
            currentStage++;
            if (currentStage > 4) currentState = STATE_ENDING;
            else currentState = STATE_STAGE_CLEAR;
        }
    }
}

void SorGame::drawBackground() {
    // Simple scrolling grid or lines based on camera.x
    int16_t camOffsetX = FROM_FP(camera.x) % 40;
    
    for(int i = 0; i < 160; i += 40) {
        Engine::drawScaledLine(arduboy, TO_FP(i - camOffsetX + FROM_FP(camera.x)), LANE_MIN_Y, TO_FP(i - camOffsetX + FROM_FP(camera.x)), LANE_MAX_Y, camera, shakeTimer);
    }
    
    Engine::drawScaledLine(arduboy, camera.x - TO_FP(20), LANE_MIN_Y, camera.x + TO_FP(150), LANE_MIN_Y, camera, shakeTimer);
    Engine::drawScaledLine(arduboy, camera.x - TO_FP(20), LANE_MAX_Y, camera.x + TO_FP(150), LANE_MAX_Y, camera, shakeTimer);
}

void SorGame::drawFight() {
    drawBackground();

    // Collect all active skeletons
    Skeleton* skels[MAX_ENEMIES + 1];
    int count = 0;
    skels[count++] = &player;
    for(int i=0; i<MAX_ENEMIES; i++) {
        if (enemyActive[i]) skels[count++] = &enemies[i];
    }
    
    // Sort by Y depth
    for(int i=0; i<count-1; i++) {
        for(int j=i+1; j<count; j++) {
            if (skels[i]->y > skels[j]->y) {
                Skeleton* temp = skels[i];
                skels[i] = skels[j];
                skels[j] = temp;
            }
        }
    }
    
    // Draw shadows and skeletons
    for(int i=0; i<count; i++) {
        Skeleton* s = skels[i];
        if (s->state == CS_HITSTUN && (s->stateTimer % 4) < 2) continue; // flicker
        
        Engine::drawScaledCircle(arduboy, s->x, s->y, 10, camera, shakeTimer);
        Engine::updateSkeleton(*s, poses[s->state], arduboy.frameCount, s->state);
        Engine::drawSkeleton(arduboy, *s, camera, shakeTimer);
    }

    // HUD
    arduboy.setCursor(0, 0);
    arduboy.print(F("HP:")); arduboy.print(player.health);
    
    arduboy.setCursor(80, 0);
    if (bossSpawned) arduboy.print(F("BOSS"));
    else { arduboy.print(F("GO->")); }
}

void SorGame::drawMenu() {
    arduboy.drawBitmap(0, 0, logo + 2, 128, 32, WHITE);
    arduboy.setCursor(20, 45);
    if ((arduboy.frameCount / 30) % 2) arduboy.print(F("PRESS A TO START"));
    
    if (arduboy.justPressed(A_BUTTON)) {
        currentState = STATE_CHAR_SELECT;
        currentStage = 0;
    }
}

void SorGame::drawCharSelect() {
    arduboy.setCursor(15, 5);
    arduboy.print(F("SELECT FIGHTER"));
    
    CharacterData d; memcpy_P(&d, &roster[selectedChar], sizeof(CharacterData));
    arduboy.setCursor(10, 20); arduboy.print(F("< ")); arduboy.print(d.name); arduboy.print(F(" >"));
    
    Engine::initSkeleton(player, selectedChar, camera.x + TO_FP(64), false);
    player.y = TO_FP(96);
    camera.y = TO_FP(48);
    Engine::updateSkeleton(player, poses[CS_IDLE], arduboy.frameCount, CS_IDLE);
    Engine::drawSkeleton(arduboy, player, camera, 0);
    
    if (arduboy.justPressed(LEFT_BUTTON) && selectedChar > 0) selectedChar--;
    if (arduboy.justPressed(RIGHT_BUTTON) && selectedChar < 2) selectedChar++; // Only 3 playable characters
    
    if (arduboy.justPressed(A_BUTTON)) {
        currentState = STATE_STAGE_INTRO;
    }
}

void SorGame::drawStageIntro() {
    arduboy.setCursor(30, 25);
    arduboy.print(F("STAGE ")); arduboy.print(currentStage + 1);
    
    if (delayTimer == 0) delayTimer = 120;
    delayTimer--;
    
    if (delayTimer == 0 || arduboy.justPressed(A_BUTTON)) {
        resetStage();
        currentState = STATE_PLAYING;
    }
}

void SorGame::drawStageClear() {
    arduboy.setCursor(20, 25);
    arduboy.print(F("STAGE CLEAR!"));
    if (arduboy.justPressed(A_BUTTON)) currentState = STATE_STAGE_INTRO;
}

void SorGame::drawEnding() {
    arduboy.setCursor(20, 20);
    arduboy.print(F("YOU SAVED THE CITY!"));
    arduboy.setCursor(20, 40);
    if ((arduboy.frameCount / 30) % 2) arduboy.print(F("PRESS A"));
    if (arduboy.justPressed(A_BUTTON)) currentState = STATE_TITLE;
}

void SorGame::loop() {
    if (!arduboy.nextFrame()) return;
    arduboy.pollButtons();
    arduboy.clear();
    updateInputBuffer();
    
    switch(currentState) {
        case STATE_TITLE: drawMenu(); break;
        case STATE_CHAR_SELECT: drawCharSelect(); break;
        case STATE_STAGE_INTRO: drawStageIntro(); break;
        case STATE_PLAYING: updateFight(); drawFight(); break;
        case STATE_STAGE_CLEAR: drawStageClear(); break;
        case STATE_ENDING: drawEnding(); break;
        case STATE_RESULTS:
            arduboy.setCursor(30, 25);
            arduboy.print(F("GAME OVER"));
            if (arduboy.justPressed(A_BUTTON)) currentState = STATE_TITLE;
            break;
        default: break;
    }
    
    arduboy.display();
}}