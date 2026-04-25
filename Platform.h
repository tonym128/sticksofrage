#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef ARDUINO
#include <Arduboy2.h>
#else

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

// Mock Arduino/Arduboy constants
#define PROGMEM
#define pgm_read_word(addr) (*(uint16_t*)(addr))
#define pgm_read_byte(addr) (*(uint8_t*)(addr))
#define pgm_read_ptr(addr) (*(void**)(addr))
#define strcpy_P(dest, src) strcpy(dest, src)
class __FlashStringHelper;
#define F(x) (reinterpret_cast<const __FlashStringHelper *>(x))

#define WHITE 1
#define BLACK 0

inline void memcpy_P(void* dest, const void* src, size_t n) { memcpy(dest, src, n); }

#define LEFT_BUTTON  (1 << 0)
#define RIGHT_BUTTON (1 << 1)
#define UP_BUTTON    (1 << 2)
#define DOWN_BUTTON  (1 << 3)
#define A_BUTTON     (1 << 4)
#define B_BUTTON     (1 << 5)
#define S_BUTTON     (1 << 6)
#define R_BUTTON     (1 << 7)

// We will implement this in PlatformSDL.cpp
class Arduboy2 {
public:
    Arduboy2();
    ~Arduboy2();
    static uint16_t frameCount;
    static bool shouldRestart;
    static int16_t mouseX, mouseY;
    static bool mousePressed, mouseJustPressed;
    
#ifdef ARDUINO
    void begin();
#else
    void* window = nullptr;
    void* renderer = nullptr;
    void* screenTexture = nullptr;
    uint32_t* pixels = nullptr;
    int width=128, height=64, scale=4;
    void begin(); // Default 128x64
    void begin(const char* title, int w, int h, int s);
#endif

    void setFrameRate(int r);
    bool nextFrame();
    void pollButtons();
    void clear();
    void display();
    
    void setCursor(int16_t x, int16_t y);
    void print(const char* s);
    void print(int32_t n);
    void print(uint32_t n);
    void print(const class __FlashStringHelper* s) { print(reinterpret_cast<const char*>(s)); }

    bool pressed(uint8_t b);
    bool justPressed(uint8_t b);
    uint8_t buttonsState();
    void setExternalButtons(uint8_t b); // For testing
    
    void drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c = WHITE);
    void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c = WHITE);
    void drawCircle(int16_t x, int16_t y, uint8_t r, uint8_t c = WHITE);
    void fillCircle(int16_t x, int16_t y, uint8_t r, uint8_t c = WHITE);
    void drawFastHLine(int16_t x, int16_t y, uint8_t w, uint8_t c = WHITE);
    void drawFastVLine(int16_t x, int16_t y, uint8_t h, uint8_t c = WHITE);
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c = WHITE);
    void drawPixel(int16_t x, int16_t y, uint8_t c = WHITE);
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color = WHITE);
    void drawBitmapMirror(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color = WHITE);
};

long random(long max);
long random(long min, long max);
inline int32_t labs(int32_t x) { return x < 0 ? -x : x; }

#endif

#endif
