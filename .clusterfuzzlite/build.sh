#!/bin/bash -eu

# Compile shared library from src/filesystem.cpp
$CXX $CXXFLAGS -I include -c src/filesystem.cpp -o filesystem.o

ar rcs libmockfs.a filesystem.o

# Build the fuzz target, linking against the shared library and fuzzing engine
$CXX $CXXFLAGS -I include \
    fuzz/fuzz_display_file.cpp \
    -o $OUT/fuzz_display_file \
    libmockfs.a \
    $LIB_FUZZING_ENGINE