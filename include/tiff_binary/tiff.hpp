#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <limits>

#include "tiff_binary/ccitt_g4.hpp"

namespace tiff_binary {

enum Tag : uint16_t {
    IMAGE_WIDTH = 256,
    IMAGE_LENGTH = 257,
    BITS_PER_SAMPLE = 258,
    COMPRESSION = 259,
    PHOTOMETRIC = 262,
    STRIP_OFFSETS = 273,
    SAMPLES_PER_PIXEL = 277,
    ROWS_PER_STRIP = 278,
    STRIP_BYTE_COUNTS = 279
};

enum Compression : uint32_t {
    NONE = 1,
    CCITT_GROUP4 = 4,
    JBIG2 = 34712
};

struct StripView {
    const uint8_t* data;
    uint32_t size;
};

struct TiffImage {
    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t compression = 0;

    std::vector<uint32_t> strip_offsets;
    std::vector<uint32_t> strip_byte_counts;

    uint16_t bits_per_sample = 1;
    uint16_t samples_per_pixel = 1;

    std::vector<uint8_t> pixels;

    std::vector<StripView> get_strips(const uint8_t* file_data) const {
        std::vector<StripView> out;

        for (size_t i = 0; i < strip_offsets.size(); i++) {
            StripView s;
            s.data = file_data + strip_offsets[i];
            s.size = strip_byte_counts[i];
            out.push_back(s);
        }

        return out;
    }
};

/*
static std::vector<StripView> get_strips(
    const TiffImage& img,
    const uint8_t* file_data
) {
    std::vector<StripView> out;

    for (size_t i = 0; i < img.strip_offsets.size(); i++) {
        out.push_back(StripView{
            file_data + img.strip_offsets[i],
            img.strip_byte_counts[i]
        });
    }

    return out;
}
*/

class ByteReader {
public:
    ByteReader(const uint8_t* data, size_t size)
        : data_(data), size_(size), offset_(0) {}

    template <typename T>
    T read() {
        if (offset_ + sizeof(T) > size_) {
            throw std::runtime_error("EOF");
        }

        T value;
        std::memcpy(&value, data_ + offset_, sizeof(T));
        offset_ += sizeof(T);
        return value;
    }

    void skip(size_t n) {
        if (offset_ + n > size_) {
            throw std::runtime_error("EOF");
        }
        offset_ += n;
    }

    void seek(size_t pos) {
        if (pos > size_) {
            throw std::runtime_error("OOB seek");
        }
        offset_ = pos;
    }

    size_t pos() const { return offset_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t offset_;
};

struct TiffContext {
    const uint8_t* data;
    size_t size;
    bool little_endian;
};

enum TiffType : uint16_t {
    BYTE = 1,
    ASCII = 2,
    SHORT = 3,
    LONG = 4
};

static uint32_t read_u32(const uint8_t* p, bool le) {
    if (le) {
        return (uint32_t)p[0]
             | (uint32_t)p[1] << 8
             | (uint32_t)p[2] << 16
             | (uint32_t)p[3] << 24;
    } else {
        return (uint32_t)p[3]
             | (uint32_t)p[2] << 8
             | (uint32_t)p[1] << 16
             | (uint32_t)p[0] << 24;
    }
}

static bool is_inline_value(uint16_t type, uint32_t count) {
    uint32_t size = 0;

    switch (type) {
        case BYTE:  size = 1; break;
        case ASCII: size = 1; break;
        case SHORT: size = 2; break;
        case LONG:  size = 4; break;
        default: return false;
    }

    return (size * count) <= 4;
}

static uint32_t read_value(
    const uint8_t* base,
    size_t size,
    bool le,
    uint16_t type,
    uint32_t count,
    uint32_t value_or_offset
) {
    if (is_inline_value(type, count)) {
        return value_or_offset;
    }

    if (value_or_offset + 4 > size) {
        throw std::runtime_error("Invalid TIFF offset");
    }

    return read_u32(base + value_or_offset, le);
}

static inline uint32_t read_u32_le(const uint8_t* data, size_t size, uint32_t offset) {
    if (offset + 4 > size) {
        throw std::runtime_error("Out of bounds read");
    }

    return (uint32_t)data[offset]
         | (uint32_t)data[offset + 1] << 8
         | (uint32_t)data[offset + 2] << 16
         | (uint32_t)data[offset + 3] << 24;
}

class BitReader {
public:
    BitReader(const uint8_t* data, size_t size)
        : data_(data), size_(size) {}

