include(ExternalProject)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

function(get_mpv_win_dev name)
  set(_mpv_static_roots
    "$ENV{MPV_WINBUILD_ROOT}"
    "$ENV{MPV_STATIC_ROOT}"
    "${PROJECT_SOURCE_DIR}/../mpv-winbuild-cmake"
    "${PROJECT_SOURCE_DIR}/mpv-winbuild-cmake"
  )

  set(_mpv_static_lib "")
  set(_mpv_include_dir "")
  foreach(_root IN LISTS _mpv_static_roots)
    if(_root AND EXISTS "${_root}")
      foreach(_candidate
        "${_root}/build64/x86_64-w64-mingw32/libmpv.a"
        "${_root}/build64/x86_64-w64-mingw32/lib/libmpv.a"
        "${_root}/build64/x86_64-w64-mingw32/lib/libmpv.dll.a"
        "${_root}/install/lib/libmpv.a"
        "${_root}/install/lib/libmpv.dll.a"
        "${_root}/libmpv.a"
        "${_root}/libmpv.dll.a"
      )
        if(EXISTS "${_candidate}")
          set(_mpv_static_lib "${_candidate}")
          get_filename_component(_mpv_static_dir "${_candidate}" DIRECTORY)
          break()
        endif()
      endforeach()

      if(_mpv_static_lib)
        foreach(_include
          "${_root}/build64/x86_64-w64-mingw32/include"
          "${_root}/install/include"
          "${_root}/include"
        )
          if(EXISTS "${_include}/mpv/client.h")
            set(_mpv_include_dir "${_include}")
            break()
          endif()
        endforeach()
        break()
      endif()
    endif()
  endforeach()

  if(_mpv_static_lib)
    set(MPV_DEV_DIR ${_mpv_static_dir})
    set(MPV_INCLUDE_DIRS ${_mpv_include_dir} PARENT_SCOPE)
    set(MPV_LIBRARY_DIRS ${_mpv_static_dir} PARENT_SCOPE)
    set(MPV_LIBRARIES mpv PARENT_SCOPE)
    return()
  endif()

  ExternalProject_Add(${name}
    URL ${PROJECT_SOURCE_DIR}/mpv-dev-x86_64-20260503-git-948c86d24c.7z
    URL_HASH SHA256=2D88F4A6BD63A559814E6FD28A0D2E286EE381028A2D22EFA4452AFD4252000E
    DOWNLOAD_NO_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include <BINARY_DIR>/include
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv.dll.a <BINARY_DIR>/mpv.lib
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv-2.dll ${CMAKE_BINARY_DIR}
    )
  ExternalProject_Get_property(${name} BINARY_DIR)
  set(MPV_DEV_DIR ${BINARY_DIR})

  set(MPV_INCLUDE_DIRS ${MPV_DEV_DIR}/include PARENT_SCOPE)
  set(MPV_LIBRARY_DIRS ${MPV_DEV_DIR} PARENT_SCOPE)
  set(MPV_LIBRARIES mpv PARENT_SCOPE)
endfunction()