#include "engine/PixelFont.h"
#include "engine/ResourceManager.h"
#include <iostream>

int main() {
    Engine::ResourceManager rm;

    auto font = rm.loadPixelFont(
        "atascii",
        "examples/resources/atascii.gif",
        8, 8, 16, "", 0
    );

    if (!font) {
        std::cerr << "Failed to load font!" << std::endl;
        return 1;
    }

    std::cout << "Font loaded successfully!" << std::endl;
    std::cout << "Char size: " << font->getCharWidth() << "x" << font->getCharHeight() << std::endl;

    // Check specific characters
    const char* testChars = "HELLO WORLD";
    for (int i = 0; testChars[i]; i++) {
        char c = testChars[i];
        if (font->hasGlyph(c)) {
            std::cout << "'" << c << "' (ASCII " << (int)c << "): ";
            const auto& glyph = font->getGlyph(c);

            // Print the glyph as ASCII art
            std::cout << std::endl;
            for (int y = 0; y < 8; y++) {
                std::cout << "  ";
                for (int x = 0; x < 8; x++) {
                    const auto& pixel = glyph[y * 8 + x];
                    std::cout << (pixel.r > 127 ? '#' : '.');
                }
                std::cout << std::endl;
            }
        } else {
            std::cout << "'" << c << "' (ASCII " << (int)c << "): NOT FOUND" << std::endl;
        }
    }

    return 0;
}
