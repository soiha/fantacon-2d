#pragma once

#include "Texture.h"
#include "Font.h"
#include "Tilemap.h"
#include "Palette.h"
#include "PixelFont.h"
#include "Mesh3D.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace Engine {

class IRenderer;

// Centralized resource management with caching
// Philosophy: Data (Texture, Font) is separate from usage (Sprite, Text)
class ResourceManager {
public:
    explicit ResourceManager(const std::string& basePath = "");
    ~ResourceManager() = default;

    // Initialize with renderer (needed for texture loading)
    void init(IRenderer* renderer);
    void shutdown();

    // Base path management
    void setBasePath(const std::string& path) { basePath_ = path; }
    const std::string& getBasePath() const { return basePath_; }

    // Texture management
    TexturePtr loadTexture(const std::string& name, const std::string& relativePath);
    TexturePtr getTexture(const std::string& name);
    bool hasTexture(const std::string& name) const;
    void unloadTexture(const std::string& name);

    // Font management
    FontPtr loadFont(const std::string& name, const std::string& relativePath, int size);
    FontPtr getFont(const std::string& name);
    bool hasFont(const std::string& name) const;
    void unloadFont(const std::string& name);

    // Tilemap management
    std::shared_ptr<Tilemap> getTilemap(const std::string& name);
    bool hasTilemap(const std::string& name) const;
    void unloadTilemap(const std::string& name);

    // Palette management
    PalettePtr loadPalette(const std::string& name, const std::string& relativePath);
    PalettePtr getPalette(const std::string& name);
    bool hasPalette(const std::string& name) const;
    void unloadPalette(const std::string& name);

    // Create palettes from presets
    PalettePtr createGrayscalePalette(const std::string& name);
    PalettePtr createVGAPalette(const std::string& name);
    PalettePtr createFirePalette(const std::string& name);
    PalettePtr createRainbowPalette(const std::string& name);

    // PixelFont management
    PixelFontPtr loadPixelFont(const std::string& name,
                               const std::string& relativePath,
                               int charWidth,
                               int charHeight,
                               int charsPerRow = 16,
                               const std::string& charMap = "",
                               int firstChar = 32,
                               int charCount = 96);
    PixelFontPtr loadPixelFontBinary(const std::string& name,
                                     const std::string& relativePath,
                                     int charWidth,
                                     int charHeight,
                                     const std::string& charMap = "",
                                     int firstChar = 0);
    PixelFontPtr getPixelFont(const std::string& name);
    bool hasPixelFont(const std::string& name) const;
    void unloadPixelFont(const std::string& name);

    // Mesh3D management
    Mesh3DPtr loadMesh3D(const std::string& name, const std::string& relativePath);
    Mesh3DPtr getMesh3D(const std::string& name);
    bool hasMesh3D(const std::string& name) const;
    void unloadMesh3D(const std::string& name);

    // LDtk integration
    // Load a tilemap layer from an LDtk level
    // Parameters:
    //   name: Cached name for this tilemap
    //   ldtkPath: Path to .ldtk file (relative to basePath)
    //   levelName: Level identifier in LDtk (e.g., "Level_0")
    //   layerName: Layer identifier in LDtk (e.g., "Ground")
    //   tilesetTextureOverride: Optional override for tileset texture path
    std::shared_ptr<Tilemap> loadLDtkTilemap(
        const std::string& name,
        const std::string& ldtkPath,
        const std::string& levelName,
        const std::string& layerName,
        const std::string& tilesetTextureOverride = "");

    // Bulk operations
    void clearTextures();
    void clearFonts();
    void clearTilemaps();
    void clearPalettes();
    void clearPixelFonts();
    void clearMeshes();
    void clearAll();

    // Statistics
    size_t getTextureCount() const { return textures_.size(); }
    size_t getFontCount() const { return fonts_.size(); }
    size_t getTilemapCount() const { return tilemaps_.size(); }
    size_t getPaletteCount() const { return palettes_.size(); }
    size_t getPixelFontCount() const { return pixelFonts_.size(); }
    size_t getMeshCount() const { return meshes_.size(); }

private:
    std::string makeFullPath(const std::string& relativePath) const;

    std::string basePath_;
    IRenderer* renderer_ = nullptr;

    std::unordered_map<std::string, TexturePtr> textures_;
    std::unordered_map<std::string, FontPtr> fonts_;
    std::unordered_map<std::string, std::shared_ptr<Tilemap>> tilemaps_;
    std::unordered_map<std::string, PalettePtr> palettes_;
    std::unordered_map<std::string, PixelFontPtr> pixelFonts_;
    std::unordered_map<std::string, Mesh3DPtr> meshes_;
};

} // namespace Engine
