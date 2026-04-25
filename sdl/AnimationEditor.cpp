#include "AnimationEditor.h"
#include <fstream>
#include <iostream>

void AnimationEditor::setup() {
    gameDisplay.begin("StickFighter Preview", 128, 64, 4);
    guiDisplay.begin("StickFighter Controls", 200, 240, 2);
    
    for (uint8_t i = 0; i < 12; i++) {
        Pose p;
        memcpy_P(&p, &poses[i], sizeof(Pose));
        localPoses.push_back(p);
    }
    
    Engine::initSkeleton(skeleton, 0, TO_FP(0), false);
    camera.zoom = 150;
}

void AnimationEditor::save() {
    std::ofstream out("poses.h");
    if (!out.is_open()) return;
    out << "#ifndef POSES_H\n#define POSES_H\n\n#ifdef ARDUINO\n#include <Arduino.h>\n#else\n#include \"Platform.h\"\n#endif\n\nconst Pose poses[] PROGMEM = {\n";
    for (size_t i = 0; i < localPoses.size(); i++) {
        out << "    {{" << (int)localPoses[i].angles[0];
        for (int j = 1; j < 6; j++) out << ", " << (int)localPoses[i].angles[j];
        out << "}}";
        if (i < localPoses.size() - 1) out << ",";
        out << "   // " << i << ": " << animNames[i] << "\n";
    }
    out << "};\n\nconst char* const animNames[] = { ";
    for (size_t i = 0; i < 12; i++) { out << "\"" << animNames[i] << "\"" << (i < 11 ? ", " : ""); }
    out << " };\n\n#endif\n";
    out.close();
    std::cout << "Saved!" << std::endl;
}

void AnimationEditor::updateSkeleton() {
    uint8_t pIdx = currentPoseIdx;
    
    // Physics
    skeleton.vy += GRAVITY;
    skeleton.y += skeleton.vy;
    if (skeleton.y >= GROUND_Y) {
        skeleton.y = GROUND_Y;
        skeleton.vy = 0;
        skeleton.isJumping = false;
    }

    if (previewSpecial) {
        if (skeleton.stateTimer > 0) skeleton.stateTimer--;
        if (skeleton.state == CS_SPECIAL_STARTUP && skeleton.stateTimer == 0) {
            skeleton.state = CS_SPECIAL_ACTIVE;
            skeleton.stateTimer = 15;
        } else if (skeleton.state == CS_SPECIAL_ACTIVE && skeleton.stateTimer == 0) {
            skeleton.state = CS_IDLE;
            previewSpecial = false;
        }
        
        if (skeleton.state == CS_SPECIAL_ACTIVE) pIdx = (skeleton.charIdx == 0) ? 6 : 7;
        else if (skeleton.state == CS_SPECIAL_STARTUP) pIdx = (skeleton.charIdx == 0) ? 8 : 5;
    } else if (isPlaying) {
        playbackFrame++;
        if (currentPoseIdx >= 1 && currentPoseIdx <= 4) pIdx = 1 + (playbackFrame / playbackSpeed) % 4;
    }
    
    if (skeleton.isJumping && !previewSpecial) pIdx = 7;

    Engine::updateSkeleton(skeleton, localPoses[pIdx], gameDisplay.frameCount, pIdx);
}

bool AnimationEditor::drawButton(Arduboy2 &display, int16_t x, int16_t y, int16_t w, int16_t h, const char* label, bool active) {
    bool hovered = (display.mouseX >= x && display.mouseX < x + w && display.mouseY >= y && display.mouseY < y + h);
    bool clicked = hovered && display.mouseJustPressed;
    
    display.drawRect(x, y, w, h, WHITE);
    if (active) display.fillRect(x + 2, y + 2, w - 4, h - 4, WHITE);
    else if (hovered && display.mousePressed) display.drawRect(x + 1, y + 1, w - 2, h - 2, WHITE);
    
    display.setCursor(x + 4, y + (h - 7) / 2);
    display.print(label);
    return clicked;
}

