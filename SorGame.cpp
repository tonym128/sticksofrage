#include "SorGame.h"

void SorGame::setup() {
    arduboy.begin();
    arduboy.setFrameRate(60);
    playerBuffer.head = 0;
    memset(playerBuffer.buttons, 0, INPUT_BUFFER_SIZE);
    for(int i=0; i<MAX_ENEMIES; i++) enemyActive[i] = false;
}

void SorGame::resetStage() {
    CharacterData d; memcpy_P(&d, &roster[selectedChar], sizeof(CharacterData));
    Engine::initSkeleton(player, selectedChar, TO_FP(100), false);
    player.y = TO_FP(64);
    player.health = d.vitality;
    
    stageProgress = 0;
    maxCameraX = 36; // Initial camera center to put player at sx=100 initially
    nextEncounterX = TO_FP(200); 
    enemiesRemainingInEncounter = 0;
    enemiesDefeated = 0;
    enemiesSpawned = 0;
    goPromptTimer = 0;
    totalEnemiesToSpawn = 10 + (currentStage * 5);
    bossSpawned = false;
    bossDefeated = false;
    
    for(int i=0; i<MAX_ENEMIES; i++) enemyActive[i] = false;
    for(int i=0; i<MAX_PICKUPS; i++) pickups[i].active = false;
    pickupTimer = 0;
    camera.x = TO_FP(maxCameraX);
    camera.y = TO_FP(48);
    camera.zoom = 100;
}

