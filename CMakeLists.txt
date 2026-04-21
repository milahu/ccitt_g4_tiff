cmake_minimum_required(VERSION 3.15)
project(tiff_binary CXX)

add_library(tiff_binary
    src/ccitt_g4.cpp
)

target_include_directories(tiff_binary PUBLIC include)

add_executable(tiff_cli tools/tiff_cli.cpp)
target_link_libraries(tiff_cli PRIVATE tiff_binary)
