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
    delayTimer = 0;
    totalEnemiesToSpawn = 10 + (currentStage * 5);
    bossSpawned = false;
    bossDefeated = false;
    
    for(int i=0; i<MAX_ENEMIES; i++) enemyActive[i] = false;
    memset(enemies, 0, sizeof(enemies));
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

    // Spawn protection: cannot hit enemies while they are off-screen
    int16_t dsx = FROM_FP(defender.x - camera.x) + 64;
    if (dsx < 5 || dsx > 123) return;

    CharacterData ad; memcpy_P(&ad, &roster[attacker.charIdx], sizeof(CharacterData));
    CharacterData dd; memcpy_P(&dd, &roster[defender.charIdx], sizeof(CharacterData));

    int16_t dmg = 10;
    if (attacker.state == CS_PUNCH_ACTIVE) dmg = 8;
    else if (attacker.state == CS_KICK_ACTIVE) dmg = 12;

    // Combo Scaling
    attacker.comboCount++;
    if (attacker.comboCount > 3) attacker.comboCount = 1;
    dmg = (dmg * (8 + attacker.comboCount)) / 10; // 90%, 100%, 110% damage

    // Scale by strength
    dmg = (dmg * ad.strength) / 60;
    uint8_t attackerSize = Engine::getSize(attacker.charIdx);
    dmg = (int16_t)dmg * attackerSize / 62;
    dmg = (dmg * 70) / (30 + dd.vitality);

    if (attacker.isBoss && &attacker != &player) dmg = (dmg * 15) / 10; // Bosses deal 50% more base damage
    
    // Scale enemy damage by 10% per stage (+40% base start)
    if (&attacker != &player) {
        dmg = (dmg * (14 + currentStage)) / 10;
    }

    // Knockback
    int16_t kb = TO_FP(2) + (attacker.comboCount * TO_FP(1));
    if (attacker.isBoss && &attacker != &player) kb += TO_FP(2); // Bosses have higher knockback
    
    if (attacker.x < defender.x) { defender.vx = kb; attacker.vx = -kb/4; } 
    else { defender.vx = -kb; attacker.vx = kb/4; }
    
    defender.health -= dmg; 
    defender.state = CS_HITSTUN; 
    defender.stateTimer = 15 + (attacker.comboCount * 5);
    
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
    if (freezeTimer > 0 || currentState == STATE_BOSS_INTRO) { 
        if (freezeTimer > 0) freezeTimer--; 
        if (currentState == STATE_BOSS_INTRO) player.state = CS_IDLE;
        return; 
    }

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
                if (abs(dx) < 30 && abs(player.y - enemies[i].y) < TO_FP(7)) {
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

    if (moving && player.state == CS_IDLE) { player.state = CS_WALK; player.comboCount = 0; }
    else if (!moving && player.state == CS_WALK) { player.state = CS_IDLE; player.comboCount = 0; }

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

    // Screen Bounds check (Keep on screen + Bounce logic for player)
    int32_t minX = camera.x - TO_FP(60);
    int32_t maxX = camera.x + TO_FP(60);
    if (player.x < minX) { 
        player.x = minX; 
        if (player.state == CS_HITSTUN) player.vx = abs(player.vx); // Bounce
    }
    if (player.x > maxX) { 
        player.x = maxX; 
        if (player.state == CS_HITSTUN) player.vx = -abs(player.vx); // Bounce
    }
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
                    currentBossCharIdx = pgm_read_byte(&stage_bosses[currentStage]);
                    Engine::initSkeleton(enemies[i], currentBossCharIdx, camera.x + TO_FP(140), true);
                    enemies[i].y = TO_FP(64);
                    enemies[i].health = 140 + (currentStage * 25); // Starts at 140, scales faster
                    enemies[i].walkSpeed = (enemies[i].walkSpeed * (14 + currentStage)) / 10;
                    enemies[i].aiBehavior = 1; // Bosses are always aggressive
                    enemies[i].isBoss = true;
                    enemyActive[i] = true;
                    bossSpawned = true;
                    bossDefeated = false; // Reset just in case
                    enemiesRemainingInEncounter = 1;
                    currentState = STATE_BOSS_INTRO;
                    return; // Stop processing this frame to avoid logic collisions
                }
            }
        } else {
            // Standard Group Encounter
            uint8_t count = 3 + (currentStage / 2); // Start at 3 enemies, up to 5
            if (count > MAX_ENEMIES) count = MAX_ENEMIES;
            
            for(uint8_t i=0; i<count; i++) {
                if (!enemyActive[i]) {
                    uint8_t eIdx = random(3, 9);
                    // Half from left, half from right - off screen but close
                    int32_t spawnX = (i % 2 == 0) ? camera.x - TO_FP(75) : camera.x + TO_FP(75);
                    Engine::initSkeleton(enemies[i], eIdx, spawnX, (spawnX > player.x));
                    enemies[i].y = TO_FP(random(48, 80));
                    enemies[i].health = 30;
                    // Starting difficulty +40%, then scale 10% per stage
                    enemies[i].walkSpeed = (TO_FP(1.3) * (14 + currentStage)) / 10; 
                    enemies[i].aiTimer = (uint8_t)((10 + (i * 10)) * (6 - currentStage)) / 10;
                    if (enemies[i].aiTimer < 2) enemies[i].aiTimer = 2; // Min floor
                    enemies[i].aiBehavior = (random(0, 100) < 70) ? 1 : 0; // 70% Aggressive
                    enemies[i].isBoss = false;
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

                if (!enemies[i].isBoss) enemiesDefeated++;
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
                if (abs(dx) < 18 && abs(e.y - player.y) < TO_FP(7)) {
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
            int32_t attackDistX = TO_FP(40 - currentStage * 4);
            int32_t attackDistY = TO_FP(7);

            // Count attackers
            int attackers = 0;
            for(int j=0; j<MAX_ENEMIES; j++) {
                if (j != i && enemyActive[j] && enemies[j].health > 0) {
                    if (abs(player.x - enemies[j].x) < attackDistX && abs(player.y - enemies[j].y) < (attackDistY + TO_FP(5))) {
                        attackers++;
                    }
                }
            }

            bool isClose = (abs(dx) < attackDistX && abs(dy) < attackDistY);
            
            if (isClose) {
                uint8_t timerScale = (e.aiBehavior == 1) ? 3 : 6; // Aggressive wait significantly less
                if (attackers >= 2 && random(0, 100) < (40 - currentStage * 10)) {
                    // Too crowded, back off slightly
                    e.vx = (dx > 0) ? -e.walkSpeed : e.walkSpeed;
                    e.vy = (dy > 0) ? -e.walkSpeed : e.walkSpeed;
                    moving = true;
                    e.aiTimer = (timerScale * 2 * (6 - currentStage)) / 10;
                } else if (random(0, (25 - currentStage * 5)) == 0) { // Attack probability
                    e.state = CS_PUNCH_STARTUP; e.stateTimer = 12; 
                    moving = false; e.vx = 0; e.vy = 0;
                    e.aiTimer = (timerScale * 3 * (6 - currentStage)) / 10; 
                } else {
                    e.aiTimer = (timerScale * (6 - currentStage)) / 10; 
                }
            } else {
                // Move towards player
                int32_t targetX = player.x;
                int32_t targetY = player.y;

                if (e.aiBehavior == 0) { // Balanced tries to flank
                    int32_t flankDistX = TO_FP(30 - currentStage * 3);
                    int32_t flankDistY = TO_FP(8 - (currentStage / 2));

                    if (i % 2 == 0) targetX -= flankDistX;
                    else targetX += flankDistX;
                    
                    if (i % 3 == 0) targetY -= flankDistY;
                    else targetY += flankDistY;
                }
                // Aggressive (behavior 1) goes straight for targetX/targetY (player)

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

        // Screen edge bounce
        int32_t eminX = camera.x - TO_FP(62);
        int32_t emaxX = camera.x + TO_FP(62);
        if (e.x < eminX) { 
            e.x = eminX; 
            if (e.state == CS_HITSTUN) e.vx = abs(e.vx); 
        }
        if (e.x > emaxX) { 
            e.x = emaxX; 
            if (e.state == CS_HITSTUN) e.vx = -abs(e.vx); 
        }
    }
}
#include "SorGame.h"

void SorGame::updateFight() {
    if (shakeTimer > 0) shakeTimer--;
    if (pickupTimer > 0) pickupTimer--;

    updatePlayer();
    updateEnemies();

    // Explicit boss defeat check
    if (bossSpawned && !bossDefeated) {
        for(int i=0; i<MAX_ENEMIES; i++) {
            if (enemyActive[i] && enemies[i].isBoss && enemies[i].health <= 0) {
                bossDefeated = true;
            }
        }
    }

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
        if (delayTimer == 0) delayTimer = 60;
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
            arduboy.drawLine(bx, 20, bx + 30, 5, WHITE); arduboy.drawLine(bx + 30, 5, bx + 60, 20, WHITE);
            arduboy.drawLine(bx + 50, 15, bx + 80, 0, WHITE); arduboy.drawLine(bx + 80, 0, bx + 120, 20, WHITE);
        }
        int16_t p2 = (camX / 2) % 64;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 64) - p2;
            arduboy.fillRect(bx, 10, 20, 22, BLACK);
            arduboy.drawRect(bx, 10, 20, 22, WHITE); 
            arduboy.fillRect(bx + 25, 15, 30, 17, BLACK);
            arduboy.drawRect(bx + 25, 15, 30, 17, WHITE);
        }
        int16_t p1 = camX % 80;
        for (int i = -1; i < 3; i++) {
            int16_t lx = (i * 80) - p1;
            arduboy.drawFastVLine(lx, 5, 27, WHITE); arduboy.drawFastHLine(lx - 5, 5, 11, WHITE);
        }
    } else if (currentStage == 1) { // PARK
        int16_t p2 = (camX / 2) % 64;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 64) - p2;
            arduboy.fillCircle(bx + 15, 15, 10, BLACK);
            arduboy.drawCircle(bx + 15, 15, 10, WHITE); 
            arduboy.drawFastVLine(bx + 15, 25, 7, WHITE);
            arduboy.fillCircle(bx + 45, 12, 8, BLACK);
            arduboy.drawCircle(bx + 45, 12, 8, WHITE); 
            arduboy.drawFastVLine(bx + 45, 20, 12, WHITE);
        }
        int16_t p1 = camX % 100;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 100) - p1;
            arduboy.fillRect(bx + 10, 25, 20, 5, BLACK);
            arduboy.drawRect(bx + 10, 25, 20, 5, WHITE); // Bench
            arduboy.drawFastVLine(bx + 12, 30, 2, WHITE); arduboy.drawFastVLine(bx + 28, 30, 2, WHITE);
        }
    } else if (currentStage == 2) { // OPEN SPACE
        int16_t p4 = (camX / 4) % 128;
        for (int i = -1; i < 2; i++) {
            int16_t bx = (i * 128) - p4;
            arduboy.drawCircle(bx + 40, 5, 3, WHITE); // Bird/Cloud?
        }
        int16_t p2 = (camX / 2) % 80;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 80) - p2;
            arduboy.fillRect(bx, 20, 40, 12, BLACK);
            arduboy.drawRect(bx, 20, 40, 12, WHITE); // Low fence
            arduboy.drawFastVLine(bx + 10, 20, 12, WHITE); arduboy.drawFastVLine(bx + 30, 20, 12, WHITE);
        }
    } else if (currentStage == 3) { // DESERT
        int16_t p4 = (camX / 4) % 128;
        for (int i = -1; i < 2; i++) {
            int16_t bx = (i * 128) - p4;
            arduboy.drawLine(bx, 25, bx + 64, 15, WHITE); arduboy.drawLine(bx + 64, 15, bx + 128, 25, WHITE); // Dunes
        }
        int16_t p1 = camX % 90;
        for (int i = -1; i < 3; i++) {
            int16_t bx = (i * 90) - p1;
            arduboy.drawFastVLine(bx + 20, 15, 15, WHITE); arduboy.drawFastHLine(bx + 15, 20, 10, WHITE); // Cactus
            arduboy.fillCircle(bx + 60, 28, 4, BLACK);
            arduboy.drawCircle(bx + 60, 28, 4, WHITE); // Rock
        }
    } else { // CITY NIGHT
        int16_t p4 = (camX / 4) % 64;
        for (int i = -1; i < 4; i++) {
            int16_t bx = (i * 64) - p4;
            arduboy.fillRect(bx + 5, 5, 15, 27, BLACK);
            arduboy.drawRect(bx + 5, 5, 15, 27, WHITE); // Skyscrapers
            arduboy.drawPixel(bx + 8, 8, WHITE); arduboy.drawPixel(bx + 12, 15, WHITE);
        }
        int16_t p1 = camX % 40;
        for (int i = -1; i < 5; i++) {
            int16_t lx = (i * 40) - p1;
            arduboy.drawFastVLine(lx, 15, 17, WHITE); // Light poles
            arduboy.fillCircle(lx, 15, 2, BLACK);
            arduboy.drawCircle(lx, 15, 2, WHITE);
        }
    }

    // Ground line separating background and lane
    arduboy.drawFastHLine(0, 32, 128, WHITE);

    // Ground lines (in the lane)
    int16_t camOffsetX = camX % 40;
    for(int i = -40; i < 160; i += 40) {
        Engine::drawScaledLine(arduboy, TO_FP(i - camOffsetX + camX), LANE_MIN_Y, TO_FP(i - camOffsetX + camX), LANE_MAX_Y, camera, shakeTimer);
    }
    Engine::drawScaledLine(arduboy, camera.x - TO_FP(64), LANE_MIN_Y, camera.x + TO_FP(150), LANE_MIN_Y, camera, shakeTimer);
    Engine::drawScaledLine(arduboy, camera.x - TO_FP(64), LANE_MAX_Y, camera.x + TO_FP(150), LANE_MAX_Y, camera, shakeTimer);
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
                case CS_WALK: pIdx = 1 + (tickCount / 8) % 4; break;
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
        Engine::updateSkeleton(*s, poses[pIdx], tickCount, pIdx);
        Engine::drawSkeleton(arduboy, *s, tempCam, shakeTimer);

        // Enemy health bars (also scaled)
        if (s != &player) {
            int16_t hx = (int16_t)((((s->x - camera.x) * tempCam.zoom) / 100) >> FP_SHIFT) + 64 - 5;
            int16_t hy = (int16_t)((((s->y - camera.y) * tempCam.zoom) / 100) >> FP_SHIFT) + 32 - (35 * z / 100);
            arduboy.drawRect(hx, hy, 12, 3, WHITE);
            int8_t bw = (s->health * 10) / (s->charIdx == currentBossCharIdx ? 100 : 30);
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
            arduboy.setCursor(45, 45);
            arduboy.print(F("GO... ->"));
        }
    }
}

