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



if(EMSCRIPTEN)

    add_executable(tiff_wasm
        tools/bmp_encoder.cpp
        tools/tiff_to_bmp.cpp
    )

    target_link_libraries(tiff_wasm PRIVATE
        ccitt_g4_tiff
    )

    # see also
    # https://github.com/paulocoutinhox/pdfium-lib/blob/master/modules/wasm.py

    target_link_options(tiff_wasm PRIVATE
        "-sWASM=1"
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=create_ccitt_g4_tiff_module"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sEXPORTED_FUNCTIONS=['_tiff_to_bmp','_tiff_free','_malloc','_free']"
        "-sEXPORTED_RUNTIME_METHODS=['cwrap','HEAPU8','HEAPU32']"
        "-sNO_DISABLE_EXCEPTION_CATCHING"

        # WONTFIX this will never work over the file protocol
        # Access to fetch at 'file:...' from origin 'null' has been blocked by CORS policy
        # "-sSINGLE_FILE=1"
        # "-sMINIMAL_RUNTIME=1"
    )

else()

    add_executable(tiff_cli
        tools/tiff_cli.cpp
        tools/bmp_encoder.cpp
        tools/tiff_to_bmp.cpp
    )

    target_link_libraries(tiff_cli PRIVATE
        ccitt_g4_tiff
    )

endif()