void SorGame::triggerHit(Skeleton &attacker, Skeleton &defender) {
    shakeTimer = 8; freezeTimer = 5;
    
    // Depth check! If they are on different Y planes, no hit
    if (abs(attacker.y - defender.y) > TO_FP(15)) return;

    CharacterData ad; memcpy_P(&ad, &roster[attacker.charIdx], sizeof(CharacterData));
    CharacterData dd; memcpy_P(&dd, &roster[defender.charIdx], sizeof(CharacterData));

    int16_t dmg = 10;
    if (attacker.state == CS_PUNCH_ACTIVE) dmg = 8;
    else if (attacker.state == CS_KICK_ACTIVE) dmg = 12;

    // Scale by strength (ad.strength is 1-100, base is ~60)
    dmg = (dmg * ad.strength) / 60;
    
    // Scale by size
    uint8_t attackerSize = Engine::getSize(attacker.charIdx);
    dmg = (int16_t)dmg * attackerSize / 62;

    // Reduce by defender vitality (dd.vitality is 1-100, high vitality takes less % damage)
    // Actually, vitality is better used as HP. 
    // Let's use it as a defense factor too:
    dmg = (dmg * 70) / (30 + dd.vitality);

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
                if (abs(dx) < 30 && abs(player.y - enemies[i].y) < TO_FP(15)) {
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

    // Lane Bounds check
    if (player.y < LANE_MIN_Y) player.y = LANE_MIN_Y;
    if (player.y > LANE_MAX_Y) player.y = LANE_MAX_Y;

    if (goPromptTimer > 0) goPromptTimer--;

    // Camera follow (Trigger when in the right 20% of screen: sx > 102)
    // 102 = (x - camX) + 64  => camX = x - 38
    int32_t targetCamX = player.x - TO_FP(38);
    if (enemiesRemainingInEncounter == 0) {
        if (targetCamX > camera.x) {
            int32_t diff = (targetCamX - camera.x) / 8; // Ease in
            if (diff < TO_FP(1)) diff = TO_FP(1); // Min speed
            camera.x += diff;
            if (camera.x > targetCamX) camera.x = targetCamX;
            maxCameraX = FROM_FP(camera.x);
            stageProgress = camera.x;
        }
    }

    // Screen Bounds check (Keep on screen)
    if (player.x < camera.x - TO_FP(60)) player.x = camera.x - TO_FP(60);
    if (player.x > camera.x + TO_FP(60)) player.x = camera.x + TO_FP(60);
}
#include "SorGame.h"

void SorGame::updateEnemies() {
    if (freezeTimer > 0) return;

    // Trigger next encounter
    if (enemiesRemainingInEncounter == 0 && stageProgress >= nextEncounterX && !bossSpawned) {
        if (stageProgress >= LEVEL_END_X) {
            // Final Boss Encounter
            for(int i=0; i<MAX_ENEMIES; i++) {
                if (!enemyActive[i]) {
                    Engine::initSkeleton(enemies[i], 9, camera.x + TO_FP(80), true);
                    enemies[i].y = TO_FP(64);
                    enemies[i].health = 100;
                    enemies[i].aiTimer = 60; 
                    enemyActive[i] = true;
                    bossSpawned = true;
                    enemiesRemainingInEncounter = 1;
                    break;
                }
            }
        } else {
            // Standard Group Encounter
            uint8_t count = 2 + (currentStage / 2); // 2-4 enemies
            if (count > MAX_ENEMIES) count = MAX_ENEMIES;
            
            for(uint8_t i=0; i<count; i++) {
                if (!enemyActive[i]) {
                    uint8_t eIdx = random(3, 9);
                    // Half from left, half from right - off screen but close
                    int32_t spawnX = (i % 2 == 0) ? camera.x - TO_FP(75) : camera.x + TO_FP(75);
                    Engine::initSkeleton(enemies[i], eIdx, spawnX, (spawnX > player.x));
                    enemies[i].y = TO_FP(random(48, 80));
                    enemies[i].health = 30;
                    enemies[i].walkSpeed = TO_FP(1.5); // Faster enemies
                    enemies[i].aiTimer = 10 + (i * 10); // Faster reaction
                    enemyActive[i] = true;
                    enemiesSpawned++;
                }
            }
            enemiesRemainingInEncounter = count;
            nextEncounterX += TO_FP(300); // Next one in 300px
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
                if (enemiesRemainingInEncounter > 0) {
                    enemiesRemainingInEncounter--;
                    if (enemiesRemainingInEncounter == 0 && !bossSpawned) goPromptTimer = 90;
                }
                
                // Drop pickup chance
                if (random(0, 100) < 20) { // 20% chance
                    for(int p=0; p<MAX_PICKUPS; p++) {
                        if (!pickups[p].active) {
                            pickups[p].x = e.x;
                            pickups[p].y = e.y;
                            pickups[p].active = true;
                            break;
                        }
                    }
                }

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
                if (abs(dx) < 30 && abs(e.y - player.y) < TO_FP(15)) {
                    if ((e.facingLeft && dx > 0) || (!e.facingLeft && dx < 0)) {
                        if (player.state != CS_HITSTUN) triggerHit(e, player);
                    }
                }
            }
            continue;
        }

        // AI Logic
        int32_t dx = player.x - e.x;
        int32_t dy = player.y - e.y;
        
        e.vx = 0; e.vy = 0;
        bool moving = false;

        // Slow to react: wait around sometimes
        if (e.aiTimer > 0) {
            e.aiTimer--;
        } else {
            // Count attackers
            int attackers = 0;
            for(int j=0; j<MAX_ENEMIES; j++) {
                if (j != i && enemyActive[j] && enemies[j].health > 0) {
                    if (abs(player.x - enemies[j].x) < TO_FP(40) && abs(player.y - enemies[j].y) < TO_FP(20)) {
                        attackers++;
                    }
                }
            }

            bool isClose = (abs(dx) < TO_FP(40) && abs(dy) < TO_FP(15));
            
            if (isClose) {
                if (attackers >= 2 && random(0, 100) < 60) {
                    // Too crowded, back off slightly
                    e.vx = (dx > 0) ? -e.walkSpeed : e.walkSpeed;
                    e.vy = (dy > 0) ? -e.walkSpeed : e.walkSpeed;
                    moving = true;
                    e.aiTimer = 20;
                } else if (random(0, 40) == 0) { // Slower attacks
                    e.state = CS_PUNCH_STARTUP; e.stateTimer = 12; // Slower startup
                    moving = false; e.vx = 0; e.vy = 0;
                    e.aiTimer = 30; // Wait after attack
                } else {
                    e.aiTimer = 10; // Wait menacingly
                }
            } else {
                // Move towards player, try to flank
                int32_t targetX = player.x;
                int32_t targetY = player.y;

                if (i % 2 == 0) targetX -= TO_FP(30);
                else targetX += TO_FP(30);
                
                if (i % 3 == 0) targetY -= TO_FP(10);
                else targetY += TO_FP(10);

                if (targetY < LANE_MIN_Y) targetY = LANE_MIN_Y;
                if (targetY > LANE_MAX_Y) targetY = LANE_MAX_Y;

                int32_t tdx = targetX - e.x;
                int32_t tdy = targetY - e.y;

                if (abs(tdx) > TO_FP(5)) { e.vx = (tdx > 0) ? e.walkSpeed : -e.walkSpeed; moving = true; }
                if (abs(tdy) > TO_FP(5)) { e.vy = (tdy > 0) ? e.walkSpeed : -e.walkSpeed; moving = true; }

                e.aiTimer = random(5, 20); // Hold this trajectory briefly
            }
        }
        
        e.facingLeft = (player.x < e.x); // Always face player
        
        if (moving && e.state == CS_IDLE) e.state = CS_WALK;
        else if (!moving && e.state == CS_WALK) e.state = CS_IDLE;
        
        e.x += e.vx; e.y += e.vy;
    }
}
#include "SorGame.h"

