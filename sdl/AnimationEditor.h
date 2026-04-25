#ifndef ANIMATION_EDITOR_H
#define ANIMATION_EDITOR_H

#include "../Platform.h"
#include "../game_data.h"
#include "../Engine.h"
#include <vector>

class AnimationEditor {
public:
    void setup();
    void loop();

private:
    void save();
    void updateSkeleton();
    void drawUI();
    bool drawButton(Arduboy2 &display, int16_t x, int16_t y, int16_t w, int16_t h, const char* label, bool active = false);

    Arduboy2 gameDisplay;
    Arduboy2 guiDisplay;
    
    Camera camera = { TO_FP(0), TO_FP(0), 180 };
    Skeleton skeleton;
    
    std::vector<Pose> localPoses;
    uint8_t currentPoseIdx = 0;
    uint8_t currentBoneIdx = 0;
    
    bool isPlaying = false;
    bool previewSpecial = false;
    uint16_t playbackFrame = 0;
    uint8_t playbackSpeed = 8;
};

#endif
