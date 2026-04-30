#!/bin/bash -eu

# ClusterFuzzLite/libFuzzer build script.
# Expected project layout:
#   include/filesystem.h
#   src/filesystem.cpp
#   fuzz/*.cpp

$CXX $CXXFLAGS -std=c++17 -I include -c src/filesystem.cpp -o filesystem.o
ar rcs libmockfs.a filesystem.o

for target in \
  fuzz_extension_policy \
  fuzz_csv_render \
  fuzz_password_auth \
  fuzz_shell_input \
  fuzz_directory_paths \
  fuzz_edit_lifecycle; do
    $CXX $CXXFLAGS -std=c++17 -I include \
        fuzz/${target}.cpp \
        -o $OUT/${target} \
        libmockfs.a \
        $LIB_FUZZING_ENGINE
done
