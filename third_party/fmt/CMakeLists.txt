cmake_minimum_required(VERSION 3.13)
project(fmt)

set(CMAKE_CXX_STANDARD 17)

add_library(fmt OBJECT src/format.cc src/os.cc)

target_include_directories(fmt PUBLIC include)
