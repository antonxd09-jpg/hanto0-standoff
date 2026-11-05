cmake_minimum_required(VERSION 3.22.1)
project("hanto0")

add_library(hanto0 SHARED hanto0_internal.cpp)
target_compile_options(hanto0 PRIVATE -std=c++20 -O3 -fvisibility=hidden)
target_link_libraries(hanto0 log dl)

add_executable(libinjector.so injector.cpp)
target_compile_options(libinjector.so PRIVATE -std=c++20 -O3 -fPIE)
target_link_options(libinjector.so PRIVATE -pie)
target_link_libraries(libinjector.so dl)
