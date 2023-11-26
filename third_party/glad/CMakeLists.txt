cmake_minimum_required(VERSION 3.13)
project(glad)

add_library(glad OBJECT
  $<IF:$<BOOL:${USE_OPENGL_ES3}>,src/gles3.c,src/gl.c>
)
target_include_directories(glad PUBLIC include)
