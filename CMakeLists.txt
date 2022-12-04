cmake_minimum_required(VERSION 3.13)
project(ImPlay VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    add_library(mpv SHARED IMPORTED)
    set_target_properties(mpv PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mpv/lib/libmpv-2.dll
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mpv/lib/libmpv.dll.a
    )
    set(MPV_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mpv/include)
    set(MPV_LIBRARY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mpv/lib)
    set(MPV_LIBRARIES mpv)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(MPV REQUIRED mpv)
endif()

add_subdirectory(third_party/fmt)
add_subdirectory(third_party/glfw)
add_subdirectory(third_party/json)
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/nativefiledialog)

set(SOURCE_FILES
    source/views/command_palette.cpp
    source/views/context_menu.cpp
    source/views/about.cpp
    source/dispatch.cpp
    source/mpv.cpp
    source/player.cpp
    source/window.cpp
    source/main.cpp
)
set(INCLUDE_DIRS include ${MPV_INCLUDE_DIRS})
set(LINK_LIBS fmt json nfd imgui ${MPV_LIBRARIES})

if(WIN32)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/win32/app.rc.in ${CMAKE_CURRENT_BINARY_DIR}/app.rc @ONLY)
    list(APPEND SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
endif()

add_executable(ImPlay ${SOURCE_FILES})
target_include_directories(ImPlay PRIVATE ${INCLUDE_DIRS})
target_link_directories(ImPlay PRIVATE ${MPV_LIBRARY_DIRS})
target_link_libraries(ImPlay PRIVATE ${LINK_LIBS})
if(WIN32)
    target_link_options(ImPlay PRIVATE -mwindows)
    add_custom_command(TARGET ImPlay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:mpv>
            $<TARGET_FILE_DIR:ImPlay>
    )
endif()