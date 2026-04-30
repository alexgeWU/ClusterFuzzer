#!/bin/bash -eu

# ClusterFuzzLite/libFuzzer build script.
# Expected project layout:
#   include/filesystem.h
#   src/filesystem.cpp
#   fuzz/*.cpp

$CXX $CXXFLAGS -std=c++17 -I include -c src/filesystem.cpp -o filesystem.o
ar rcs libmockfs.a filesystem.o

for target in \
  fuzz_directory_permissions \
  fuzz_file_permissions \
  fuzz_permission_ownership \
  fuzz_permission_shell_sequences \
  fuzz_shell_input \
  fuzz_display_file; do
    $CXX $CXXFLAGS -std=c++17 -I include \
        fuzz/${target}.cpp \
        -o $OUT/${target} \
        libmockfs.a \
        $LIB_FUZZING_ENGINE
done