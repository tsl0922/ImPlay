include(ExternalProject)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

function(get_mpv_win_dev name)
  ExternalProject_Add(${name}
    URL https://downloads.sourceforge.net/mpv-player-windows/mpv-dev-x86_64-20250713-git-bd21180.7z
    URL_HASH SHA256=e75982d90a1fa3620b194204420386f590b4e96f03dc9daf46b996fccce3549f
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