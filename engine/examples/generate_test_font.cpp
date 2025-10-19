#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>

// Generate a simple 8x8 test font with perfect grid alignment
// Creates a 128x128 PNG (16x16 grid of 8x8 glyphs)

void drawGlyph(SDL_Surface* surface, int gridX, int gridY, int charCode) {
    const int charSize = 8;
    int baseX = gridX * charSize;
    int baseY = gridY * charSize;

    Uint32* pixels = static_cast<Uint32*>(surface->pixels);
    Uint32 white = SDL_MapRGB(surface->format, 255, 255, 255);

    auto setPixel = [&](int dx, int dy) {
        if (dx >= 0 && dx < charSize && dy >= 0 && dy < charSize) {
            pixels[(baseY + dy) * surface->w + (baseX + dx)] = white;
        }
    };

    if (charCode == ' ') {
        // Space - leave blank
    }
    else if (charCode == 'H') {
        // Two vertical lines with horizontal crossbar
        for (int dy = 0; dy < 8; dy++) {
            setPixel(1, dy);
            setPixel(6, dy);
        }
        for (int dx = 2; dx < 6; dx++) {
            setPixel(dx, 4);
        }
    }
    else if (charCode == 'E') {
        for (int dy = 0; dy < 8; dy++) {
            setPixel(1, dy);
        }
        for (int dx = 1; dx < 7; dx++) {
            setPixel(dx, 0);
            setPixel(dx, 4);
            setPixel(dx, 7);
        }
    }
    else if (charCode == 'L') {
        for (int dy = 0; dy < 8; dy++) {
            setPixel(1, dy);
        }
        for (int dx = 1; dx < 7; dx++) {
            setPixel(dx, 7);
        }
    }
    else if (charCode == 'O') {
        // Draw a rectangle
        for (int dy = 1; dy < 7; dy++) {
            setPixel(1, dy);
            setPixel(6, dy);
        }
        for (int dx = 2; dx < 6; dx++) {
            setPixel(dx, 1);
            setPixel(dx, 6);
        }
    }
    else if (charCode == 'W') {
        for (int dy = 0; dy < 7; dy++) {
            setPixel(1, dy);
            setPixel(6, dy);
        }
        for (int dy = 4; dy < 8; dy++) {
            setPixel(3, dy);
            setPixel(4, dy);
        }
    }
    else if (charCode == 'R') {
        // Left vertical line
        for (int dy = 0; dy < 8; dy++) {
            setPixel(1, dy);
        }
        // Top horizontal
        for (int dx = 1; dx < 6; dx++) {
            setPixel(dx, 0);
        }
        // Middle horizontal
        for (int dx = 1; dx < 6; dx++) {
            setPixel(dx, 4);
        }
        // Right side top
        for (int dy = 1; dy < 4; dy++) {
            setPixel(6, dy);
        }
        // Diagonal leg
        for (int i = 0; i < 3; i++) {
            setPixel(4 + i, 5 + i);
        }
    }
    else if (charCode == 'D') {
        // Left vertical line
        for (int dy = 0; dy < 8; dy++) {
            setPixel(1, dy);
        }
        // Top and bottom
        for (int dx = 1; dx < 6; dx++) {
            setPixel(dx, 0);
            setPixel(dx, 7);
        }
        // Right side
        for (int dy = 1; dy < 7; dy++) {
            setPixel(6, dy);
        }
    }
    else if (charCode == '0') {
        // Similar to O
        for (int dy = 1; dy < 7; dy++) {
            setPixel(1, dy);
            setPixel(6, dy);
        }
        for (int dx = 2; dx < 6; dx++) {
            setPixel(dx, 1);
            setPixel(dx, 6);
        }
    }
    else if (charCode >= '1' && charCode <= '9') {
        // Simple vertical line for all numbers (simplified)
        for (int dy = 0; dy < 8; dy++) {
            setPixel(3, dy);
            setPixel(4, dy);
        }
    }
    else if (charCode == '!') {
        for (int dy = 0; dy < 5; dy++) {
            setPixel(3, dy);
            setPixel(4, dy);
        }
        setPixel(3, 7);
        setPixel(4, 7);
    }
    else if (charCode == '.') {
        setPixel(3, 7);
        setPixel(4, 7);
    }
    else {
        // For all other characters, draw a simple test pattern
        // A small square in the center
        for (int dy = 3; dy < 6; dy++) {
            for (int dx = 3; dx < 6; dx++) {
                setPixel(dx, dy);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create 128x128 image (16x16 grid of 8x8 glyphs)
    const int imgSize = 128;
    const int charsPerRow = 16;
    const int charSize = 8;

    SDL_Surface* surface = SDL_CreateRGBSurface(0, imgSize, imgSize, 32,
                                                 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    if (!surface) {
        std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Fill with black
    SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 0, 0, 0));

    SDL_LockSurface(surface);

    // Generate glyphs for ASCII 0-255
    for (int i = 0; i < 256; i++) {
        int gridX = i % charsPerRow;
        int gridY = i / charsPerRow;
        drawGlyph(surface, gridX, gridY, i);
    }

    SDL_UnlockSurface(surface);

    // Save the image
    const char* outputPath = "../examples/resources/test_font_8x8.png";
    if (IMG_SavePNG(surface, outputPath) != 0) {
        std::cerr << "Failed to save PNG: " << IMG_GetError() << std::endl;
        SDL_FreeSurface(surface);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    std::cout << "Generated " << outputPath << ": " << imgSize << "x" << imgSize
              << " pixels, " << charsPerRow << "x" << charsPerRow
              << " grid of " << charSize << "x" << charSize << " glyphs" << std::endl;
    std::cout << "Total glyphs: " << (charsPerRow * charsPerRow) << std::endl;

    SDL_FreeSurface(surface);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