void SorGame::drawMenu() {
    arduboy.drawBitmap(0, 0, logo + 2, 128, 32, WHITE);
    arduboy.setCursor(20, 45);
    if ((arduboy.frameCount / 30) % 2) arduboy.print(F("PRESS A TO START"));
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

    // Render character
    Engine::drawSkeleton(arduboy, player, camera, 0);
}

void SorGame::drawCharIntro() {
    CharacterData d; memcpy_P(&d, &roster[selectedChar], sizeof(CharacterData));
    arduboy.setCursor(0, 0);
    arduboy.print(d.name);
    arduboy.print(F(" STORY:"));
    
    arduboy.setCursor(0, 15);
    char buffer[100];
    strcpy_P(buffer, (char*)pgm_read_ptr(&intros[selectedChar]));
    arduboy.print(buffer);
}
void SorGame::drawStageIntro() {
    arduboy.setCursor(43, 20);
    arduboy.print(F("STAGE ")); arduboy.print(currentStage + 1);
    
    char buffer[20];
    strcpy_P(buffer, (char*)pgm_read_ptr(&stage_names[currentStage]));
    uint8_t len = strlen(buffer);
    arduboy.setCursor(64 - (len * 3), 35);
    arduboy.print(buffer);
}

void SorGame::drawStageClear() {
    arduboy.setCursor(20, 0);
    arduboy.print(F("STAGE CLEAR!"));
    
    arduboy.setCursor(0, 15);
    char buffer[100];
    strcpy_P(buffer, (char*)pgm_read_ptr(&stage_clears[currentStage-1]));
    arduboy.print(buffer);

    arduboy.setCursor(20, 55);
    if ((arduboy.frameCount / 30) % 2) arduboy.print(F("PRESS A"));
}

