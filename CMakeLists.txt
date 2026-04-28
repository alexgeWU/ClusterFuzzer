cmake_minimum_required(VERSION 3.30)
project(ClusterFuzzer)

set(CMAKE_CXX_STANDARD 17)

# Magically fetch FTXUI from GitHub
include(FetchContent)
FetchContent_Declare(
        ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
        GIT_TAG main # You can lock this to a version like v5.0.0 later
)
FetchContent_MakeAvailable(ftxui)

# Create your executable
add_executable(showcase main.cpp)

# Link the three main FTXUI modules to your app
target_link_libraries(showcase PRIVATE
        ftxui::screen
        ftxui::dom
        ftxui::component
)