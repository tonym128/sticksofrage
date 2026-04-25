# StickFighter Makefile for arduino-cli

SKETCH = sticksofrage.ino
BOARD = arduboy:avr:arduboy
# If the Arduboy board package is not installed, use Leonardo:
# BOARD = arduino:avr:leonardo
BUILD_DIR = build
OUTPUT_HEX = $(BUILD_DIR)/sticksofrage.ino.hex

.PHONY: all setup compile upload clean test sdl editor

all: compile

sdl:
	g++ -O2 SorGame.cpp Engine.cpp sdl/PlatformSDL.cpp sdl/main_sor.cpp -o sticksofrage_sdl `sdl2-config --cflags --libs`

editor:
	g++ -O2 sdl/AnimationEditor.cpp Engine.cpp sdl/PlatformSDL.cpp sdl/main_editor.cpp -o stickfighter_editor `sdl2-config --cflags --libs`

test:
	g++ -O2 -I. SorGame.cpp Engine.cpp sdl/PlatformSDL.cpp tests/unit_tests.cpp -o tests/runner `sdl2-config --cflags --libs`
	./tests/runner

setup:
	./bin/arduino-cli core update-index
	./bin/arduino-cli core install arduboy:avr
	./bin/arduino-cli lib install Arduboy2

compile:
	mkdir -p $(BUILD_DIR)
	./bin/arduino-cli compile --fqbn $(BOARD) --output-dir $(BUILD_DIR) $(SKETCH)

upload:
	./bin/arduino-cli upload -p /dev/ttyACM0 --fqbn $(BOARD) --input-dir $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
