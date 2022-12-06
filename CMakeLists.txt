cmake_minimum_required(VERSION 3.13)
project(ImPlay)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
pkg_search_module(MPV REQUIRED mpv)

add_subdirectory(third_party/imgui)
add_subdirectory(third_party/json)
add_subdirectory(third_party/nativefiledialog)

add_executable(ImPlay
    source/mpv.cpp
    source/player.cpp
    source/window.cpp
    source/main.cpp
)
target_include_directories(ImPlay PRIVATE include ${MPV_INCLUDE_DIRS})
target_link_directories(ImPlay PRIVATE ${MPV_LIBRARY_DIRS})
target_link_libraries(ImPlay PRIVATE imgui json nfd ${MPV_LIBRARIES})
if(MINGW)
    target_link_options(ImPlay PRIVATE -mwindows)
endif()