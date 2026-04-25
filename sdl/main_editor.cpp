#include "AnimationEditor.h"

int main(int argc, char* argv[]) {
    AnimationEditor editor;
    editor.setup();
    while (true) {
        editor.loop();
    }
    return 0;
}
