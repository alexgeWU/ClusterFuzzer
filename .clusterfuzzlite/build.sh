#!/bin/bash -eu

$CXX $CXXFLAGS -I include -c src/filesystem.cpp -o filesystem.o

ar rcs libmockfs.a lib.o filesystem.o

$CXX $CXXFLAGS -I include \
    fuzz/fuzz_display_file.cpp \
    -o $OUT/fuzz_display_file \
    libmockfs.a \
    $LIB_FUZZING_ENGINENGINE