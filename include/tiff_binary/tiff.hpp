#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <limits>

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

struct TiffImage {
    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t compression = 0;

    uint32_t strip_offset = 0;
    uint32_t strip_byte_count = 0;

    uint16_t bits_per_sample = 1;
    uint16_t samples_per_pixel = 1;
};

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

        for (int i = 0; i < entry_count; i++) {
            uint16_t tag = r.read<uint16_t>();
            uint16_t type = r.read<uint16_t>(); // ignored (we enforce uint32-like usage)
            uint32_t count = r.read<uint32_t>();
            uint32_t value_or_offset = r.read<uint32_t>();

            switch (tag) {
                case IMAGE_WIDTH:
                    img.width = value_or_offset;
                    has_width = true;
                    break;

                case IMAGE_LENGTH:
                    img.height = value_or_offset;
                    has_height = true;
                    break;

                case COMPRESSION:
                    img.compression = value_or_offset;
                    break;

                case BITS_PER_SAMPLE:
                    img.bits_per_sample = (uint16_t)value_or_offset;
                    break;

                case SAMPLES_PER_PIXEL:
                    img.samples_per_pixel = (uint16_t)value_or_offset;
                    break;

                case STRIP_OFFSETS:
                    img.strip_offset = value_or_offset;
                    has_strip_offset = true;
                    break;

                case STRIP_BYTE_COUNTS:
                    img.strip_byte_count = value_or_offset;
                    has_strip_count = true;
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

        // -----------------------------
        // 5. STRICT SINGLE-STRIP RULE
        // -----------------------------
        if (img.strip_offset == 0 || img.strip_byte_count == 0) {
            throw std::runtime_error("Invalid strip");
        }

        if (img.strip_offset + img.strip_byte_count > size) {
            throw std::runtime_error("Strip out of bounds");
        }

        return img;
    }
};

} // namespace tiff_binary
