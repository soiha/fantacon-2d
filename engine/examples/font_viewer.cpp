#include <iostream>
#include <fstream>
#include <vector>

// Simple tool to view glyphs from binary font file
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: font_viewer <font.bin> [glyph_index]" << std::endl;
        return 1;
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open: " << argv[1] << std::endl;
        return 1;
    }

    // Read all font data
    std::vector<uint8_t> fontData((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
    in.close();

    // Detect glyph size from file size
    int charWidth, charHeight;
    if (fontData.size() == 12544) {  // 64 glyphs × 14×14
        charWidth = 14;
        charHeight = 14;
    } else if (fontData.size() == 4096) {  // 64 glyphs × 8×8
        charWidth = 8;
        charHeight = 8;
    } else {
        std::cerr << "Unknown font format (size: " << fontData.size() << ")" << std::endl;
        return 1;
    }

    int bytesPerGlyph = charWidth * charHeight;
    int numGlyphs = fontData.size() / bytesPerGlyph;

    std::cout << "Font file: " << argv[1] << std::endl;
    std::cout << "Total bytes: " << fontData.size() << std::endl;
    std::cout << "Glyphs: " << numGlyphs << " (" << charWidth << "x" << charHeight << " each)" << std::endl;
    std::cout << std::endl;

    // If glyph index specified, show just that one
    if (argc >= 3) {
        int glyphIndex = std::atoi(argv[2]);
        if (glyphIndex < 0 || glyphIndex >= numGlyphs) {
            std::cerr << "Glyph index out of range (0-" << (numGlyphs - 1) << ")" << std::endl;
            return 1;
        }

        std::cout << "Glyph " << glyphIndex << ":" << std::endl;
        int offset = glyphIndex * bytesPerGlyph;
        for (int y = 0; y < charHeight; ++y) {
            for (int x = 0; x < charWidth; ++x) {
                uint8_t pixel = fontData[offset + y * charWidth + x];
                std::cout << (pixel ? "█" : " ");
            }
            std::cout << std::endl;
        }
    } else {
        // Show first 10 glyphs
        for (int g = 0; g < std::min(10, numGlyphs); ++g) {
            std::cout << "Glyph " << g << ":" << std::endl;
            int offset = g * bytesPerGlyph;
            for (int y = 0; y < charHeight; ++y) {
                for (int x = 0; x < charWidth; ++x) {
                    uint8_t pixel = fontData[offset + y * charWidth + x];
                    std::cout << (pixel ? "█" : " ");
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    }

    return 0;
}