void SorGame::drawEnding() {
    arduboy.setCursor(0, 0);
    arduboy.print(F("VICTORY!"));
    
    arduboy.setCursor(0, 15);
    char buffer[100];
    strcpy_P(buffer, (char*)pgm_read_ptr(&endings[selectedChar]));
    arduboy.print(buffer);

    arduboy.setCursor(20, 55);
    if ((arduboy.frameCount / 30) % 2) arduboy.print(F("PRESS A"));
}

void SorGame::drawBossIntro() {
    for(int i=0; i<MAX_ENEMIES; i++) {
        if (enemyActive[i] && enemies[i].charIdx == currentBossCharIdx) {
            int32_t targetX = camera.x + TO_FP(40);
            if (enemies[i].x > targetX) {
                drawFight(); 
            } else {
                CharacterData d; memcpy_P(&d, &roster[currentBossCharIdx], sizeof(CharacterData));
                arduboy.setCursor(0, 0);
                arduboy.print(d.name);
                arduboy.print(F(":"));
                
                arduboy.setCursor(0, 15);
                char buffer[100];
                strcpy_P(buffer, (char*)pgm_read_ptr(&boss_talks[currentStage]));
                arduboy.print(buffer);
                
                arduboy.setCursor(20, 55);
                if ((arduboy.frameCount / 30) % 2) arduboy.print(F("PRESS A TO FIGHT"));
            }
        }
    }
}

