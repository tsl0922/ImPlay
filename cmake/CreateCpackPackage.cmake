include(ExternalProject)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

function(get_mpv_win_bin name)
  ExternalProject_Add(${name}
    URL https://downloads.sourceforge.net/mpv-player-windows/mpv-x86_64-20250713-git-bd21180.7z
    URL_HASH SHA256=03dacaf114df07d33b9faafb9fd7d5486957abfa967515fdce19192d6e313c39
    DOWNLOAD_NO_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/mpv.com ${CMAKE_BINARY_DIR}/ImPlay.com
            COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/doc ${CMAKE_BINARY_DIR}/doc
  )
endfunction()

function(get_yt_dlp_bin name)
  ExternalProject_Add(${name}
    URL https://github.com/yt-dlp/yt-dlp/releases/download/2025.06.30/yt-dlp.exe
    URL_HASH SHA256=c070670e32cc7deaa5f3fcf51563b415ad9091323208f242fa6a3a3bab85bffd
    DOWNLOAD_NO_PROGRESS ON
    DOWNLOAD_NO_EXTRACT ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <DOWNLOAD_DIR>/yt-dlp.exe ${CMAKE_BINARY_DIR}
  )
endfunction()

function(get_electron_bin name)
  ExternalProject_Add(${name}
    URL https://github.com/electron/electron/releases/download/v22.3.27/electron-v22.3.27-win32-x64.zip
    URL_HASH SHA256=ad723ed7dad32f9459f7a9de1fd6d718cf713c4809c2431503bea62ce8f786e6
    DOWNLOAD_NO_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libEGL.dll ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libGLESv2.dll ${CMAKE_BINARY_DIR}
  )
endfunction()

macro(prepare_package)
  if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14.0")
    cmake_policy(SET CMP0087 NEW)
  endif()

  if(WIN32)
    get_mpv_win_bin(mpv_bin)
    get_yt_dlp_bin(yt_dlp)
    add_dependencies(${PROJECT_NAME} mpv_bin yt_dlp)

    target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/doc DESTINATION .)
    install(FILES ${CMAKE_BINARY_DIR}/ImPlay.com DESTINATION .)
    install(FILES ${CMAKE_BINARY_DIR}/yt-dlp.exe DESTINATION .)
    
    if(USE_OPENGL_ES3)
      get_electron_bin(electron_bin)
      add_dependencies(${PROJECT_NAME} electron_bin)
      install(FILES ${CMAKE_BINARY_DIR}/libEGL.dll DESTINATION .)
      install(FILES ${CMAKE_BINARY_DIR}/libGLESv2.dll DESTINATION .)
    endif()
    
    install(CODE [[file(GET_RUNTIME_DEPENDENCIES
      EXECUTABLES $<TARGET_FILE:ImPlay>
      RESOLVED_DEPENDENCIES_VAR _r_deps
      UNRESOLVED_DEPENDENCIES_VAR _u_deps
      DIRECTORIES $ENV{PATH}
      POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )

    if(_u_deps)
      message(WARNING "Unresolved dependencies detected: ${_u_deps}!")
    endif()

    foreach(_file ${_r_deps})
      file(INSTALL
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        TYPE SHARED_LIBRARY
        FOLLOW_SYMLINK_CHAIN
        FILES "${_file}"
      )
    endforeach()]])
  elseif (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
      BUILD_RPATH "@executable_path/../Frameworks"
      INSTALL_RPATH "@executable_path/../Frameworks"
      MACOSX_RPATH TRUE
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/resources/macos/Info.plist.in
    )
    
    target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/resources/macos/AppIcon.icns)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/resources/macos/AppIcon.icns
      PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )

    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)
    
    set(APP "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
    set(DIRS "/usr/local/lib" "/lib" "/usr/lib")
    file(GLOB_RECURSE LIBS "${APP}/Contents/MacOS/*.dylib")
    install(CODE "include(BundleUtilities)
    fixup_bundle(\"${APP}\" \"${LIBS}\" \"${DIRS}\" IGNORE_ITEM Python)
    execute_process(COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath
      \"@executable_path/../Frameworks/\"
      \"${APP}/Contents/MacOS/${PROJECT_NAME}\"
    )")
  else()
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${PROJECT_SOURCE_DIR}/resources/linux/implay.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES ${PROJECT_SOURCE_DIR}/resources/icon.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps RENAME implay.png)
  endif()
endmacro()

macro(create_package)
  if(WIN32)
    set(CPACK_GENERATOR ZIP WIX)
    set(CPACK_WIX_PATCH_FILE "${PROJECT_SOURCE_DIR}/resources/win32/wix/patch.xml")
    set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/resources/win32/app.ico")
    set(CPACK_WIX_UPGRADE_GUID "D7438EFE-D62A-4E94-A024-6E71AE1A7A63")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER ".")
    set_property(INSTALL "$<TARGET_FILE_NAME:ImPlay>" PROPERTY CPACK_START_MENU_SHORTCUTS "ImPlay")
  elseif (APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_ICON_FILE "AppIcon")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.tsl0922.ImPlay")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2022 tsl0922. All rights reserved." )

    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
    set(CPACK_BUNDLE_ICON ${PROJECT_SOURCE_DIR}/resources/macos/app.icns)
    set(CPACK_BUNDLE_PLIST ${CMAKE_BINARY_DIR}/ImHex.app/Contents/Info.plist)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_GENERATOR TGZ DEB)
    set(CPACK_DEBIAN_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "tsl0922")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS TRUE)
  else()
    set(CPACK_GENERATOR TGZ)
  endif()

  set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
  set(CPACK_PACKAGE_VENDOR "tsl0922")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A Cross-Platform Desktop Media Player")
  set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt")
  set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

  include(CPack)
endmacro()