#include <SDL.h>
#include <SDL_image.h>
#include <iostream>

// Convert the 225x225 C64 charset to clean 128x128 (16x8 grid of 8x8 chars)
int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Load the original 225x225 font
    SDL_Surface* source = IMG_Load("examples/resources/c64charset.png");
    if (!source) {
        std::cerr << "Failed to load source font: " << IMG_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Convert to RGBA
    SDL_Surface* sourceRGBA = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(source);

    // Create 128x128 output (16x8 = 128 glyphs, 8x8 each)
    SDL_Surface* dest = SDL_CreateRGBSurface(0, 128, 64, 32,
                                              0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

    SDL_LockSurface(sourceRGBA);
    SDL_LockSurface(dest);

    Uint32* srcPixels = static_cast<Uint32*>(sourceRGBA->pixels);
    Uint32* dstPixels = static_cast<Uint32*>(dest->pixels);

    // Extract 64 characters (first 4 rows of 16)
    const int numChars = 64;
    const int charsPerRow = 16;

    for (int i = 0; i < numChars; i++) {
        // Calculate source position with floating point for accuracy
        float srcCellSize = 225.0f / 16.0f;  // 14.0625
        float srcX = (i % charsPerRow) * srcCellSize;
        float srcY = (i / charsPerRow) * srcCellSize;

        // Calculate dest position (clean 8x8 grid)
        int dstX = (i % charsPerRow) * 8;
        int dstY = (i / charsPerRow) * 8;

        // Extract center 8x8 from source cell
        int srcStartX = static_cast<int>(srcX + 3);  // Offset by 3 to get center
        int srcStartY = static_cast<int>(srcY + 3);

        // Copy 8x8 pixels
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                int sx = srcStartX + x;
                int sy = srcStartY + y;

                if (sx >= 0 && sx < sourceRGBA->w && sy >= 0 && sy < sourceRGBA->h) {
                    Uint32 pixel = srcPixels[sy * sourceRGBA->w + sx];
                    dstPixels[(dstY + y) * dest->w + (dstX + x)] = pixel;
                }
            }
        }
    }

    SDL_UnlockSurface(dest);
    SDL_UnlockSurface(sourceRGBA);

    // Save
    const char* outputPath = "examples/resources/c64font_8x8.png";
    if (IMG_SavePNG(dest, outputPath) != 0) {
        std::cerr << "Failed to save PNG: " << IMG_GetError() << std::endl;
        SDL_FreeSurface(dest);
        SDL_FreeSurface(sourceRGBA);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    std::cout << "Created clean 8x8 C64 font: " << outputPath << std::endl;
    std::cout << "  Size: 128x64 (16x8 grid of 8x8 glyphs)" << std::endl;
    std::cout << "  Total: 128 characters" << std::endl;

    SDL_FreeSurface(dest);
    SDL_FreeSurface(sourceRGBA);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