    uint32_t read_bit() {
        if (byte_pos_ >= size_) {
            throw std::runtime_error("BitReader EOF");
        }

        uint8_t byte = data_[byte_pos_];

        // MSB-first (default TIFF FillOrder=1)
        uint32_t bit = (byte >> (7 - bit_pos_)) & 1;

        bit_pos_++;
        if (bit_pos_ == 8) {
            bit_pos_ = 0;
            byte_pos_++;
        }

        return bit;
    }

private:
    const uint8_t* data_;
    size_t size_;

    size_t byte_pos_ = 0;
    int bit_pos_ = 0;
};

class TiffParser {
public:
    static TiffImage parse(const uint8_t* data, size_t size) {
        ByteReader r(data, size);

        // -----------------------------
        // 1. TIFF HEADER
        // -----------------------------
        uint16_t byte_order = r.read<uint16_t>();

        bool little_endian;
        if (byte_order == 0x4949) { // "II"
            little_endian = true;
        } else if (byte_order == 0x4D4D) { // "MM"
            little_endian = false;
        } else {
            throw std::runtime_error("Invalid TIFF byte order");
        }

        // We ignore dynamic endian swapping for simplicity here,
        // but in production you'd wrap reads accordingly.

        uint16_t magic = r.read<uint16_t>();
        if (magic != 42) {
            throw std::runtime_error("Invalid TIFF magic");
        }

        uint32_t ifd_offset = r.read<uint32_t>();
        if (ifd_offset >= size) {
            throw std::runtime_error("Invalid IFD offset");
        }

        // -----------------------------
        // 2. GO TO IFD
        // -----------------------------
        r.seek(ifd_offset);

        uint16_t entry_count = r.read<uint16_t>();
        if (entry_count == 0 || entry_count > 50) {
            throw std::runtime_error("Invalid IFD entry count");
        }

        // -----------------------------
        // 3. TAG PARSING
        // -----------------------------
        TiffImage img;

        bool has_width = false;
        bool has_height = false;
        bool has_strip_offset = false;
        bool has_strip_count = false;

        uint32_t strip_offsets_count = 0;
        uint32_t strip_offsets_vo = 0;

        uint32_t strip_byte_counts_count = 0;
        uint32_t strip_byte_counts_vo = 0;

        uint32_t rows_per_strip = 0;

        for (int i = 0; i < entry_count; i++) {
            uint16_t tag = r.read<uint16_t>();
            uint16_t type = r.read<uint16_t>();
            uint32_t count = r.read<uint32_t>();
            uint32_t value_or_offset = r.read<uint32_t>();

            if (count == 0) {
                throw std::runtime_error("Invalid strip count zero");
            }

            switch (tag) {
                case IMAGE_WIDTH:
                    img.width = read_value(data, size, little_endian, type, count, value_or_offset);
                    has_width = true;
                    break;

                case IMAGE_LENGTH:
                    img.height = read_value(data, size, little_endian, type, count, value_or_offset);
                    has_height = true;
                    break;

                case COMPRESSION:
                    img.compression = read_value(data, size, little_endian, type, count, value_or_offset);
                    break;

                case BITS_PER_SAMPLE:
                    img.bits_per_sample = (uint16_t) read_value(data, size, little_endian, type, count, value_or_offset);;
                    break;

                case SAMPLES_PER_PIXEL:
                    img.samples_per_pixel = (uint16_t) read_value(data, size, little_endian, type, count, value_or_offset);;
                    break;

                case STRIP_OFFSETS:
                    strip_offsets_count = count;
                    strip_offsets_vo = value_or_offset;
                    has_strip_offset = true;
                    break;

                case STRIP_BYTE_COUNTS:
                    strip_byte_counts_count = count;
                    strip_byte_counts_vo = value_or_offset;
                    has_strip_count = true;
                    break;

                case ROWS_PER_STRIP:
                    rows_per_strip = read_value(data, size, little_endian, type, count, value_or_offset);
                    break;

                default:
                    // ignore unknown tags
                    continue;
            }
        }

        // -----------------------------
        // 4. VALIDATION (VERY IMPORTANT)
        // -----------------------------

        if (!has_width || !has_height)
            throw std::runtime_error("Missing dimensions");

        if (!has_strip_offset || !has_strip_count)
            throw std::runtime_error("Missing strip data");

        if (img.bits_per_sample != 1)
            throw std::runtime_error("Only 1-bit supported");

        if (img.samples_per_pixel != 1)
            throw std::runtime_error("Only 1 sample per pixel supported");

        if (img.compression != CCITT_GROUP4 &&
            img.compression != JBIG2 &&
            img.compression != NONE) {
            throw std::runtime_error("Unsupported compression");
        }

        if (strip_offsets_count == 0 || strip_byte_counts_count == 0) {
            throw std::runtime_error("Missing strip data");
        }

        if (strip_offsets_count != strip_byte_counts_count) {
            throw std::runtime_error("Mismatched strip arrays");
        }

        if (strip_offsets_count != strip_byte_counts_count) {
            throw std::runtime_error("Mismatched strip arrays");
        }

        if (rows_per_strip == 0) {
            rows_per_strip = img.height; // fallback (legal TIFF behavior)
        }

        auto read_array = [&](
            uint32_t count,
            uint32_t value_or_offset
        ) {
            std::vector<uint32_t> out;

            if (count == 1) {
                // inline value
                out.push_back(value_or_offset);
                return out;
            }

            // array stored at offset
            for (uint32_t i = 0; i < count; i++) {
                uint32_t v = read_u32_le(data, size, value_or_offset + i * 4);
                out.push_back(v);
            }

            return out;
        };

        img.strip_offsets = read_array(
            strip_offsets_count,
            strip_offsets_vo
        );

        img.strip_byte_counts = read_array(
            strip_byte_counts_count,
            strip_byte_counts_vo
        );

        for (size_t i = 0; i < img.strip_offsets.size(); i++) {
            uint32_t off = img.strip_offsets[i];
            uint32_t len = img.strip_byte_counts[i];

            if (off == 0 || len == 0) {
                throw std::runtime_error("Invalid strip entry");
            }

            if ((uint64_t)off + (uint64_t)len > size) {
                throw std::runtime_error("Strip out of bounds");
            }
        }

        std::vector<StripView> strips;

        for (size_t i = 0; i < img.strip_offsets.size(); i++) {
            strips.push_back({
                data + img.strip_offsets[i],
                img.strip_byte_counts[i]
            });
        }

        std::vector<uint8_t> decoded;
        decoded.reserve(img.width * img.height);

        uint32_t rows_decoded = 0;
        std::vector<uint8_t> ref_row(img.width, 0);

        for (size_t i = 0; i < strips.size(); i++) {
            const auto& strip = strips[i];

            uint32_t rows = std::min(rows_per_strip, img.height - rows_decoded);

            std::vector<uint8_t> strip_out;
            strip_out.reserve(rows * img.width);

            // debug
            std::cout
                << "Decoding strip " << i
                << " rows=" << rows
                << " offset=" << img.strip_offsets[i]
                << "\n"
            ;

            uint32_t done_input_bytes = decode_ccitt_g4(
                strip.data,
                strip.size,
                img.width,
                rows_per_strip, // "height"
                strip_out
            );

            // TODO? check done_input_bytes

            #if false
            // debug
            uint32_t pitch = (img.width + 7) / 8;
            std::cout << "img.width: " << img.width << "\n";
            std::cout << "img.height: " << img.height << "\n";
            std::cout << "pitch: " << pitch << "\n";
            std::cout << "rows_per_strip: " << rows_per_strip << "\n";
            std::cout << "done_input_bytes: " << done_input_bytes << "\n";
            #endif

            // append to final image
            decoded.insert(decoded.end(), strip_out.begin(), strip_out.end());

            rows_decoded += rows;
        }

        img.pixels = std::move(decoded);

        return img;
    }
};

} // namespace tiff_binary
