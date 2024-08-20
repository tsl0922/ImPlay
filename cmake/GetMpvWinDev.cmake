include(ExternalProject)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

function(get_mpv_win_dev name)
  ExternalProject_Add(${name}
    URL https://downloads.sourceforge.net/mpv-player-windows/mpv-dev-x86_64-20240818-git-a3baf94.7z
    URL_HASH SHA256=1a7eb75247e06f4acf8e81a0d0bb1e8cacf8fc1a00de402d3cb4f22d19818ce8
    DOWNLOAD_NO_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include <BINARY_DIR>/include
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv.dll.a <BINARY_DIR>
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv-2.dll ${CMAKE_BINARY_DIR}
    )
  ExternalProject_Get_property(${name} BINARY_DIR)
  set(MPV_DEV_DIR ${BINARY_DIR})

  set(MPV_INCLUDE_DIRS ${MPV_DEV_DIR}/include PARENT_SCOPE)
  set(MPV_LIBRARY_DIRS ${MPV_DEV_DIR} PARENT_SCOPE)
  set(MPV_LIBRARIES mpv PARENT_SCOPE)
endfunction()