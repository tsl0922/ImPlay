cmake_minimum_required(VERSION 3.13)
project(imgui)

set(CMAKE_CXX_STANDARD 17)

find_package(Freetype REQUIRED)
find_package(OpenGL REQUIRED)

add_library(imgui OBJECT
        source/imgui.cpp
        source/imgui_demo.cpp
        source/imgui_draw.cpp
        source/imgui_freetype.cpp
        source/imgui_impl_glfw.cpp
        source/imgui_impl_opengl3.cpp
        source/imgui_tables.cpp
        source/imgui_widgets.cpp
        source/fonts/fontawesome.c
        source/fonts/unifont.c
)

target_include_directories(imgui PUBLIC include ${FREETYPE_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})
target_link_libraries(imgui PUBLIC ${FREETYPE_LIBRARIES} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
