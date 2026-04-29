#!/bin/bash -eu

# Build the main project as a static library so the fuzz target can link against it
$CXX $CXXFLAGS -c main.cpp -o main.o

ar rcs libmain.a main.o

# Build the fuzz target and link it against the main library and the fuzzing engine
$CXX $CXXFLAGS fuzz_target.cpp \
    -o $OUT/fuzz_target \
    libmain.a \
    $LIB_FUZZING_ENGINE