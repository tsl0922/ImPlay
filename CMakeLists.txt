cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GetGitVersion)

get_git_version(GIT_VERSION SEM_VER)

project(ImPlay VERSION "${SEM_VER}")
set(CMAKE_CXX_STANDARD 20)

include(CMakeDependentOption)

option(USE_OPENGL_ES3 "Compile with OpenGL ES 3.0 loader" OFF)
option(USE_PATCHED_GLFW "Use patched GLFW to support additional features" OFF)
option(CREATE_PACKAGE "Create binary packages with CPack" OFF)
cmake_dependent_option(USE_MPV_WIN_BUILD "Use Prebuilt static mpv dll on Windows" ON "WIN32" OFF)
cmake_dependent_option(USE_XDG_PORTAL "Use xdg-desktop-portal for file dialogs on Linux" OFF "UNIX;NOT APPLE" OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

if(USE_MPV_WIN_BUILD)
  include(GetMpvWinDev)
  get_mpv_win_dev(mpv_dev)
else()
  pkg_search_module(MPV REQUIRED mpv>=0.29.0)
endif()
if(USE_PATCHED_GLFW)
  add_subdirectory(third_party/glfw)
  set(GLFW_LIBRARIES glfw)
else()
  pkg_search_module(GLFW REQUIRED glfw3>=3.1)
endif()
if(USE_XDG_PORTAL)
  set(NFD_PORTAL ON CACHE BOOL "Use xdg-desktop-portal for file dialogs on Linux" FORCE)
endif()

set(LIBROMFS_PROJECT_NAME ${PROJECT_NAME})
set(LIBROMFS_RESOURCE_LOCATION "${CMAKE_SOURCE_DIR}/resources/romfs")
set(OPENGL_LIBRARIES "glad")

add_subdirectory(third_party/glad)
add_subdirectory(third_party/fmt)
add_subdirectory(third_party/natsort)
add_subdirectory(third_party/json)
add_subdirectory(third_party/inipp)
add_subdirectory(third_party/imgui)
add_subdirectory(third_party/nativefiledialog)
add_subdirectory(third_party/libromfs)

set(SOURCE_FILES
  source/helpers/imgui.cpp
  source/helpers/lang.cpp
  source/helpers/nfd.cpp
  source/helpers/utils.cpp
  source/views/view.cpp
  source/views/command_palette.cpp
  source/views/context_menu.cpp
  source/views/debug.cpp
  source/views/about.cpp
  source/views/quickview.cpp
  source/views/settings.cpp
  source/theme.cpp
  source/config.cpp
  source/mpv.cpp
  source/player.cpp
  source/window.cpp
  source/main.cpp
)
set(INCLUDE_DIRS include ${MPV_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
set(LINK_LIBS glad fmt natsort json inipp nfd imgui ${CMAKE_THREAD_LIBS_INIT} ${MPV_LIBRARIES} ${GLFW_LIBRARIES} ${LIBROMFS_LIBRARY})

if(WIN32)
  configure_file(${PROJECT_SOURCE_DIR}/resources/win32/app.rc.in ${PROJECT_BINARY_DIR}/app.rc @ONLY)
  list(APPEND SOURCE_FILES ${PROJECT_BINARY_DIR}/app.rc)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${MPV_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${GIT_VERSION}")
if(USE_OPENGL_ES3)
  target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_IMPL_OPENGL_ES3)
endif()
if(USE_PATCHED_GLFW)
  target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_PATCHED)
endif()
if(USE_MPV_WIN_BUILD)
  add_dependencies(${PROJECT_NAME} mpv_dev)
endif()

if(CREATE_PACKAGE)
  include(CreateCpackPackage)
  prepare_package()
  create_package()
else()
  include(GNUInstallDirs)
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  if(UNIX AND NOT APPLE)
    install(FILES ${PROJECT_SOURCE_DIR}/resources/linux/implay.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES ${PROJECT_SOURCE_DIR}/resources/icon.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps RENAME implay.png)
  endif()
endif()