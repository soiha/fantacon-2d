#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/IUpdateable.h"
#include "engine/Layer.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/AttributedTextGrid.h"
#include "engine/Palette.h"
#include "engine/ResourceManager.h"
#include "engine/GLRenderer.h"
#include "engine/FPSCounter.h"
#include "engine/Logger.h"
#include <SDL.h>
#include <cmath>

// Demo showing AttributedTextGrid - VGA text mode style display
class TextGridDemo : public Engine::GameObject,
                      public Engine::IUpdateable {
public:
    TextGridDemo() : GameObject("TextGridDemo") {}

    void onAttached() override {
        LOG_INFO("TextGridDemo attached!");

        // Set up a classic VGA-style 16-color palette
        Engine::Palette palette;
        // Classic VGA colors
        palette.setColor(0, {0, 0, 0});         // Black
        palette.setColor(1, {0, 0, 170});       // Blue
        palette.setColor(2, {0, 170, 0});       // Green
        palette.setColor(3, {0, 170, 170});     // Cyan
        palette.setColor(4, {170, 0, 0});       // Red
        palette.setColor(5, {170, 0, 170});     // Magenta
        palette.setColor(6, {170, 85, 0});      // Brown
        palette.setColor(7, {170, 170, 170});   // Light Gray
        palette.setColor(8, {85, 85, 85});      // Dark Gray
        palette.setColor(9, {85, 85, 255});     // Bright Blue
        palette.setColor(10, {85, 255, 85});    // Bright Green
        palette.setColor(11, {85, 255, 255});   // Bright Cyan
        palette.setColor(12, {255, 85, 85});    // Bright Red
        palette.setColor(13, {255, 85, 255});   // Bright Magenta
        palette.setColor(14, {255, 255, 85});   // Yellow
        palette.setColor(15, {255, 255, 255});  // White

        // Get the layer to attach text grid to
        auto& layers = getEngine()->getLayers();
        if (layers.empty()) {
            LOG_ERROR("No layers available!");
            return;
        }
        layer_ = layers[0];

        // Load the ATASCII font (16x16 characters)
        auto& resourceMgr = getEngine()->getResourceManager();
        font_ = resourceMgr.loadPixelFont(
            "atascii",
            "examples/resources/atascii.gif",
            16,  // char width
            16,  // char height
            16,  // chars per row
            "",  // use standard ASCII mapping
            0,   // first char code
            256  // char count (load full 16x16 grid)
        );

        if (!font_) {
            LOG_ERROR("Failed to load font!");
            return;
        }

        // Create text grid WIDER than viewport for scrolling demo
        // Viewport: 960x600, 16x16 font = 60x37 chars visible
        // Let's make it 100 chars wide so we have 40 extra chars to scroll
        textGrid_ = std::make_shared<Engine::AttributedTextGrid>(font_, 100, 37);
        textGrid_->setPosition(Engine::Vec2{0, 0});
        textGrid_->getBuffer()->setPalette(palette.getColors());

        // Define some text attributes (foreground, background)
        textGrid_->setAttributeDef(0, 7, 0);   // Light gray on black (default)
        textGrid_->setAttributeDef(1, 15, 1);  // White on blue (header)
        textGrid_->setAttributeDef(2, 14, 0);  // Yellow on black (highlighted)
        textGrid_->setAttributeDef(3, 10, 0);  // Bright green on black (success)
        textGrid_->setAttributeDef(4, 12, 0);  // Bright red on black (error)
        textGrid_->setAttributeDef(5, 11, 0);  // Bright cyan on black (info)
        textGrid_->setAttributeDef(6, 0, 7);   // Black on light gray (inverted)
        textGrid_->setAttributeDef(7, 13, 0);  // Bright magenta on black (special)
        textGrid_->setAttributeDef(8, 9, 0);   // Bright blue on black
        textGrid_->setAttributeDef(9, 15, 4);  // White on red (alert)

        // Draw initial content
        drawContent();

        // Add text grid to layer
        layer_->addAttachable(textGrid_);

        LOG_INFO("Created AttributedTextGrid: 100x37 characters (60 visible, 40 off-screen for scrolling)!");
    }

    void drawContent() {
        // Clear with default attribute
        textGrid_->clear(' ', 0);

        // Title bar (white on blue) - spans full 100 char width
        textGrid_->fill(0, 0, 100, 1, ' ', 1);
        textGrid_->print(2, 0, " ATTRIBUTED TEXT GRID DEMO ", 1);
        textGrid_->print(50, 0, "[ESC:Q]", 1);
        textGrid_->print(70, 0, "SCROLL TO SEE MORE -->", 1);

        // Section headers and content
        textGrid_->print(2, 2, "16-COLOR VGA PALETTE:", 2);

        // Color swatches - show all 16 colors
        for (int i = 0; i < 16; ++i) {
            int x = 2 + (i % 8) * 14;
            int y = 4 + (i / 8) * 2;

            // Create attribute with this color as background
            uint8_t attr = 10 + i;
            textGrid_->setAttributeDef(attr, 0, i);  // Black on color i

            // Draw color swatch
            for (int dx = 0; dx < 12; ++dx) {
                textGrid_->setCell(x + dx, y, ' ', attr);
            }

            // Label with color number
            char label[16];
            snprintf(label, sizeof(label), "COLOR %2d", i);
            textGrid_->print(x + 1, y, label, attr);
        }

        // Text attributes demonstration
        textGrid_->print(2, 9, "TEXT ATTRIBUTES:", 2);
        textGrid_->print(2, 11, "DEFAULT: Light gray on black (attr 0)", 0);
        textGrid_->print(2, 12, "HEADER: White on blue (attr 1)", 1);
        textGrid_->print(2, 13, "HIGHLIGHTED: Yellow on black (attr 2)", 2);
        textGrid_->print(2, 14, "SUCCESS: Bright green on black (attr 3)", 3);
        textGrid_->print(2, 15, "ERROR: Bright red on black (attr 4)", 4);
        textGrid_->print(2, 16, "INFO: Bright cyan on black (attr 5)", 5);
        textGrid_->print(2, 17, "INVERTED: Black on light gray (attr 6)", 6);
        textGrid_->print(2, 18, "SPECIAL: Bright magenta on black (attr 7)", 7);

        // Box drawing (using ASCII art)
        textGrid_->print(2, 20, "+--------------------------------------------------+", 2);
        textGrid_->print(2, 21, "|  This is like classic VGA text mode from DOS!   |", 2);
        textGrid_->print(2, 22, "|  Each character cell has:                       |", 2);
        textGrid_->print(2, 23, "|  - 8-bit glyph index (character)                |", 2);
        textGrid_->print(2, 24, "|  - 8-bit attribute index (256 possible)         |", 2);
        textGrid_->print(2, 25, "|  Each attribute defines:                        |", 2);
        textGrid_->print(2, 26, "|  - Foreground color (palette index)             |", 2);
        textGrid_->print(2, 27, "|  - Background color (palette index)             |", 2);
        textGrid_->print(2, 28, "+--------------------------------------------------+", 2);

        // Animated status line
        textGrid_->print(2, 30, "ANIMATED CONTENT (updated each frame):", 5);

        // Additional content on the right side (will scroll into view)
        textGrid_->print(65, 2, "Extra Content Area:", 2);
        textGrid_->print(65, 4, "This content is off-screen", 3);
        textGrid_->print(65, 5, "when centered, but scrolls", 3);
        textGrid_->print(65, 6, "into view from the right!", 3);

        textGrid_->print(65, 9, "Grid is 100 chars wide", 5);
        textGrid_->print(65, 10, "Viewport shows 60 chars", 5);
        textGrid_->print(65, 11, "= 40 chars off-screen!", 5);

        // Draw a vertical bar to show the boundary
        for (int y = 2; y < 35; ++y) {
            textGrid_->setCell(63, y, '|', 2);
        }
        textGrid_->print(61, 14, "60", 4);
        textGrid_->print(61, 15, "char", 4);
        textGrid_->print(61, 16, "mark", 4);

        // Footer - spans full width
        textGrid_->fill(0, 36, 100, 1, ' ', 1);
        textGrid_->print(2, 36, "100x37 text grid | 16-color VGA | ATASCII 16x16", 1);
        textGrid_->print(80, 36, "<-- Scroll Left", 1);
    }

    void update(float deltaTime) override {
        time_ += deltaTime;

        // Horizontal scrolling animation - smooth sine wave motion
        // Grid is 100 chars * 16 pixels = 1600 pixels wide
        // Viewport is 960 pixels wide, so we need to scroll -640 pixels to see the right edge
        // Scroll from 0 (left edge visible) to -640 (right edge visible)
        // Use (sin - 1) / 2 to map sine wave from [-1, 1] to [-1, 0], then scale by 640
        float scrollOffset = (std::sin(time_ * 0.5f) - 1.0f) * 0.5f * 640.0f;  // Scroll 0 to -640 pixels
        textGrid_->setPosition(Engine::Vec2{scrollOffset, 0.0f});

        // Update animated status line
        int animFrame = static_cast<int>(time_ * 10) % 40;

        // Progress bar animation
        char progressBar[50];
        int filled = (animFrame * 40) / 40;
        for (int i = 0; i < 40; ++i) {
            progressBar[i] = (i < filled) ? '#' : '-';
        }
        progressBar[40] = '\0';

        char statusText[100];
        snprintf(statusText, sizeof(statusText), "Progress: [%s] %3d%%", progressBar, (animFrame * 100) / 40);
        textGrid_->print(2, 32, statusText, 3);

        // Blinking alert (every 0.5 seconds)
        bool alertOn = (static_cast<int>(time_ * 2) % 2) == 0;
        if (alertOn) {
            textGrid_->print(2, 34, ">>> ALERT: This text blinks! <<<", 9);
        } else {
            textGrid_->print(2, 34, "                                 ", 0);
        }

        // Cycling colors
        int colorIndex = (static_cast<int>(time_ * 3) % 7) + 2;  // Cycle through attrs 2-8
        textGrid_->print(2, 36, "Color cycling text...", colorIndex);

        // No manual rendering needed - the layer handles it!
    }

    void onDestroy() override {
        // Cleanup handled automatically by layer
    }

private:
    std::shared_ptr<Engine::Layer> layer_;
    std::shared_ptr<Engine::PixelFont> font_;
    std::shared_ptr<Engine::AttributedTextGrid> textGrid_;
    float time_ = 0.0f;
};

