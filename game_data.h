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
enum GameState { STATE_TITLE, STATE_CHAR_SELECT, STATE_STAGE_INTRO, STATE_PLAYING, STATE_STAGE_CLEAR, STATE_ENDING, STATE_RESULTS };

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

struct CharacterData { char name[8]; uint8_t lengths[6]; int16_t walkSpeed; FaceData face; AIProfile profile; };

const char intro_0[] PROGMEM = "ZENITH HAS TRAINED\nFOR A LIFETIME. HIS\nWILL IS UNBREAKABLE.\nHE CLAIMS DESTINY.";
const char intro_1[] PROGMEM = "CINDER RISES FROM\nTHE ASHES. A HEART\nOF FIRE DRIVES HIM\nTO BURN ALL FOES.";
const char intro_2[] PROGMEM = "GOLIATH IS A FORCE\nOF NATURE. MOUNTAINS\nBOW TO HIS MIGHT.\nTHE GROUND SHAKES.";
const char intro_3[] PROGMEM = "VOLT STRIKES LIKE\nLIGHTNING. DEADLY,\nAND FULL OF ENERGY.\nNONE CAN OUTRUN HIM.";
const char intro_4[] PROGMEM = "KAGE MOVES IN THE\nSHADOWS. A MASTER\nOF STEALTH. THE\nTITLE IS HIS.";
const char intro_5[] PROGMEM = "SIREN'S GRACE HIDES\nA LETHAL TRAP. SHE\nFIGHTS ELEGANTLY\nAND DEADLY.";
const char intro_6[] PROGMEM = "DRIFT WANDERS WITH\nTHE WIND. CALM UNTIL\nTHE STORM BREAKS.\nHE IS UNSTOPPABLE.";
const char intro_7[] PROGMEM = "TUSK CARRIES THE\nWEIGHT OF KINGS. HIS\nSTRENGTH IS PRIMAL\nAND SPIRIT ETERNAL.";
const char intro_8[] PROGMEM = "JADE HAS FOUND PEACE.\nNOW HE SEEKS OUTER\nDOMINANCE. HIS FOCUS\nIS PERFECT.";
const char intro_9[] PROGMEM = "ECHO IS A MYSTERY.\nA SOUL FORGED IN\nSILENCE. HE SPEAKS\nONLY VIA VICTORY.";

const char* const intros[] PROGMEM = { intro_0, intro_1, intro_2, intro_3, intro_4, intro_5, intro_6, intro_7, intro_8, intro_9 };

const char end_0[] PROGMEM = "ZENITH HAS CONQUERED\nTHE LADDER. THE WORLD\nKNOWS HIS NAME. HE IS\nSUPREME CHAMPION!";
const char end_1[] PROGMEM = "CINDER'S FIRE HAS\nCONSUMED ALL FOES.\nHE STANDS ATOP THE\nASHES, THE KING.";
const char end_2[] PROGMEM = "GOLIATH HAS CRUSHED\nEVERY CHALLENGER.\nHE IS THE NEW PEAK\nOF HUMAN STRENGTH.";
const char end_3[] PROGMEM = "VOLT HAS CHARGED\nINTO HISTORY. HIS\nSPEED IS UNMATCHED.\nA LEGEND IS BORN.";
const char end_4[] PROGMEM = "KAGE DISAPPEARS INTO\nTHE NIGHT, HIS TASK\nCOMPLETE. THE LADDER\nWAS THE BEGINNING.";
const char end_5[] PROGMEM = "SIREN HAS PROVEN THAT\nBEAUTY IS DEADLY.\nSHE REIGNS SUPREME\nOVER THE FALLEN.";
const char end_6[] PROGMEM = "DRIFT CONTINUES HIS\nJOURNEY AS A MASTER.\nTHE WIND WHISPERS\nHIS TRIUMPH.";
const char end_7[] PROGMEM = "TUSK HAS RESTORED\nHIS PEOPLE'S HONOR.\nHIS NAME WILL BE\nSUNG FOREVER.";
const char end_8[] PROGMEM = "JADE HAS ACHIEVED\nPERFECTION. HIS\nDOMINANCE IS ABSOLUTE.\nA NEW ERA BEGINS.";
const char end_9[] PROGMEM = "ECHO'S SILENCE IS\nBROKEN BY APPLAUSE.\nHE IS THE UNKNOWN\nKING OF FIGHTERS.";

const char* const endings[] PROGMEM = { end_0, end_1, end_2, end_3, end_4, end_5, end_6, end_7, end_8, end_9 };

const CharacterData roster[] PROGMEM = {
    {"ZENITH",  {12, 6, 10, 10, 12, 12}, TO_FP(1.3), {0, 1, 0, 2, 0, 1}, AI_BALANCED},
    {"CINDER",  {11, 5, 8, 8, 10, 10},   TO_FP(2.1), {2, 3, 2, 1, 1, 2}, AI_RUSHDOWN},
    {"GOLIATH", {15, 8, 12, 12, 14, 14}, TO_FP(0.8), {1, 0, 1, 2, 2, 1}, AI_TANK},
    {"VOLT",    {12, 6, 14, 14, 12, 12}, TO_FP(1.1), {0, 4, 3, 0, 1, 0}, AI_ZONER},
    {"KAGE",    {12, 6, 9, 9, 11, 11},   TO_FP(1.5), {2, 1, 2, 1, 0, 2}, AI_RUSHDOWN},
    {"SIREN",   {11, 6, 10, 10, 12, 12}, TO_FP(1.4), {0, 2, 0, 1, 1, 0}, AI_BALANCED},
    {"DRIFT",   {12, 6, 10, 10, 12, 12}, TO_FP(1.2), {2, 4, 3, 2, 0, 1}, AI_BALANCED},
    {"TUSK",    {13, 7, 11, 11, 13, 13}, TO_FP(0.9), {1, 1, 2, 2, 2, 2}, AI_TANK},
    {"JADE",    {11, 6, 9, 9, 11, 11},   TO_FP(1.7), {0, 2, 1, 1, 1, 1}, AI_RUSHDOWN},
    {"ECHO",    {12, 6, 10, 10, 12, 12}, TO_FP(1.3), {2, 0, 0, 0, 0, 0}, AI_ZONER}
};

#define MAX_BONES 12
struct Bone { int8_t parent; uint8_t length; bool isHitbox; bool isHurtbox; };
struct Skeleton { int32_t x, y; int16_t vx, vy; bool facingLeft, isJumping; CombatState state; uint8_t stateTimer; int8_t health, special; Bone bones[MAX_BONES]; uint8_t currentAngles[MAX_BONES]; int32_t worldX[MAX_BONES], worldY[MAX_BONES]; int16_t walkSpeed; AIState aiState; uint8_t aiTimer; uint8_t charIdx; uint8_t breathingPhase; };

#endif
