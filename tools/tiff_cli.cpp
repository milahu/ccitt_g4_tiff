#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "tiff_binary/tiff.hpp"

static void print_image(const tiff_binary::TiffImage& img) {
    std::cout << "TIFF Parsed Successfully\n";
    std::cout << "------------------------\n";
    std::cout << "Width: " << img.width << "\n";
    std::cout << "Height: " << img.height << "\n";

    std::cout << "Compression: ";
    switch (img.compression) {
        case tiff_binary::NONE:
            std::cout << "NONE";
            break;
        case tiff_binary::CCITT_GROUP4:
            std::cout << "CCITT_GROUP4";
            break;
        case tiff_binary::JBIG2:
            std::cout << "JBIG2 (stub)";
            break;
        default:
            std::cout << "UNKNOWN";
            break;
    }
    std::cout << "\n";

    std::cout << "BitsPerSample: " << img.bits_per_sample << "\n";
    std::cout << "SamplesPerPixel: " << img.samples_per_pixel << "\n";

    std::cout << "Strips: " << img.strip_offsets.size() << "\n";

    for (size_t i = 0; i < img.strip_offsets.size(); i++) {
        std::cout << "  Strip " << i
                << ": offset=" << img.strip_offsets[i]
                << ", size=" << img.strip_byte_counts[i]
                << "\n";
    }
}

void write_pgm(
    const std::string& path,
    const std::vector<uint8_t>& packed,
    uint32_t width,
    uint32_t height)
{
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) throw std::runtime_error("fopen failed");

    fprintf(f, "P5\n%u %u\n255\n", width, height);

    size_t pitch = (width + 7) / 8;

    for (uint32_t y = 0; y < height; y++) {
        const uint8_t* row = packed.data() + y * pitch;

        for (uint32_t x = 0; x < width; x++) {
            uint8_t byte = row[x / 8];
            uint8_t bit = (byte >> (7 - (x % 8))) & 1;

            uint8_t pixel = bit ? 0 : 255; // min-is-black
            fwrite(&pixel, 1, 1, f);
        }
    }

    fclose(f);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input.tiff [-o output.pgm]\n", argv[0]);
        return 1;
    }

    const char* input_path = argv[1];
    const char* output_path = nullptr;

    for (int i = 2; i < argc; i++) {
        if (std::string(argv[i]) == "-o") {
            if (i + 1 >= argc) {
                fprintf(stderr, "-o requires a filename\n");
                return 1;
            }
            output_path = argv[i + 1];
            i++; // skip filename
        }
    }

    std::ifstream file(input_path, std::ios::binary);
    if (!file) {
        std::cerr << "Error: cannot open file: " << input_path << "\n";
        return 1;
    }

    // Load entire file into memory (fine for CLI testing)
    std::vector<uint8_t> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    try {
        auto img = tiff_binary::TiffParser::parse(data.data(), data.size());
        print_image(img);

        if (output_path) {
            try {
                printf("Writing decoded image: %s\n", output_path);
                write_pgm(output_path, img.pixels, img.width, img.height);
            } catch (const std::exception& e) {
                fprintf(stderr, "Failed to write output: %s\n", e.what());
                return 1;
            }
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "TIFF parse error: " << e.what() << "\n";
        return 1;
    }
}
