cmake_minimum_required(VERSION 3.14)
project(ClusterFuzzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ──────────────────────────────────────────
# Shared library
# filesystem.cpp is the core logic shared by
# the main program and all fuzz targets
# ──────────────────────────────────────────
add_library(mockfs_lib src/filesystem.cpp)
target_include_directories(mockfs_lib PUBLIC include)

# ──────────────────────────────────────────
# Main program executable
# ──────────────────────────────────────────
add_executable(mockfs src/main.cpp)
target_include_directories(mockfs PRIVATE include)
target_link_libraries(mockfs PRIVATE mockfs_lib)

# ──────────────────────────────────────────
# Fuzz targets (only built inside ClusterFuzzLite environment)
# Add a new block below for each additional fuzz target
# ──────────────────────────────────────────
if(DEFINED ENV{LIB_FUZZING_ENGINE})

    # fuzz_display_file — fuzzes displayFile auth logic
    add_executable(fuzz_display_file fuzz/fuzz_display_file.cpp)
    target_include_directories(fuzz_display_file PRIVATE include)
    target_link_libraries(fuzz_display_file PRIVATE mockfs_lib $ENV{LIB_FUZZING_ENGINE})
    target_compile_options(fuzz_display_file PRIVATE -fsanitize=fuzzer,address)
    target_link_options(fuzz_display_file PRIVATE -fsanitize=fuzzer,address)

endif()