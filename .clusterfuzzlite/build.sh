#!/bin/bash -eu

# ClusterFuzzLite/libFuzzer build script for MockFS.
# Supports either:
#   include/filesystem.h + src/filesystem.cpp + fuzz/*.cpp
# or the flat uploaded layout:
#   filesystem.h + filesystem.cpp + fuzz/*.cpp

ROOT="${SRC:-$(pwd)}"

if [[ -f "$ROOT/src/filesystem.cpp" ]]; then
  SRC_FILE="$ROOT/src/filesystem.cpp"
  INC_DIR="$ROOT/include"
elif [[ -f "$ROOT/filesystem.cpp" ]]; then
  SRC_FILE="$ROOT/filesystem.cpp"
  INC_DIR="$ROOT"
else
  echo "Could not find filesystem.cpp in $ROOT/src or $ROOT" >&2
  exit 1
fi

FUZZ_DIR="$ROOT/fuzz"
: "${OUT:=$ROOT/out}"
mkdir -p "$OUT"

$CXX $CXXFLAGS -std=c++17 -I"$INC_DIR" -I"$FUZZ_DIR" -c "$SRC_FILE" -o "$OUT/filesystem.o"
ar rcs "$OUT/libmockfs.a" "$OUT/filesystem.o"

for fuzz_src in "$FUZZ_DIR"/fuzz_*.cpp; do
  target="$(basename "$fuzz_src" .cpp)"
  $CXX $CXXFLAGS -std=c++17 -I"$INC_DIR" -I"$FUZZ_DIR" \
      "$fuzz_src" \
      -o "$OUT/$target" \
      "$OUT/libmockfs.a" \
      $LIB_FUZZING_ENGINE
done