// Simple FPS counter
class FPSDisplay : public Engine::GameObject,
                   public Engine::IUpdateable {
public:
    FPSDisplay() : GameObject("FPSDisplay"), fpsCounter_(60) {}

    void update(float deltaTime) override {
        fpsCounter_.update();

        timeSinceUpdate_ += deltaTime;
        if (timeSinceUpdate_ >= 1.0f) {
            LOG_INFO_FMT("FPS: %.1f | AttributedTextGrid Demo", fpsCounter_.getFPS());
            timeSinceUpdate_ = 0.0f;
        }
    }

private:
    Engine::FPSCounter fpsCounter_;
    float timeSinceUpdate_ = 0.0f;
};

int main(int argc, char* argv[]) {
    // Configure engine
    Engine::EngineConfig config;
    config.title = "AttributedTextGrid Demo - VGA Text Mode";
    config.width = 960;
    config.height = 600;
    config.logLevel = Engine::LogLevel::Info;
    config.logToFile = false;
    config.showTimestamp = true;
    config.showWallClock = true;
    config.showFrame = true;
    config.showLogLevel = true;

    // Create engine with GLRenderer
    Engine::Engine engine;

    auto glRenderer = std::make_unique<Engine::GLRenderer>();
    if (!glRenderer->init(config.title, config.width, config.height)) {
        LOG_ERROR("Failed to initialize GLRenderer!");
        return 1;
    }

    if (!engine.init(config, std::move(glRenderer))) {
        LOG_ERROR("Failed to initialize engine!");
        return 1;
    }

    // Create a layer
    auto layer = engine.createLayer(0);

    // Create and attach GameObjects
    auto textGridDemo = std::make_shared<TextGridDemo>();
    auto fpsDisplay = std::make_shared<FPSDisplay>();

    engine.attachGameObject(textGridDemo);
    engine.attachGameObject(fpsDisplay);

    // Info text
    LOG_INFO("\n=== AttributedTextGrid Demo ===");
    LOG_INFO("VGA text mode simulation with attributed characters");
    LOG_INFO("Features:");
    LOG_INFO("  - 100x37 character grid (60 visible, 40 off-screen)");
    LOG_INFO("  - 16x16 ATASCII font");
    LOG_INFO("  - 16-bit cells (8-bit glyph + 8-bit attribute)");
    LOG_INFO("  - 256 possible attributes per grid");
    LOG_INFO("  - Foreground/background color per attribute");
    LOG_INFO("  - Classic VGA 16-color palette");
    LOG_INFO("  - Animated text effects");
    LOG_INFO("  - Smooth horizontal scrolling (positioning demo)");
    LOG_INFO("\nControls:");
    LOG_INFO("  ESC - Quit\n");

    while (engine.isRunning()) {
        if (!engine.pollEvents()) {
            break;
        }

        if (engine.getInput().isKeyPressed(SDLK_ESCAPE)) {
            break;
        }

        engine.update();
        engine.render();
    }

    engine.shutdown();

    LOG_INFO("AttributedTextGrid demo shut down successfully!");
    return 0;
}
