#include "../SorGame.h"

int main(int argc, char* argv[]) {
    SorGame game;
    game.setup();
    while (true) {
        game.loop();
    }
    return 0;
}
