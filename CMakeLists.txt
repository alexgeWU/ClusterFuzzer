cmake_minimum_required(VERSION 3.30)
project(ClusterFuzzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mylib src/lib.cpp)
target_include_directories(mylib PUBLIC include)

add_executable(my_program src/main.cpp)
target_include_directories(my_program PRIVATE include)
target_link_libraries(my_program PRIVATE mylib)

if(DEFINED ENV{LIB_FUZZING_ENGINE})
    add_executable(fuzz_target fuzz/fuzz_target.cpp)
    target_include_directories(fuzz_target PRIVATE include)
    target_link_libraries(fuzz_target PRIVATE mylib $ENV{LIB_FUZZING_ENGINE})
    target_compile_options(fuzz_target PRIVATE -fsanitize=fuzzer,address)
    target_link_options(fuzz_target PRIVATE -fsanitize=fuzzer,address)
endif()