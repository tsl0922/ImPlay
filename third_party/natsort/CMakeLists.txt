cmake_minimum_required(VERSION 3.13)
project(natsort)

add_library(natsort OBJECT src/strnatcmp.c)

target_include_directories(natsort PUBLIC include)