void AnimationEditor::drawUI() {
    guiDisplay.clear();
    
    // Character Selection
    guiDisplay.setCursor(5, 5);
    guiDisplay.print("CHAR: ");
    CharacterData cd;
    memcpy_P(&cd, &roster[skeleton.charIdx], sizeof(CharacterData));
    guiDisplay.print(cd.name);
    
    if (drawButton(guiDisplay, 5, 15, 20, 15, "<")) {
        if (skeleton.charIdx > 0) {
            skeleton.charIdx--;
            Engine::initSkeleton(skeleton, skeleton.charIdx, 0, false);
        }
    }
    if (drawButton(guiDisplay, 30, 15, 20, 15, ">")) {
        if (skeleton.charIdx < 9) {
            skeleton.charIdx++;
            Engine::initSkeleton(skeleton, skeleton.charIdx, 0, false);
        }
    }

    // Poses
    guiDisplay.setCursor(5, 40);
    guiDisplay.print("POSES:");
    for (uint8_t i = 0; i < 12; i++) {
        if (drawButton(guiDisplay, 5, 50 + i * 12, 50, 10, animNames[i], currentPoseIdx == i && !previewSpecial)) {
            currentPoseIdx = i;
            previewSpecial = false;
        }
    }
    
    // Bones
    guiDisplay.setCursor(65, 40);
    guiDisplay.print("BONES:");
    const char* boneNames[] = {"TORSO", "HEAD", "R ARM", "L ARM", "R LEG", "L LEG"};
    for (uint8_t i = 0; i < 6; i++) {
        if (drawButton(guiDisplay, 65, 50 + i * 15, 45, 12, boneNames[i], currentBoneIdx == i)) currentBoneIdx = i;
    }
    
    // Angle
    guiDisplay.setCursor(120, 70);
    guiDisplay.print("ANGLE");
    guiDisplay.setCursor(120, 80);
    guiDisplay.print((int)localPoses[currentPoseIdx].angles[currentBoneIdx]);
    if (drawButton(guiDisplay, 120, 90, 20, 15, "-")) localPoses[currentPoseIdx].angles[currentBoneIdx] -= 1;
    if (drawButton(guiDisplay, 145, 90, 20, 15, "+")) localPoses[currentPoseIdx].angles[currentBoneIdx] += 1;
    
    // Zoom
    guiDisplay.setCursor(120, 115);
    guiDisplay.print("ZOOM");
    guiDisplay.setCursor(120, 125);
    guiDisplay.print((int)camera.zoom);
    if (drawButton(guiDisplay, 120, 135, 22, 15, "-")) { if (camera.zoom > 10) camera.zoom -= 10; }
    if (drawButton(guiDisplay, 145, 135, 22, 15, "+")) camera.zoom += 10;
    
    // Controls
    if (drawButton(guiDisplay, 120, 5, 50, 15, isPlaying ? "STOP" : "PLAY", isPlaying)) { isPlaying = !isPlaying; playbackFrame = 0; previewSpecial = false; }
    if (drawButton(guiDisplay, 120, 25, 50, 15, "SPECIAL", previewSpecial)) {
        previewSpecial = true;
        skeleton.state = CS_SPECIAL_STARTUP;
        skeleton.stateTimer = 15;
    }
    if (drawButton(guiDisplay, 120, 45, 50, 15, "JUMP")) {
        if (!skeleton.isJumping) {
            skeleton.vy = JUMP_IMPULSE;
            skeleton.isJumping = true;
        }
    }
    if (drawButton(guiDisplay, 120, 150, 50, 15, "SAVE")) save();
    
    guiDisplay.display();
}

void AnimationEditor::loop() {
    if (!gameDisplay.nextFrame()) return;
    gameDisplay.pollButtons();
    
    // Keyboard Bone Editing (Debounced via justPressed)
    if (gameDisplay.justPressed(LEFT_BUTTON)) {
        localPoses[currentPoseIdx].angles[currentBoneIdx] -= 1;
    }
    if (gameDisplay.justPressed(RIGHT_BUTTON)) {
        localPoses[currentPoseIdx].angles[currentBoneIdx] += 1;
    }
    
    // Zoom control (smooth)
    if (gameDisplay.pressed(UP_BUTTON)) {
        camera.zoom += 2;
    }
    if (gameDisplay.pressed(DOWN_BUTTON)) {
        if (camera.zoom > 10) camera.zoom -= 2;
    }

    updateSkeleton();
    
    gameDisplay.clear();
    
    // Position camera to keep character roughly centered and ground visible
    camera.x = skeleton.x; 
    // Keep GROUND_Y at screen Y = 58 (near bottom)
    // 58 = (((GROUND_Y - camera.y) * zoom) / 100) + 32
    // 26 = (GROUND_Y - camera.y) * zoom / 100
    // GROUND_Y - camera.y = 2600 / zoom
    camera.y = GROUND_Y - TO_FP(2600 / camera.zoom);

    Engine::drawSkeleton(gameDisplay, skeleton, camera, 0);
    
    // Ground line
    Engine::drawScaledLine(gameDisplay, TO_FP(-200), GROUND_Y, TO_FP(200), GROUND_Y, camera, 0);
    
    gameDisplay.display();
    
    drawUI();
}
