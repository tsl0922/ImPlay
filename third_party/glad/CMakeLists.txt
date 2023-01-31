cmake_minimum_required(VERSION 3.13)
project(glad)

add_library(glad OBJECT src/gles2.c)

target_include_directories(glad PUBLIC include)
