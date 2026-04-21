#include "tiff_binary/ccitt_g4.hpp"
#include <stdexcept>
#include <algorithm>

namespace tiff_binary {

struct BitStream {
    const uint8_t* data;
    size_t size;
    size_t byte_pos = 0;
    int bit_pos = 0;

    int read_bit() {
        if (byte_pos >= size)
            return -1;

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

// --- Simplified helper ---
static void set_run(std::vector<uint8_t>& row, uint32_t start, uint32_t len, uint8_t value) {
    for (uint32_t i = 0; i < len && (start + i) < row.size(); i++) {
        row[start + i] = value;
    }
}

// --- Find next change in reference line ---
static uint32_t next_change(const std::vector<uint8_t>& ref, uint32_t start) {
    uint8_t val = ref[start];
    for (uint32_t i = start; i < ref.size(); i++) {
        if (ref[i] != val)
            return i;
    }
    return ref.size();
}

static inline int read_bits(BitStream& bs, int n) {
    int v = 0;
    for (int i = 0; i < n; i++) {
        int b = bs.read_bit();
        if (b < 0) throw std::runtime_error("EOF in bitstream");
        v = (v << 1) | b;
    }
    return v;
}

static int decode_white_run(BitStream& bs) {
    int code = 0;
    for (int len = 1; len <= 13; len++) {
        code = (code << 1) | bs.read_bit();

        switch ((len << 16) | code) {
            case (4<<16)|0b0011: return 2;
            case (3<<16)|0b010: return 3;
            case (2<<16)|0b11: return 1;
            case (6<<16)|0b000111: return 4;
            case (7<<16)|0b0001000: return 5;
            case (7<<16)|0b0001001: return 6;
            case (7<<16)|0b0001010: return 7;
            case (7<<16)|0b0001011: return 8;
            case (7<<16)|0b0001100: return 9;
            case (7<<16)|0b0001101: return 10;
            case (7<<16)|0b0001110: return 11;
            case (7<<16)|0b0001111: return 12;
            // ... (extend as needed)
        }
    }
    throw std::runtime_error("Invalid white run");
}

static int decode_black_run(BitStream& bs) {
    int code = 0;
    for (int len = 1; len <= 13; len++) {
        code = (code << 1) | bs.read_bit();

        switch ((len << 16) | code) {
            case (2<<16)|0b10: return 3;
            case (3<<16)|0b011: return 2;
            case (2<<16)|0b11: return 1;
            case (4<<16)|0b0011: return 4;
            // ... extend
        }
    }
    throw std::runtime_error("Invalid black run");
}

enum Mode {
    PASS,
    HORIZONTAL,
    VERTICAL_0,
    VERTICAL_R1,
    VERTICAL_L1,
    VERTICAL_R2,
    VERTICAL_L2,
    VERTICAL_R3,
    VERTICAL_L3
};

static Mode read_mode(BitStream& bs) {
    int b1 = bs.read_bit();
    if (b1 == 1) {
        int b2 = bs.read_bit();
        if (b2 == 1) return VERTICAL_0;
        int b3 = bs.read_bit();
        if (b3 == 1) return VERTICAL_R1;
        int b4 = bs.read_bit();
        if (b4 == 1) return VERTICAL_L1;
        int b5 = bs.read_bit();
        if (b5 == 1) return VERTICAL_R2;
        int b6 = bs.read_bit();
        if (b6 == 1) return VERTICAL_L2;
        int b7 = bs.read_bit();
        if (b7 == 1) return VERTICAL_R3;
        return VERTICAL_L3;
    } else {
        int b2 = bs.read_bit();
        if (b2 == 1) return HORIZONTAL;
        return PASS;
    }
}

static uint32_t next_transition(const std::vector<uint8_t>& row, uint32_t start) {
    uint8_t v = row[start];
    for (uint32_t i = start; i < row.size(); i++) {
        if (row[i] != v)
            return i;
    }
    return row.size();
}

static void decode_row(BitStream& bs,
                       const std::vector<uint8_t>& ref,
                       std::vector<uint8_t>& out) {
    uint32_t width = out.size();
    uint32_t a0 = 0;
    uint8_t color = 0;

    while (a0 < width) {
        Mode m = read_mode(bs);

        uint32_t b1 = next_transition(ref, a0);
        uint32_t b2 = next_transition(ref, b1);

        if (m == PASS) {
            a0 = b2;
        }
        else if (m == HORIZONTAL) {
            int run1 = (color == 0) ? decode_white_run(bs) : decode_black_run(bs);
            int run2 = (color == 0) ? decode_black_run(bs) : decode_white_run(bs);

            for (int i = 0; i < run1 && a0 < width; i++) out[a0++] = color;
            color ^= 1;
            for (int i = 0; i < run2 && a0 < width; i++) out[a0++] = color;
            color ^= 1;
        }
        else {
            int offset = 0;
            switch (m) {
                case VERTICAL_0: offset = 0; break;
                case VERTICAL_R1: offset = 1; break;
                case VERTICAL_L1: offset = -1; break;
                case VERTICAL_R2: offset = 2; break;
                case VERTICAL_L2: offset = -2; break;
                case VERTICAL_R3: offset = 3; break;
                case VERTICAL_L3: offset = -3; break;
                default: break;
            }

            int a1 = (int)b1 + offset;

            while (a0 < (uint32_t)a1 && a0 < width) {
                out[a0++] = color;
            }

            color ^= 1;
        }
    }
}

// --- Strip decoding ---
static void decode_strip(const uint8_t* data,
                         size_t size,
                         uint32_t width,
                         uint32_t rows,
                         std::vector<uint8_t>& out,
                         std::vector<uint8_t>& ref_row) {

    BitStream bs{data, size};

    std::vector<uint8_t> curr(width, 0);

    for (uint32_t y = 0; y < rows; y++) {
        std::fill(curr.begin(), curr.end(), 0);

        decode_row(bs, ref_row, curr);

        out.insert(out.end(), curr.begin(), curr.end());
        ref_row = curr;
    }
}

// --- Public API ---
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

    std::vector<uint8_t> ref_row(width, 0);

    uint32_t rows_decoded = 0;

    while (rows_decoded < height) {
        uint32_t rows = std::min(rows_per_strip, height - rows_decoded);

        decode_strip(data, size, width, rows, out, ref_row);

        rows_decoded += rows;
    }

    if (out.size() != width * height) {
        throw std::runtime_error("CCITT decode size mismatch");
    }

    return true;
}

} // namespace tiff_binary