void SorGame::update() {
    tickCount++;
    updateInputBuffer();

    switch(currentState) {
        case STATE_TITLE: updateMenu(); break;
        case STATE_CHAR_SELECT: updateCharSelect(); break;
        case STATE_CHAR_INTRO: updateCharIntro(); break;
        case STATE_STAGE_INTRO: updateStageIntro(); break;
        case STATE_PLAYING: updateFight(); break;
        case STATE_BOSS_INTRO: updateBossIntro(); break;
        case STATE_STAGE_CLEAR: updateStageClear(); break;
        case STATE_ENDING: updateEnding(); break;
        case STATE_RESULTS: updateResults(); break;
        default: break;
    }
}

void SorGame::updateMenu() {
    if (arduboy.justPressed(A_BUTTON)) {
        currentState = STATE_CHAR_SELECT;
        currentStage = 0;
        camera.x = 0;
        camera.y = TO_FP(48);
        Engine::initSkeleton(player, selectedChar, camera.x + TO_FP(35), false);
    }
}

void SorGame::updateCharSelect() {
    if (player.charIdx != selectedChar) {
        Engine::initSkeleton(player, selectedChar, camera.x + TO_FP(35), false);
    }
    player.y = TO_FP(74);
    camera.y = TO_FP(48);

    uint8_t pIdx = 1 + (tickCount / 12) % 4; 
    Engine::updateSkeleton(player, poses[pIdx], tickCount, pIdx);
    
    if (arduboy.justPressed(LEFT_BUTTON) && selectedChar > 0) selectedChar--;
    if (arduboy.justPressed(RIGHT_BUTTON) && selectedChar < 2) selectedChar++; 

    if (arduboy.justPressed(A_BUTTON)) {
        currentState = STATE_CHAR_INTRO;
        delayTimer = 0;
    }
}

