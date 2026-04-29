#!/bin/bash -eu

# Compile shared library from src/lib.cpp
$CXX $CXXFLAGS -I include -c src/lib.cpp -o lib.o

ar rcs libmylib.a lib.o

# Build the fuzz target, linking against the shared library and fuzzing engine
$CXX $CXXFLAGS -I include \
    fuzz/fuzz_target.cpp \
    -o $OUT/fuzz_target \
    libmylib.a \
    $LIB_FUZZING_ENGINE