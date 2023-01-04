cmake_minimum_required(VERSION 3.13)
project(ImPlay VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    add_library(mpv SHARED IMPORTED)
    set_target_properties(mpv PROPERTIES
        IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/mpv/lib/libmpv-2.dll
        IMPORTED_IMPLIB ${PROJECT_SOURCE_DIR}/third_party/mpv/lib/libmpv.dll.a
    )
    set(MPV_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/third_party/mpv/include)
    set(MPV_LIBRARY_DIRS ${PROJECT_SOURCE_DIR}/third_party/mpv/lib)
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
    source/views/view.cpp
    source/views/command_palette.cpp
    source/views/context_menu.cpp
    source/views/about.cpp
    source/command.cpp
    source/dispatch.cpp
    source/mpv.cpp
    source/player.cpp
    source/window.cpp
    source/main.cpp
)
set(INCLUDE_DIRS include ${MPV_INCLUDE_DIRS})
set(LINK_LIBS fmt json nfd imgui ${MPV_LIBRARIES})
set(APPLICATION_TYPE)

if(WIN32)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/win32/app.rc.in ${CMAKE_CURRENT_BINARY_DIR}/app.rc @ONLY)
    list(APPEND SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
elseif(APPLE)
    set(APPLICATION_TYPE MACOSX_BUNDLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_ICON_FILE "app.icns")
    set(MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/Info.plist.in")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    set(MACOSX_BUNDLE_INFO_STRING "tsl0922")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.tsl0922.ImPlay")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2022 tsl0922. All rights reserved." )
endif()

add_executable(ImPlay ${APPLICATION_TYPE} ${SOURCE_FILES})
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
elseif (APPLE)
    set_property(TARGET ImPlay PROPERTY MACOSX_BUNDLE_INFO_PLIST ${MACOSX_BUNDLE_INFO_PLIST})
    add_custom_command(TARGET ImPlay POST_BUILD
        COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath
            "@executable_path/../Frameworks/"
            $<TARGET_FILE:ImPlay>
    )
    install(FILES ${PROJECT_SOURCE_DIR}/resources/macos/app.icns
            DESTINATION ${CMAKE_BINARY_DIR}/ImPlay.app/Contents/Resources)
    install(TARGETS ImPlay BUNDLE DESTINATION .)
    install(FILES $<TARGET_FILE:ImPlay> DESTINATION ${CMAKE_BINARY_DIR}/ImPlay.app)
    set(APPS "\${CMAKE_INSTALL_PREFIX}/ImPlay.app")
    set(DIRS ${CMAKE_BINARY_DIR})
    install(CODE "include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\" IGNORE_ITEM Python)")
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_BUNDLE_NAME ImPlay)
    set(CPACK_BUNDLE_ICON ${PROJECT_SOURCE_DIR}/resources/macos/app.icns)
    set(CPACK_BUNDLE_PLIST ${CMAKE_BINARY_DIR}/ImHex.app/Contents/Info.plist)
    include(CPack)
else()
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
endif()