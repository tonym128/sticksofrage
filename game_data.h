#ifndef GAME_DATA_H
#define GAME_DATA_H

#ifdef ARDUINO
#include <Arduino.h>
#endif

// --- Fixed-Point Math Constants ---
#define FP_SHIFT 8
#define TO_FP(x) ((int32_t)((x) * 256L))
#define FROM_FP(x) ((int32_t)(x) >> 8)

// --- Trig Table ---
const int16_t SIN_TABLE[64] PROGMEM = {
    0, 6, 12, 18, 25, 31, 37, 43, 49, 56, 62, 68, 74, 80, 86, 92,
    97, 103, 109, 114, 120, 125, 130, 135, 140, 144, 149, 153, 157, 161, 165, 169,
    173, 176, 179, 182, 185, 188, 191, 193, 195, 197, 199, 200, 202, 203, 204, 205,
    206, 206, 207, 207, 207, 207, 207, 207, 206, 206, 205, 204, 203, 202, 200, 199
};

// --- Game States ---
enum GameState { STATE_TITLE, STATE_CHAR_SELECT, STATE_CHAR_INTRO, STATE_STAGE_INTRO, STATE_PLAYING, STATE_BOSS_INTRO, STATE_STAGE_CLEAR, STATE_ENDING, STATE_RESULTS };

// --- Combat States ---
enum CombatState { 
    CS_IDLE, CS_WALK, CS_BLOCK, CS_DUCK, 
    CS_PUNCH_STARTUP, CS_PUNCH_ACTIVE, CS_PUNCH_RECOVERY, 
    CS_KICK_STARTUP, CS_KICK_ACTIVE, CS_KICK_RECOVERY, 
    CS_HITSTUN, CS_PARRY_STUN, CS_SUPER_STARTUP, 
    CS_DUCK_PUNCH_STARTUP, CS_DUCK_PUNCH_ACTIVE, CS_DUCK_PUNCH_RECOVERY, 
    CS_DUCK_KICK_STARTUP, CS_DUCK_KICK_ACTIVE, CS_DUCK_KICK_RECOVERY,
    CS_SPECIAL_STARTUP, CS_SPECIAL_ACTIVE, CS_SPECIAL_RECOVERY
};

// --- Input Buffer ---
#define INPUT_BUFFER_SIZE 60
struct InputBuffer {
    uint8_t buttons[INPUT_BUFFER_SIZE];
    uint8_t head;
};

// --- Projectiles ---
struct Projectile {
    int32_t x, y, z;
    int16_t vx;
    bool active;
    bool ownerIsPlayer;
};
#define MAX_PROJECTILES 2

// --- AI States ---
enum AIState { AI_IDLE, AI_APPROACH, AI_RETREAT, AI_WAIT, AI_ATTACKING };

// --- Constants ---
#define GROUND_Y TO_FP(120)
#define GRAVITY TO_FP(0.2)
#define JUMP_IMPULSE TO_FP(-4.5)
#define ACCEL TO_FP(0.3)
#define FRICTION TO_FP(0.2)

// --- Face Data ---
struct FaceData { uint8_t headShape, hairStyle, eyeType, noseType, mouthType, browType; };

// --- Poses ---
struct Pose { uint8_t angles[6]; };

#include "poses.h"

// --- AI Profiles ---
enum AIProfile { AI_BALANCED, AI_RUSHDOWN, AI_ZONER, AI_TANK };

struct CharacterData { char name[8]; uint8_t lengths[6]; int16_t walkSpeed; FaceData face; AIProfile profile; uint8_t strength; uint8_t vitality; };

const char intro_0[] PROGMEM = "THE CITY HAS FALLEN\nTO CHAOS. ZENITH,\nA DISGRACED COP,\nTAKES THE LAW INTO\nHIS OWN HANDS.";
const char intro_1[] PROGMEM = "CINDER'S HOME WAS\nBURNED BY GANGS.\nWITH NOTHING LEFT,\nHE VOWS TO PURGE\nTHE STREETS BY FIRE.";
const char intro_2[] PROGMEM = "GOLIATH HAS SEEN\nTOO MANY INNOCENTS\nHURT. THIS GIANT\nWILL CRUSH ANY GANG\nIN HIS PATH.";

const char* const intros[] PROGMEM = { intro_0, intro_1, intro_2 };

const char end_0[] PROGMEM = "THE STREETS ARE SAFE.\nZENITH HAS CLEANED\nUP THE TRASH. LAW\nAND ORDER RETURN\nTO THE CITY.";
const char end_1[] PROGMEM = "THE FIRE OF REVENGE\nIS QUENCHED. CINDER\nSTANDS IN THE ASHES\nOF THE SYNDICATE.\nSTREETS ARE CLEAR.";
const char end_2[] PROGMEM = "GOLIATH'S MIGHT HAS\nRESTORED PEACE. NO\nGANG DARE SHOW ITS\nFACE AGAIN. THE\nCITY IS PROTECTED.";

