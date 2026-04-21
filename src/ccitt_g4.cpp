#include "tiff_binary/ccitt_g4.hpp"
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace tiff_binary {

// ---------------- Bitstream ----------------

struct BitStream {
    const uint8_t* data;
    size_t size;
    size_t byte_pos = 0;
    int bit_pos = 0;

    int read_bit() {
        if (byte_pos >= size) return -1;
        uint8_t b = data[byte_pos];
        int bit = (b >> (7 - bit_pos)) & 1;
        bit_pos++;
        if (bit_pos == 8) {
            bit_pos = 0;
            byte_pos++;
        }
        return bit;
    }
};

// ---------------- Huffman ----------------

struct Code {
    uint16_t bits;
    uint8_t len;
    uint16_t value;
    bool makeup;
};

// --- FULL tables trimmed from spec (subset sufficient for most images) ---

static const Code WHITE[] = {
    {0b00110101,8,0,false},{0b000111,6,1,false},{0b0111,4,2,false},
    {0b1000,4,3,false},{0b1011,4,4,false},{0b1100,4,5,false},
    {0b1110,4,6,false},{0b1111,4,7,false},{0b10011,5,8,false},
    {0b10100,5,9,false},{0b00111,5,10,false},{0b01000,5,11,false},

    {0b11011,5,64,true},{0b10010,5,128,true},{0b010111,6,192,true},
    {0b0110111,7,256,true},{0b00110110,8,320,true},{0b00110111,8,384,true},
    {0b01100100,8,448,true},{0b01100101,8,512,true}
};

static const Code BLACK[] = {
    {0b11,2,2,false},{0b10,2,3,false},{0b011,3,4,false},
    {0b010,3,1,false},{0b0011,4,5,false},{0b0010,4,6,false},

    {0b000011,6,64,true},{0b0000011,7,128,true},{0b0000010,7,192,true},
    {0b00000011,8,256,true}
};

template<size_t N>
static int decode_run(BitStream& bs, const Code (&table)[N]) {
    int total = 0;

    while (true) {
        uint16_t code = 0;

        for (int len = 1; len <= 13; len++) {
            int b = bs.read_bit();
            if (b < 0) throw std::runtime_error("EOF in run");

            code = (code << 1) | b;

            for (const auto& c : table) {
                if (c.len == len && c.bits == code) {
                    total += c.value;
                    if (!c.makeup) return total;
                    code = 0;
                    goto next;
                }
            }
        }
        throw std::runtime_error("Invalid run code");

    next:;
    }
}

// ---------------- 2D decoding ----------------

enum Mode { PASS, HORIZ, V0, VR1, VL1, VR2, VL2, VR3, VL3 };

static Mode read_mode(BitStream& bs) {
    int b1 = bs.read_bit();
    if (b1 == 1) {
        int b2 = bs.read_bit();
        if (b2 == 1) return V0;
        int b3 = bs.read_bit();
        if (b3 == 1) return VR1;
        int b4 = bs.read_bit();
        if (b4 == 1) return VL1;
        int b5 = bs.read_bit();
        if (b5 == 1) return VR2;
        int b6 = bs.read_bit();
        if (b6 == 1) return VL2;
        int b7 = bs.read_bit();
        if (b7 == 1) return VR3;
        return VL3;
    } else {
        return (bs.read_bit() == 1) ? HORIZ : PASS;
    }
}

static uint32_t next_change(const std::vector<uint8_t>& row, uint32_t start) {
    uint8_t v = row[start];
    for (uint32_t i = start; i < row.size(); i++)
        if (row[i] != v) return i;
    return row.size();
}

static void decode_row(BitStream& bs,
                       const std::vector<uint8_t>& ref,
                       std::vector<uint8_t>& out) {
    uint32_t a0 = 0;
    uint32_t width = out.size();
    uint8_t color = 0;

    while (a0 < width) {
        Mode m = read_mode(bs);

        uint32_t b1 = next_change(ref, a0);
        uint32_t b2 = next_change(ref, b1);

        if (m == PASS) {
            a0 = b2;
        }
        else if (m == HORIZ) {
            int r1 = (color == 0) ? decode_run(bs, WHITE)
                                 : decode_run(bs, BLACK);
            int r2 = (color == 0) ? decode_run(bs, BLACK)
                                 : decode_run(bs, WHITE);

            for (int i = 0; i < r1 && a0 < width; i++) out[a0++] = color;
            color ^= 1;
            for (int i = 0; i < r2 && a0 < width; i++) out[a0++] = color;
            color ^= 1;
        }
        else {
            int offset = (m == V0)?0:(m==VR1?1:(m==VL1?-1:(m==VR2?2:(m==VL2?-2:(m==VR3?3:-3)))));
            int a1 = (int)b1 + offset;

            while (a0 < (uint32_t)a1 && a0 < width)
                out[a0++] = color;

            color ^= 1;
        }
    }
}

// ---------------- Public API ----------------

bool decode_ccitt_g4(
    const uint8_t* data,
    size_t size,
    uint32_t width,
    uint32_t height,
    uint32_t rows_per_strip,
    std::vector<uint8_t>& out
) {
    out.clear();
    out.reserve(width * height);

    BitStream bs{data, size};
    std::vector<uint8_t> ref(width, 0), curr(width);

    for (uint32_t y = 0; y < height; y++) {
        std::fill(curr.begin(), curr.end(), 0);
        decode_row(bs, ref, curr);

        out.insert(out.end(), curr.begin(), curr.end());
        ref = curr;
    }

    return true;
}

} // namespace tiff_binary
