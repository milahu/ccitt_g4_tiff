cmake_minimum_required(VERSION 3.15)
project(tiff_binary CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_library(tiff_binary
    src/ccitt_g4.cpp
    src/third_party/pdfium/core/fxcodec/fax/faxmodule.cpp
)

target_include_directories(tiff_binary PUBLIC
    include
    src/third_party/pdfium
)

target_compile_options(tiff_binary PRIVATE
    # -std=c++20

    # clang
    # -stdlib=libc++

    # -D_LIBCPP_ENABLE_CXX20_REMOVED_FEATURES
)



add_executable(tiff_cli tools/tiff_cli.cpp)

target_link_libraries(tiff_cli PRIVATE tiff_binary)
