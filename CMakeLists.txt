cmake_minimum_required(VERSION 3.13)
project(ImPlay VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

option(USE_MPV_WIN_BUILD "Use Prebuilt static mpv dll for Windows" ON)

cmake_policy(SET CMP0087 NEW)
cmake_policy(SET CMP0135 NEW)
include(ExternalProject)

if(WIN32 AND USE_MPV_WIN_BUILD)
    ExternalProject_Add(mpv_dev
        URL https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20230101/mpv-dev-x86_64-20230101-git-ad65c88.7z
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include <BINARY_DIR>/include/mpv
              COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv.dll.a <BINARY_DIR>
              COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/mpv-2.dll <BINARY_DIR>/libmpv-2.dll
        INSTALL_COMMAND ""
    )
    ExternalProject_Get_property(mpv_dev BINARY_DIR)
    set(MPV_DEV_DIR ${BINARY_DIR})

    set(MPV_INCLUDE_DIRS ${MPV_DEV_DIR}/include)
    set(MPV_LIBRARY_DIRS ${MPV_DEV_DIR})
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
    source/views/debug.cpp
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

if(WIN32)
    configure_file(${PROJECT_SOURCE_DIR}/resources/win32/app.rc.in ${PROJECT_BINARY_DIR}/app.rc @ONLY)
    list(APPEND SOURCE_FILES ${PROJECT_BINARY_DIR}/app.rc)
elseif(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_ICON_FILE "AppIcon")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.tsl0922.ImPlay")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2022 tsl0922. All rights reserved." )
endif()

add_executable(ImPlay ${SOURCE_FILES})
target_include_directories(ImPlay PRIVATE ${INCLUDE_DIRS})
target_link_directories(ImPlay PRIVATE ${MPV_LIBRARY_DIRS})
target_link_libraries(ImPlay PRIVATE ${LINK_LIBS})
if(WIN32)
    ExternalProject_Add(mpv_bin
        URL https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20230101/mpv-x86_64-20230101-git-ad65c88.7z
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/mpv.com <BINARY_DIR>
        INSTALL_COMMAND ""
    )
    add_dependencies(ImPlay mpv_bin)
    ExternalProject_Get_property(mpv_bin BINARY_DIR)
    set(MPV_BIN_DIR ${BINARY_DIR})

    target_link_options(ImPlay PRIVATE -mwindows)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
    install(FILES ${MPV_BIN_DIR}/mpv.com DESTINATION . RENAME ImPlay.com)
    if(USE_MPV_WIN_BUILD)
        add_dependencies(ImPlay mpv_dev)
        add_custom_command(TARGET ImPlay POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MPV_DEV_DIR}/libmpv-2.dll
                $<TARGET_FILE_DIR:ImPlay>
        )
    endif()
    
    install(CODE [[
    file(GET_RUNTIME_DEPENDENCIES
        EXECUTABLES $<TARGET_FILE:ImPlay>
        RESOLVED_DEPENDENCIES_VAR _r_deps
        UNRESOLVED_DEPENDENCIES_VAR _u_deps
        DIRECTORIES $ENV{PATH}
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )

    if(_u_deps)
        message(WARNING "Unresolved dependencies detected: \"${_u_deps}\"!")
    endif()

    foreach(_file ${_r_deps})
        file(INSTALL
            DESTINATION ${CMAKE_INSTALL_PREFIX}
            TYPE SHARED_LIBRARY
            FOLLOW_SYMLINK_CHAIN
            FILES "${_file}"
        )
    endforeach()
    ]])

    execute_process(COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/LICENSE
        ${CMAKE_BINARY_DIR}/LICENSE.txt
    )

    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/resources/win32/app.ico")
    set(CPACK_WIX_UPGRADE_GUID "D7438EFE-D62A-4E94-A024-6E71AE1A7A63")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "ImPlay")
    set_property(INSTALL "$<TARGET_FILE_NAME:ImPlay>" PROPERTY CPACK_START_MENU_SHORTCUTS "ImPlay")
    set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_BINARY_DIR}/LICENSE.txt)
elseif (APPLE)
    set_target_properties(ImPlay PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/resources/macos/Info.plist.in
    )
    add_custom_command(TARGET ImPlay POST_BUILD
        COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath
            "@executable_path/../Frameworks/"
            $<TARGET_FILE:ImPlay>
    )
    
    target_sources(ImPlay PRIVATE ${PROJECT_SOURCE_DIR}/resources/macos/AppIcon.icns)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/resources/macos/AppIcon.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )

    install(TARGETS ImPlay BUNDLE DESTINATION .)
    
    set(APPS "\${CMAKE_INSTALL_PREFIX}/ImPlay.app")
    set(DIRS ${CMAKE_BINARY_DIR})
    install(CODE "include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\" IGNORE_ITEM Python)"
    )
    
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_BUNDLE_NAME ImPlay)
    set(CPACK_BUNDLE_ICON ${PROJECT_SOURCE_DIR}/resources/macos/app.icns)
    set(CPACK_BUNDLE_PLIST ${CMAKE_BINARY_DIR}/ImHex.app/Contents/Info.plist)
else()
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_PACKAGE_NAME "ImPlay")
set(CPACK_PACKAGE_VENDOR "tsl0922")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A Cross-Platform Desktop Media Player")

include(CPack)