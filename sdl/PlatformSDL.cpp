#ifndef ARDUINO
#include "../Platform.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <map>

static uint8_t currentButtons = 0;
static uint8_t previousButtons = 0;
static int frameRate = 60;
static auto lastFrameTime = std::chrono::steady_clock::now();
static std::map<uint32_t, Arduboy2*> windowMap;

uint16_t Arduboy2::frameCount = 0;
bool Arduboy2::shouldRestart = false;
int16_t Arduboy2::mouseX = 0;
int16_t Arduboy2::mouseY = 0;
bool Arduboy2::mousePressed = false;
bool Arduboy2::mouseJustPressed = false;

Arduboy2::Arduboy2() {}

Arduboy2::~Arduboy2() {
    if (window) {
        uint32_t id = SDL_GetWindowID((SDL_Window*)window);
        if (windowMap[id] == this) {
            windowMap.erase(id);
        }
        SDL_DestroyTexture((SDL_Texture*)screenTexture);
        SDL_DestroyRenderer((SDL_Renderer*)renderer);
        SDL_DestroyWindow((SDL_Window*)window);
    }
    if (pixels) delete[] pixels;
}

void Arduboy2::begin() {
    begin("StickFighter SDL", 128, 64, 4);
}

void Arduboy2::begin(const char* title, int w, int h, int s) {
    width = w; height = h; scale = s;
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }
    }
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width * scale, height * scale, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer((SDL_Window*)window, -1, SDL_RENDERER_ACCELERATED);
    screenTexture = SDL_CreateTexture((SDL_Renderer*)renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    pixels = new uint32_t[width * height];
    windowMap[SDL_GetWindowID((SDL_Window*)window)] = this;
}

void Arduboy2::setFrameRate(int r) {
    frameRate = r;
}

bool Arduboy2::nextFrame() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count();
    if (elapsed < (1000 / frameRate)) {
        return false;
    }
    lastFrameTime = now;
    frameCount++;
    return true;
}

void Arduboy2::pollButtons() {
    SDL_Event e;
    previousButtons = currentButtons;
    mouseJustPressed = false;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            exit(0);
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                mousePressed = true;
                mouseJustPressed = true;
                if (windowMap.count(e.button.windowID)) {
                    mouseX = e.button.x / windowMap[e.button.windowID]->scale;
                    mouseY = e.button.y / windowMap[e.button.windowID]->scale;
                }
            }
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                mousePressed = false;
            }
        } else if (e.type == SDL_MOUSEMOTION) {
            if (windowMap.count(e.motion.windowID)) {
                mouseX = e.motion.x / windowMap[e.motion.windowID]->scale;
                mouseY = e.motion.y / windowMap[e.motion.windowID]->scale;
            }
        }
    }

    const uint8_t* state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_Q]) exit(0);
    currentButtons = 0;
    if (state[SDL_SCANCODE_LEFT]) currentButtons |= LEFT_BUTTON;
    if (state[SDL_SCANCODE_RIGHT]) currentButtons |= RIGHT_BUTTON;
    if (state[SDL_SCANCODE_UP]) currentButtons |= UP_BUTTON;
    if (state[SDL_SCANCODE_DOWN]) currentButtons |= DOWN_BUTTON;
    if (state[SDL_SCANCODE_Z]) currentButtons |= A_BUTTON;
    if (state[SDL_SCANCODE_X]) currentButtons |= B_BUTTON;
    if (state[SDL_SCANCODE_R]) { currentButtons |= R_BUTTON; shouldRestart = true; }
    if (state[SDL_SCANCODE_S]) currentButtons |= S_BUTTON;
}

void Arduboy2::clear() {
    memset(pixels, 0, width * height * sizeof(uint32_t));
}

void Arduboy2::display() {
    SDL_UpdateTexture((SDL_Texture*)screenTexture, NULL, pixels, width * sizeof(uint32_t));
    SDL_RenderClear((SDL_Renderer*)renderer);
    SDL_RenderCopy((SDL_Renderer*)renderer, (SDL_Texture*)screenTexture, NULL, NULL);
    SDL_RenderPresent((SDL_Renderer*)renderer);
}

void Arduboy2::drawPixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    pixels[y * width + x] = (color == WHITE) ? 0xFFFFFFFF : 0xFF000000;
}

void Arduboy2::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color) {
    for (int16_t j = 0; j < h; j += 8) {
        for (int16_t i = 0; i < w; i++) {
            uint8_t byte = pgm_read_byte(bitmap + (j / 8) * w + i);
            for (int16_t k = 0; k < 8; k++) {
                if (byte & (1 << k)) {
                    drawPixel(x + i, y + j + k, color);
                }
            }
        }
    }
}

void Arduboy2::drawBitmapMirror(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color) {
    for (int16_t j = 0; j < h; j += 8) {
        for (int16_t i = 0; i < w; i++) {
            uint8_t byte = pgm_read_byte(bitmap + (j / 8) * w + i);
            for (int16_t k = 0; k < 8; k++) {
                if (byte & (1 << k)) {
                    drawPixel(x + w - 1 - i, y + j + k, color);
                }
            }
        }
    }
}