void SorGame::updateCharIntro() {
    if (arduboy.justPressed(A_BUTTON)) {
        currentState = STATE_STAGE_INTRO;
    }
}

void SorGame::updateStageIntro() {
    if (delayTimer == 0) delayTimer = 120;
    delayTimer--;
    
    if (delayTimer == 0 || arduboy.justPressed(A_BUTTON)) {
        resetStage();
        currentState = STATE_PLAYING;
    }
}

void SorGame::updateBossIntro() {
    // 1. Scroll camera until player is on the left (sx ~ 30)
    // sx = (player.x - camera.x) + 64 => 30 = px - camX + 64 => camX = px + 34
    int32_t targetCamX = player.x + TO_FP(34);
    if (camera.x < targetCamX) {
        camera.x += TO_FP(1);
        if (camera.x > targetCamX) camera.x = targetCamX;
        player.state = CS_IDLE;
        player.facingLeft = false;
        return; // Wait for camera
    }

    // 2. Bring in the boss
    for(int i=0; i<MAX_ENEMIES; i++) {
        if (enemyActive[i] && enemies[i].isBoss) {
            int32_t targetX = camera.x + TO_FP(40); // sx ~ 104
            if (enemies[i].x > targetX) {
                enemies[i].x -= TO_FP(1);
                enemies[i].state = CS_WALK;
                enemies[i].facingLeft = true;
            } else {
                enemies[i].state = CS_IDLE;
                if (arduboy.justPressed(A_BUTTON)) {
                    currentState = STATE_PLAYING;
                    enemies[i].aiTimer = 30;
                }
            }
        }
    }
}

