#pragma once
#include <cstdint>
#include <vector>

namespace ccitt_g4_tiff {

uint32_t decode_ccitt_g4(
    const uint8_t* data,
    size_t size,
    uint32_t width,
    uint32_t height,
    // uint32_t rows_per_strip,
    std::vector<uint8_t>& out
);

}
