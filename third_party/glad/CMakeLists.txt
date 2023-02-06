cmake_minimum_required(VERSION 3.13)
project(glad)

if(USE_OPENGL_ES3)
  add_library(glad OBJECT src/gles3.c)
else()
  add_library(glad OBJECT src/gl.c)
endif()

target_include_directories(glad PUBLIC include)
