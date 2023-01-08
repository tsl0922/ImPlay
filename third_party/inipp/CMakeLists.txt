cmake_minimum_required(VERSION 3.13)
project(inipp)

set(CMAKE_CXX_STANDARD 11)

add_library(inipp INTERFACE)

target_include_directories(inipp INTERFACE include)