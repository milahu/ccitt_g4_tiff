cmake_minimum_required(VERSION 3.15)
project(ccitt_g4_tiff CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_library(ccitt_g4_tiff
    # src/third_party/pdfium/core/fxcrt/fx_memory_pa.cpp
    src/third_party/pdfium/core/fxcrt/fx_memory_malloc.cpp

    src/third_party/pdfium/core/fxcrt/fx_memory.h
    src/third_party/pdfium/core/fxcrt/fx_memory.cpp

    src/third_party/pdfium/core/fxcrt/debug/alias.h
    src/third_party/pdfium/core/fxcrt/debug/alias.cc

    src/third_party/pdfium/core/fxge/calculate_pitch.h
    src/third_party/pdfium/core/fxge/calculate_pitch.cpp

    src/third_party/pdfium/core/fxcodec/scanlinedecoder.h
    src/third_party/pdfium/core/fxcodec/scanlinedecoder.cpp

    src/third_party/pdfium/core/fxcodec/fax/faxmodule.cpp

    src/ccitt_g4.cpp
)

target_include_directories(ccitt_g4_tiff PUBLIC
    include
    # src/build/build_config.h for pdfium
    src
    src/third_party/pdfium
)

target_compile_options(ccitt_g4_tiff PRIVATE
    # -std=c++20

    # clang
    # -stdlib=libc++

    # -D_LIBCPP_ENABLE_CXX20_REMOVED_FEATURES
)



add_executable(tiff_cli tools/tiff_cli.cpp)

target_link_libraries(tiff_cli PRIVATE ccitt_g4_tiff)
