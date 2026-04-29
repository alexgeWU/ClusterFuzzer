cmake_minimum_required(VERSION 3.30)
project(ClusterFuzzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(program_build main.cpp)

if(DEFINED ENV{LIB_FUZZING_ENGINE})
    add_executable(fuzz_target fuzz_target.cpp)
    target_compile_options(fuzz_target PRIVATE -fsanitize=fuzzer,address)
    target_link_libraries(fuzz_target PRIVATE $ENV{LIB_FUZZING_ENGINE})
    target_link_options(fuzz_target PRIVATE -fsanitize=fuzzer,address)
endif()