void Arduboy2::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        drawPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void Arduboy2::drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c) {
    drawFastHLine(x, y, w, c);
    drawFastHLine(x, y + h - 1, w, c);
    drawFastVLine(x, y, h, c);
    drawFastVLine(x + w - 1, y, h, c);
}

void Arduboy2::fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c) {
    for (int i = 0; i < h; i++) drawFastHLine(x, y + i, w, c);
}

void Arduboy2::drawCircle(int16_t x0, int16_t y0, uint8_t r, uint8_t c) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;
    drawPixel(x0, y0 + r, c);
    drawPixel(x0, y0 - r, c);
    drawPixel(x0 + r, y0, c);
    drawPixel(x0 - r, y0, c);
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        drawPixel(x0 + x, y0 + y, c);
        drawPixel(x0 - x, y0 + y, c);
        drawPixel(x0 + x, y0 - y, c);
        drawPixel(x0 - x, y0 - y, c);
        drawPixel(x0 + y, y0 + x, c);
        drawPixel(x0 - y, y0 + x, c);
        drawPixel(x0 + y, y0 - x, c);
        drawPixel(x0 - y, y0 - x, c);
    }
}

void Arduboy2::fillCircle(int16_t x0, int16_t y0, uint8_t r, uint8_t c) {
    for (int8_t y = -r; y <= r; y++) {
        for (int8_t x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                drawPixel(x0 + x, y0 + y, c);
            }
        }
    }
}

void Arduboy2::drawFastHLine(int16_t x, int16_t y, uint8_t w, uint8_t c) {
    for (int i = 0; i < w; i++) drawPixel(x + i, y, c);
}

void Arduboy2::drawFastVLine(int16_t x, int16_t y, uint8_t h, uint8_t c) {
    for (int i = 0; i < h; i++) drawPixel(x, y + i, c);
}

static int cursorX = 0, cursorY = 0;

static const uint8_t font5x7[] = {
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x7C, 0x12, 0x11, 0x12, 0x7C, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x00, 0x00, 0x00, 0x00, // Space
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x33, 0x33, 0x00, 0x00, // :
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x22, 0x14, 0x08, 0x14, 0x22, // *
    0x00, 0x41, 0x41, 0x00, 0x00, // = (placeholder)
    0x04, 0x02, 0x7F, 0x02, 0x04, // >
    0x10, 0x20, 0x7F, 0x20, 0x10, // <
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x08, 0x08, 0x2A, 0x1C, 0x08, // {
    0x08, 0x1C, 0x2A, 0x08, 0x08  // }
};

void drawChar(Arduboy2* a, int x, int y, char c, uint8_t color) {
    const uint8_t* glyph = nullptr;
    if (c >= '0' && c <= '9') glyph = &font5x7[(c - '0') * 5];
    else if (c >= 'A' && c <= 'Z') glyph = &font5x7[(c - 'A' + 10) * 5];
    else if (c >= 'a' && c <= 'z') glyph = &font5x7[(c - 'a' + 10) * 5];
    else if (c == ' ') glyph = &font5x7[36 * 5];
    else if (c == '+') glyph = &font5x7[37 * 5];
    else if (c == '-') glyph = &font5x7[38 * 5];
    else if (c == ':') glyph = &font5x7[39 * 5];
    else if (c == '.') glyph = &font5x7[40 * 5];
    else if (c == '*') glyph = &font5x7[41 * 5];
    else if (c == '=') glyph = &font5x7[42 * 5];
    else if (c == '>') glyph = &font5x7[43 * 5];
    else if (c == '<') glyph = &font5x7[44 * 5];
    else if (c == '!') glyph = &font5x7[45 * 5];
    else if (c == '?') glyph = &font5x7[46 * 5];
    else if (c == '{') glyph = &font5x7[47 * 5];
    else if (c == '}') glyph = &font5x7[48 * 5];
    else glyph = &font5x7[36 * 5];

    for (int i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (int j = 0; j < 7; j++) {
            if (line & (1 << j)) {
                a->drawPixel(x + i, y + j, color);
            }
        }
    }
}

void Arduboy2::setCursor(int16_t x, int16_t y) {
    cursorX = x; cursorY = y;
}

void Arduboy2::print(const char* s) {
    while (*s) {
        if (*s == '\n') {
            cursorX = 0;
            cursorY += 8;
        } else {
            drawChar(this, cursorX, cursorY, *s, WHITE);
            cursorX += 6;
        }
        s++;
    }
}

void Arduboy2::print(int32_t n) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", n);
    print(buf);
}

void Arduboy2::print(uint32_t n) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%u", n);
    print(buf);
}

bool Arduboy2::pressed(uint8_t b) {
    return currentButtons & b;
}

bool Arduboy2::justPressed(uint8_t b) {
    return (currentButtons & b) && !(previousButtons & b);
}

uint8_t Arduboy2::buttonsState() {
    return currentButtons;
}

void Arduboy2::setExternalButtons(uint8_t b) {
    currentButtons = b;
}

long random(long max) {
    return rand() % max;
}

long random(long min, long max) {
    if (max == min) return min;
    return min + (rand() % (max - min));
}

#endif
