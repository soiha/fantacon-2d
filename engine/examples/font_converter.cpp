#include <SDL_image.h>
#include <iostream>
#include <fstream>
#include <vector>

// Simple tool to convert C64 PNG font to raw binary format
// Each glyph is 8x8 bytes (0 or 1)
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: font_converter <input.png> <output.bin>" << std::endl;
        return 1;
    }

    // Load PNG
    SDL_Surface* surface = IMG_Load(argv[1]);
    if (!surface) {
        std::cerr << "Failed to load: " << argv[1] << std::endl;
        return 1;
    }

    // Convert to RGBA
    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);

    if (!rgba) {
        std::cerr << "Failed to convert surface format" << std::endl;
        return 1;
    }

    SDL_LockSurface(rgba);
    Uint32* pixels = static_cast<Uint32*>(rgba->pixels);

    // Extract all 64 glyphs - try extracting from top-left 8x8 of each cell
    // PNG is 225x225, so 225/16 = 14.0625, each source cell is approximately 14x14
    std::vector<uint8_t> fontData;
    int srcCharWidth = 14;
    int srcCharHeight = 14;
    int dstCharWidth = 8;
    int dstCharHeight = 8;
    int charsPerRow = 16;
    int numGlyphs = 64;  // 4 rows × 16 cols (only upper half)

    for (int i = 0; i < numGlyphs; ++i) {
        int gridX = (i % charsPerRow) * srcCharWidth;
        int gridY = (i / charsPerRow) * srcCharHeight;

        std::cout << "Glyph " << i << " at (" << gridX << "," << gridY << "): ";

        // Extract top-left 8x8 pixels from each cell
        for (int dy = 0; dy < dstCharHeight; ++dy) {
            for (int dx = 0; dx < dstCharWidth; ++dx) {
                // Extract from top-left of cell
                int sx = gridX + dx;
                int sy = gridY + dy;

                Uint32 pixel = pixels[sy * rgba->w + sx];

                uint8_t r, g, b, a;
                SDL_GetRGBA(pixel, rgba->format, &r, &g, &b, &a);

                // White (>127) = 1, Black (<=127) = 0
                uint8_t value = (r > 127) ? 1 : 0;
                fontData.push_back(value);
            }
        }

        std::cout << "OK" << std::endl;
    }

    SDL_UnlockSurface(rgba);
    SDL_FreeSurface(rgba);

    // Write to file
    std::ofstream out(argv[2], std::ios::binary);
    out.write(reinterpret_cast<const char*>(fontData.data()), fontData.size());
    out.close();

    std::cout << "Wrote " << fontData.size() << " bytes (" << numGlyphs << " glyphs)" << std::endl;

    return 0;
}
