#!/bin/bash

CODE_PATH=code
BUILD_PATH=build
ASSETS_PATH=data

OPTIMIZED_SWITCHES="-fno-builtin -O2 -ffast-math -ftrapping-math"
DEBUG_SWITCHES=""

IGNORE_WARNING_FLAGS="-Wno-unused-function -Wno-unused-variable -Wno-missing-braces -Wno-c++11-compat-deprecated-writable-strings"
OSX_DEPENDENCIES="-framework Cocoa -framework IOKit -framework CoreAudio -framework AudioToolbox"

clang -g $DEBUG_SWITCHES -Wall $IGNORE_WARNING_FLAGS -lstdc++ -DINTERNAL $CODE_PATH/midi2note.cc -o $BUILD_PATH/midi2note
$BUILD_PATH/midi2note

clang -g $DEBUG_SWITCHES -Wall $IGNORE_WARNING_FLAGS -lstdc++ -DINTERNAL $CODE_PATH/sf2bb.cc -o $BUILD_PATH/sf2bb
# clang++ -S -mllvm --x86-asm-syntax=intel $CODE_PATH/sf2bb.cc -o $(BUILD_PATH)/sf2bb

exit 0