const char* const endings[] PROGMEM = { end_0, end_1, end_2 };

const char boss_0[] PROGMEM = "YOU THINK YOU CAN\nCLEAN THESE STREETS?\nI AM THE ECHO\nOF YOUR FAILURE!";
const char boss_1[] PROGMEM = "THE PARK IS MINE.\nYOUR VIGILANTE ACT\nENDS HERE!";
const char boss_2[] PROGMEM = "THIS FACTORY GRINDS\nHEROES INTO DUST.\nPREPARE TO BE\nRECYCLED!";
const char boss_3[] PROGMEM = "THE DESERT BURIES\nALL SECRETS. AND\nSOON, IT WILL\nBURY YOU!";
const char boss_4[] PROGMEM = "I AM THE CITY'S\nTRUE MASTER.\nKNEEL BEFORE\nTHE ECHO!";

const char* const boss_talks[] PROGMEM = { boss_0, boss_1, boss_2, boss_3, boss_4 };

const char clear_0[] PROGMEM = "THE CITY STREETS ARE\nQUIETER NOW, BUT THE\nREAL THREAT LURKS\nDEEPER IN THE PARK.";
const char clear_1[] PROGMEM = "NATURE PROVIDES NO\nREFUGE FOR EVIL. THE\nPATH TO THE FACTORY\nIS NOW OPEN.";
const char clear_2[] PROGMEM = "THE INDUSTRIAL ZONE\nIS COLD, BUT YOUR\nWILL IS ON FIRE.\nNEXT: THE WASTELAND.";
const char clear_3[] PROGMEM = "THE DESERT HEAT IS\nNOTHING COMPARED TO\nTHE HEAT OF BATTLE.\nTHE FINAL CITY AWAITS.";
const char clear_4[] PROGMEM = "THE FINAL BOSS IS\nNEAR. FINISH THIS\nAND SAVE THE CITY\nONCE AND FOR ALL!";

const char* const stage_clears[] PROGMEM = { clear_0, clear_1, clear_2, clear_3, clear_4 };

const CharacterData roster[] PROGMEM = {
    {"ZENITH",  {12, 6, 10, 10, 12, 12}, TO_FP(1.3), {0, 1, 0, 2, 0, 1}, AI_BALANCED, 60, 70},
    {"CINDER",  {11, 5, 8, 8, 10, 10},   TO_FP(2.1), {2, 3, 2, 1, 1, 2}, AI_RUSHDOWN, 40, 50},
    {"GOLIATH", {15, 8, 12, 12, 14, 14}, TO_FP(0.8), {1, 0, 1, 2, 2, 1}, AI_TANK,     90, 100},
    {"VOLT",    {12, 6, 14, 14, 12, 12}, TO_FP(1.1), {0, 4, 3, 0, 1, 0}, AI_ZONER,    50, 60},
    {"KAGE",    {12, 6, 9, 9, 11, 11},   TO_FP(1.5), {2, 1, 2, 1, 0, 2}, AI_RUSHDOWN, 55, 55},
    {"SIREN",   {11, 6, 10, 10, 12, 12}, TO_FP(1.4), {0, 2, 0, 1, 1, 0}, AI_BALANCED, 45, 65},
    {"DRIFT",   {12, 6, 10, 10, 12, 12}, TO_FP(1.2), {2, 4, 3, 2, 0, 1}, AI_BALANCED, 65, 75},
    {"TUSK",    {13, 7, 11, 11, 13, 13}, TO_FP(0.9), {1, 1, 2, 2, 2, 2}, AI_TANK,     80, 90},
    {"JADE",    {11, 6, 9, 9, 11, 11},   TO_FP(1.7), {0, 2, 1, 1, 1, 1}, AI_RUSHDOWN, 50, 60},
    {"ECHO",    {12, 6, 10, 10, 12, 12}, TO_FP(1.3), {2, 0, 0, 0, 0, 0}, AI_ZONER,    60, 70}
};

#define MAX_BONES 12
struct Bone { int8_t parent; uint8_t length; bool isHitbox; bool isHurtbox; };
struct Skeleton { int32_t x, y; int16_t vx, vy; bool facingLeft, isJumping; CombatState state; uint8_t stateTimer; int8_t health, special; Bone bones[MAX_BONES]; uint8_t currentAngles[MAX_BONES]; int32_t worldX[MAX_BONES], worldY[MAX_BONES]; int16_t walkSpeed; AIState aiState; uint8_t aiTimer; uint8_t charIdx; uint8_t breathingPhase; };

#endif