void SorGame::updateStageClear() {
    if (arduboy.justPressed(A_BUTTON)) currentState = STATE_STAGE_INTRO;
}

void SorGame::updateEnding() {
    if (arduboy.justPressed(A_BUTTON)) currentState = STATE_TITLE;
}

void SorGame::updateResults() {
    if (arduboy.justPressed(A_BUTTON)) currentState = STATE_TITLE;
}

void SorGame::draw() {
    switch(currentState) {
        case STATE_TITLE: drawMenu(); break;
        case STATE_CHAR_SELECT: drawCharSelect(); break;
        case STATE_CHAR_INTRO: drawCharIntro(); break;
        case STATE_STAGE_INTRO: drawStageIntro(); break;
        case STATE_PLAYING: drawFight(); break;
        case STATE_BOSS_INTRO: drawBossIntro(); break;
        case STATE_STAGE_CLEAR: drawStageClear(); break;
        case STATE_ENDING: drawEnding(); break;
        case STATE_RESULTS:
            arduboy.setCursor(30, 25);
            arduboy.print(F("GAME OVER"));
            break;
        default: break;
    }
}

void SorGame::loop() {
    static uint32_t lastMillis = 0;
    static uint32_t accumulator = 0;
    const uint32_t timeStep = 1000 / 60; // 16.66ms per logic tick

    uint32_t currentMillis = millis();
    uint32_t elapsed = currentMillis - lastMillis;
    lastMillis = currentMillis;

    // Safety: don't accumulate too much if the app was suspended or severely lagged
    if (elapsed > 100) elapsed = 100;
    accumulator += elapsed;

    // Logic updates (fixed timestep)
    while (accumulator >= timeStep) {
        arduboy.pollButtons();
        update();
        accumulator -= timeStep;
    }

    // Render as fast as possible (or capped by Arduboy/SDL)
    if (!arduboy.nextFrame()) return;

    arduboy.clear();
    draw();
    arduboy.display();
}