void SorGame::updateFight() {
    if (shakeTimer > 0) shakeTimer--;
    if (pickupTimer > 0) pickupTimer--;

    updatePlayer();
    updateEnemies();

    // Pickup collection
    if (player.health > 0) {
        for(int i=0; i<MAX_PICKUPS; i++) {
            if (pickups[i].active) {
                if (abs(player.x - pickups[i].x) < TO_FP(15) && abs(player.y - pickups[i].y) < TO_FP(10)) {
                    player.health += 25;
                    if (player.health > 100) player.health = 100;
                    pickups[i].active = false;
                    pickupTimer = 30; // 30 frames of ducking
                }
            }
        }
    }
    
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
    int16_t camX = FROM_FP(camera.x);

    if (currentStage == 0) { // CITY
        int16_t p4 = (camX / 4) % 128;
        for (int i = -1; i < 2; i++) {
            int16_t bx = (i * 128) - p4;
            arduboy.drawLine(bx, 20, bx + 30, 5); arduboy.drawLine(bx + 30, 5, bx + 60, 20);
            arduboy.drawLine(bx + 50, 15, bx + 80, 0); arduboy.drawLine(bx + 80, 0, bx + 120, 20);
        }
        int16_t p2 = (camX / 2) % 64;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 64) - p2;
            arduboy.drawRect(bx, 10, 20, 22); arduboy.drawRect(bx + 25, 15, 30, 17);
        }
        int16_t p1 = camX % 80;
        for (int i = -1; i < 3; i++) {
            int16_t lx = (i * 80) - p1;
            arduboy.drawFastVLine(lx, 5, 27); arduboy.drawFastHLine(lx - 5, 5, 11);
        }
    } else if (currentStage == 1) { // PARK
        int16_t p2 = (camX / 2) % 64;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 64) - p2;
            arduboy.drawCircle(bx + 15, 15, 10); arduboy.drawFastVLine(bx + 15, 25, 7); // Tree
            arduboy.drawCircle(bx + 45, 12, 8); arduboy.drawFastVLine(bx + 45, 20, 12); // Tree
        }
        int16_t p1 = camX % 100;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 100) - p1;
            arduboy.drawRect(bx + 10, 25, 20, 5); // Bench
            arduboy.drawFastVLine(bx + 12, 30, 2); arduboy.drawFastVLine(bx + 28, 30, 2);
        }
    } else if (currentStage == 2) { // OPEN SPACE
        int16_t p4 = (camX / 4) % 128;
        for (int i = -1; i < 2; i++) {
            int16_t bx = (i * 128) - p4;
            arduboy.drawCircle(bx + 40, 5, 3); // Bird/Cloud?
        }
        int16_t p2 = (camX / 2) % 80;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 80) - p2;
            arduboy.drawRect(bx, 20, 40, 12); // Low fence
            arduboy.drawFastVLine(bx + 10, 20, 12); arduboy.drawFastVLine(bx + 30, 20, 12);
        }
    } else if (currentStage == 3) { // DESERT
        int16_t p4 = (camX / 4) % 128;
        for (int i = -1; i < 2; i++) {
            int16_t bx = (i * 128) - p4;
            arduboy.drawLine(bx, 25, bx + 64, 15); arduboy.drawLine(bx + 64, 15, bx + 128, 25); // Dunes
        }
        int16_t p1 = camX % 90;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 90) - p1;
            arduboy.drawFastVLine(bx + 20, 15, 15); arduboy.drawFastHLine(bx + 15, 20, 10); // Cactus
            arduboy.drawCircle(bx + 60, 28, 4); // Rock
        }
    } else { // CITY NIGHT
        int16_t p4 = (camX / 4) % 64;
        for (int i = -1; i < 4; i++) {
            int16_t bx = (i * 64) - p4;
            arduboy.drawRect(bx + 5, 5, 15, 27); // Skyscrapers
            arduboy.drawPixel(bx + 8, 8); arduboy.drawPixel(bx + 12, 15);
        }
        int16_t p1 = camX % 40;
        for (int i = -1; i < 5; i++) {
            int16_t lx = (i * 40) - p1;
            arduboy.drawFastVLine(lx, 15, 17); // Light poles
            arduboy.drawCircle(lx, 15, 2);
        }
    }

    // Ground line separating background and lane
    arduboy.drawFastHLine(0, 32, 128);

    // Ground lines (in the lane)
    int16_t camOffsetX = camX % 40;
    for(int i = 0; i < 160; i += 40) {
        Engine::drawScaledLine(arduboy, TO_FP(i - camOffsetX + camX), LANE_MIN_Y, TO_FP(i - camOffsetX + camX), LANE_MAX_Y, camera, shakeTimer);
    }
    Engine::drawScaledLine(arduboy, camera.x - TO_FP(20), LANE_MIN_Y, camera.x + TO_FP(150), LANE_MIN_Y, camera, shakeTimer);
    Engine::drawScaledLine(arduboy, camera.x - TO_FP(20), LANE_MAX_Y, camera.x + TO_FP(150), LANE_MAX_Y, camera, shakeTimer);
}

