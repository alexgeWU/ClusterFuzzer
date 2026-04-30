#!/bin/bash -eu

# Compile shared library from src/filesystem.cpp
$CXX $CXXFLAGS -I include -c src/filesystem.cpp -o filesystem.o

ar rcs libmockfs.a filesystem.o

for target in fuzz_display_file fuzz_shell_input fuzz_password fuzz_directory fuzz_edit_file; do
    $CXX $CXXFLAGS -I include \
        fuzz/${target}.cpp \
        -o $OUT/${target} \
        libmockfs.a \
        $LIB_FUZZING_ENGINE
done