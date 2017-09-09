#!/bin/bash

CODE_PATH=code
BUILD_PATH=build
ASSETS_PATH=data

INSTALL_PATH="/usr/local/sf2bb/"

OPTIMIZED_SWITCHES="-fno-builtin -O2 -ffast-math -ftrapping-math"
DEBUG_SWITCHES=""

IGNORE_WARNING_FLAGS="-Wno-unused-function -Wno-unused-variable -Wno-missing-braces -Wno-c++11-compat-deprecated-writable-strings"
OSX_DEPENDENCIES="-framework Cocoa -framework IOKit -framework CoreAudio -framework AudioToolbox"

rm -rf $BUILD_PATH/*

clang -g $DEBUG_SWITCHES -Wall $IGNORE_WARNING_FLAGS -lstdc++ -DINTERNAL $CODE_PATH/sf2bb.cc -o $BUILD_PATH/sf2bb
# clang++ -S -mllvm --x86-asm-syntax=intel $CODE_PATH/sf2bb.cc -o $(BUILD_PATH)/sf2bb

rm -rf $INSTALL_PATH
mkdir -p $INSTALL_PATH

cp $BUILD_PATH/sf2bb $INSTALL_PATH

exit 0