void SorGame::drawFight() {
    drawBackground();

    // Draw pickups
    for(int i=0; i<MAX_PICKUPS; i++) {
        if (pickups[i].active) {
            int16_t sx = FROM_FP(pickups[i].x - camera.x) + 64;
            int16_t sy = FROM_FP(pickups[i].y - camera.y) + 32;
            arduboy.drawRect(sx-3, sy-3, 7, 7);
            arduboy.drawFastHLine(sx-1, sy, 3);
            arduboy.drawFastVLine(sx, sy-1, 3);
        }
    }

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
        
        uint8_t pIdx = 0;
        if (s == &player && pickupTimer > 0) {
            pIdx = 8; // DUCK animation when picking up
        } else {
            switch(s->state) {
                case CS_IDLE: pIdx = 0; break;
                case CS_WALK: pIdx = 1 + (arduboy.frameCount / 8) % 4; break;
                case CS_BLOCK: pIdx = 5; break;
                case CS_PUNCH_STARTUP:
                case CS_PUNCH_ACTIVE:
                case CS_PUNCH_RECOVERY: pIdx = 6; break;
                case CS_KICK_STARTUP:
                case CS_KICK_ACTIVE:
                case CS_KICK_RECOVERY: pIdx = 7; break;
                case CS_HITSTUN: pIdx = 9; break;
                default: pIdx = 0; break;
            }
        }

        // Depth-based scaling (pseudo-perspective)
        // LANE_MIN_Y (48) -> 80% zoom, LANE_MAX_Y (80) -> 120% zoom (40% total range)
        int16_t z = 80 + ((FROM_FP(s->y) - FROM_FP(LANE_MIN_Y)) * 40) / (FROM_FP(LANE_MAX_Y) - FROM_FP(LANE_MIN_Y));
        Camera tempCam = camera;
        tempCam.zoom = (camera.zoom * z) / 100;

        Engine::drawScaledCircle(arduboy, s->x, s->y, (10 * z) / 100, tempCam, shakeTimer);
        Engine::updateSkeleton(*s, poses[pIdx], arduboy.frameCount, pIdx);
        Engine::drawSkeleton(arduboy, *s, tempCam, shakeTimer);

        // Enemy health bars (also scaled)
        if (s != &player) {
            int16_t hx = (int16_t)((((s->x - camera.x) * tempCam.zoom) / 100) >> FP_SHIFT) + 64 - 5;
            int16_t hy = (int16_t)((((s->y - camera.y) * tempCam.zoom) / 100) >> FP_SHIFT) + 32 - (35 * z / 100);
            arduboy.drawRect(hx, hy, 12, 3, WHITE);
            int8_t bw = (s->health * 10) / (s->charIdx == 9 ? 100 : 30);
            if (bw > 10) bw = 10;
            arduboy.fillRect(hx + 1, hy + 1, bw, 1, WHITE);
        }
    }

    // HUD
    arduboy.setCursor(0, 0);
    arduboy.print(F("HP:")); arduboy.print(player.health);

    // Progress Bar (Top Right)
    int16_t barX = 100, barY = 2, barW = 25, barH = 4;
    arduboy.drawRect(barX, barY, barW, barH, WHITE);
    int16_t filledW = (int32_t)(stageProgress * (barW - 2)) / LEVEL_END_X;
    if (filledW > barW - 2) filledW = barW - 2;
    if (filledW < 0) filledW = 0;
    arduboy.fillRect(barX + 1, barY + 1, filledW, barH - 2, WHITE);

    arduboy.setCursor(65, 0);
    if (bossSpawned) arduboy.print(F("BOSS"));
    else if (enemiesRemainingInEncounter > 0) arduboy.print(F("FIGHT!"));
    else { 
        arduboy.print(F("GO->")); 
        if (goPromptTimer > 0 && (goPromptTimer / 10) % 2) {
            arduboy.setCursor(45, 25);
            arduboy.print(F("GO... ->"));
        }
    }
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

    // Draw Stats
    const char* labels[] = {"STR", "SPD", "VIT"};
    uint8_t vals[] = {d.strength, (uint8_t)(FROM_FP(d.walkSpeed * 40)), d.vitality};
    for(int i=0; i<3; i++) {
        arduboy.setCursor(10, 35 + i*10);
        arduboy.print(labels[i]);
        arduboy.drawRect(35, 36 + i*10, 32, 5, WHITE);
        int8_t bw = (vals[i] * 30) / 100;
        if (bw > 30) bw = 30;
        arduboy.fillRect(36, 37 + i*10, bw, 3, WHITE);
    }

    // Move character further right and down
    if (player.charIdx != selectedChar) {
        Engine::initSkeleton(player, selectedChar, camera.x + TO_FP(35), false);
    }
    player.y = TO_FP(74);
    camera.y = TO_FP(48);
    
    // Slower walking animation
    uint8_t pIdx = 1 + (arduboy.frameCount / 12) % 4; 
    Engine::updateSkeleton(player, poses[pIdx], arduboy.frameCount, pIdx);
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
}