cmake_minimum_required(VERSION 3.22.1)
project("hanto0")

add_library(hanto0 SHARED hanto0_bridge.cpp)
target_compile_options(hanto0 PRIVATE -std=c++20 -O2)
target_link_libraries(hanto0 log)
