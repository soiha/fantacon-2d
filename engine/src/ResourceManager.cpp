#include "engine/ResourceManager.h"
#include "engine/IRenderer.h"
#include "engine/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace Engine {

ResourceManager::ResourceManager(const std::string& basePath)
    : basePath_(basePath) {
}

void ResourceManager::init(IRenderer* renderer) {
    renderer_ = renderer;
}

void ResourceManager::shutdown() {
    clearAll();
    renderer_ = nullptr;
}

std::string ResourceManager::makeFullPath(const std::string& relativePath) const {
    if (basePath_.empty()) {
        return relativePath;
    }

    // Simple path concatenation - could be improved with proper path handling
    if (basePath_.back() == '/' || relativePath.front() == '/') {
        return basePath_ + relativePath;
    } else {
        return basePath_ + "/" + relativePath;
    }
}

// Texture management

TexturePtr ResourceManager::loadTexture(const std::string& name, const std::string& relativePath) {
    // Check if already loaded
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        LOG_DEBUG_FMT("Texture '%s' already loaded, returning cached version", name.c_str());
        return it->second;
    }

    // Check renderer is initialized
    if (!renderer_) {
        LOG_ERROR("Cannot load texture: ResourceManager not initialized with renderer!");
        return nullptr;
    }

    // Create and load texture
    auto texture = std::make_shared<Texture>();
    std::string fullPath = makeFullPath(relativePath);

    if (!texture->loadFromFile(fullPath, *renderer_)) {
        LOG_ERROR_FMT("Failed to load texture '%s' from '%s'", name.c_str(), fullPath.c_str());
        return nullptr;
    }

    // Cache and return
    textures_[name] = texture;
    LOG_INFO_FMT("Loaded texture '%s' from '%s'", name.c_str(), fullPath.c_str());
    return texture;
}

TexturePtr ResourceManager::getTexture(const std::string& name) {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ResourceManager::hasTexture(const std::string& name) const {
    return textures_.find(name) != textures_.end();
}

void ResourceManager::unloadTexture(const std::string& name) {
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        LOG_DEBUG_FMT("Unloading texture '%s'", name.c_str());
        textures_.erase(it);
    }
}

// Font management

FontPtr ResourceManager::loadFont(const std::string& name, const std::string& relativePath, int size) {
    // Check if already loaded
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        LOG_DEBUG_FMT("Font '%s' already loaded, returning cached version", name.c_str());
        return it->second;
    }

    // Create and load font
    auto font = std::make_shared<Font>();
    std::string fullPath = makeFullPath(relativePath);

    if (!font->loadFromFile(fullPath, size)) {
        LOG_ERROR_FMT("Failed to load font '%s' from '%s' at size %d", name.c_str(), fullPath.c_str(), size);
        return nullptr;
    }

    // Cache and return
    fonts_[name] = font;
    LOG_INFO_FMT("Loaded font '%s' from '%s' at size %d", name.c_str(), fullPath.c_str(), size);
    return font;
}

FontPtr ResourceManager::getFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ResourceManager::hasFont(const std::string& name) const {
    return fonts_.find(name) != fonts_.end();
}

void ResourceManager::unloadFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        LOG_DEBUG_FMT("Unloading font '%s'", name.c_str());
        fonts_.erase(it);
    }
}

// LDtk integration

std::shared_ptr<Tilemap> ResourceManager::loadLDtkTilemap(
    const std::string& name,
    const std::string& ldtkPath,
    const std::string& levelName,
    const std::string& layerName,
    const std::string& tilesetTextureOverride) {

    // Check if already loaded
    auto it = tilemaps_.find(name);
    if (it != tilemaps_.end()) {
        LOG_DEBUG_FMT("Tilemap '%s' already loaded, returning cached version", name.c_str());
        return it->second;
    }

    // Load and parse JSON file
    std::string fullPath = makeFullPath(ldtkPath);
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        LOG_ERROR_FMT("Failed to open LDtk file: '%s'", fullPath.c_str());
        return nullptr;
    }

    json ldtkData;
    try {
        file >> ldtkData;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("Failed to parse LDtk JSON from '%s': %s", fullPath.c_str(), e.what());
        return nullptr;
    }

    // Find the level
    if (!ldtkData.contains("levels")) {
        LOG_ERROR_FMT("LDtk file '%s' has no levels array", fullPath.c_str());
        return nullptr;
    }

    json levelData;
    bool foundLevel = false;
    for (const auto& level : ldtkData["levels"]) {
        if (level.value("identifier", "") == levelName) {
            levelData = level;
            foundLevel = true;
            break;
        }
    }

    if (!foundLevel) {
        LOG_ERROR_FMT("Level '%s' not found in LDtk file '%s'", levelName.c_str(), fullPath.c_str());
        return nullptr;
    }

    // Find the layer
    if (!levelData.contains("layerInstances")) {
        LOG_ERROR_FMT("Level '%s' has no layerInstances", levelName.c_str());
        return nullptr;
    }

    json layerData;
    bool foundLayer = false;
    for (const auto& layer : levelData["layerInstances"]) {
        if (layer.value("__identifier", "") == layerName) {
            layerData = layer;
            foundLayer = true;
            break;
        }
    }

    if (!foundLayer) {
        LOG_ERROR_FMT("Layer '%s' not found in level '%s'", layerName.c_str(), levelName.c_str());
        return nullptr;
    }

    // Check layer type
    std::string layerType = layerData.value("__type", "");
    if (layerType != "Tiles" && layerType != "AutoLayer") {
        LOG_ERROR_FMT("Layer '%s' is type '%s', only 'Tiles' and 'AutoLayer' are supported",
                      layerName.c_str(), layerType.c_str());
        return nullptr;
    }

    // Get layer dimensions
    int layerWidth = layerData.value("__cWid", 0);
    int layerHeight = layerData.value("__cHei", 0);
    int gridSize = layerData.value("__gridSize", 16);

    if (layerWidth == 0 || layerHeight == 0) {
        LOG_ERROR_FMT("Layer '%s' has invalid dimensions: %dx%d",
                      layerName.c_str(), layerWidth, layerHeight);
        return nullptr;
    }

    // Create tilemap
    auto tilemap = std::make_shared<Tilemap>(layerWidth, layerHeight, gridSize, gridSize);

    // Get tileset info
    std::string tilesetPath;
    int tilesetUid = layerData.value("__tilesetDefUid", -1);

    if (!tilesetTextureOverride.empty()) {
        tilesetPath = tilesetTextureOverride;
    } else {
        // Find tileset definition
        if (tilesetUid < 0 || !ldtkData.contains("defs") || !ldtkData["defs"].contains("tilesets")) {
            LOG_ERROR_FMT("Cannot find tileset definition for layer '%s'", layerName.c_str());
            return nullptr;
        }

        for (const auto& tileset : ldtkData["defs"]["tilesets"]) {
            if (tileset.value("uid", -1) == tilesetUid) {
                tilesetPath = tileset.value("relPath", "");
                break;
            }
        }

        if (tilesetPath.empty()) {
            LOG_ERROR_FMT("Tileset path not found for layer '%s'", layerName.c_str());
            return nullptr;
        }

        // LDtk tileset paths are relative to the .ldtk file location
        // Extract directory from ldtkPath
        size_t lastSlash = ldtkPath.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            std::string ldtkDir = ldtkPath.substr(0, lastSlash + 1);
            tilesetPath = ldtkDir + tilesetPath;
        }
    }

    // Load tileset texture (create name from tilemap name + "_tileset")
    std::string tilesetName = name + "_tileset";
    auto tilesetTexture = loadTexture(tilesetName, tilesetPath);
    if (!tilesetTexture) {
        LOG_ERROR_FMT("Failed to load tileset texture from '%s'", tilesetPath.c_str());
        return nullptr;
    }

    // Calculate tiles per row in tileset
    int tilesPerRow = tilesetTexture->getWidth() / gridSize;
    tilemap->setTileset(tilesetTexture, tilesPerRow);

    // Fill tile data
    // LDtk uses "gridTiles" for Tiles layers and "autoLayerTiles" for AutoLayers
    const char* tilesKey = (layerType == "Tiles") ? "gridTiles" : "autoLayerTiles";

    if (layerData.contains(tilesKey)) {
        for (const auto& tile : layerData[tilesKey]) {
            // Get tile position in grid
            auto px = tile.value("px", std::vector<int>{0, 0});
            if (px.size() >= 2) {
                int tileX = px[0] / gridSize;
                int tileY = px[1] / gridSize;

                // Get tile ID from source position
                auto src = tile.value("src", std::vector<int>{0, 0});
                if (src.size() >= 2) {
                    int srcX = src[0] / gridSize;
                    int srcY = src[1] / gridSize;
                    int tileId = srcY * tilesPerRow + srcX;

                    tilemap->setTile(tileX, tileY, tileId);
                }
            }
        }
    }

    // Cache and return
    tilemaps_[name] = tilemap;
    LOG_INFO_FMT("Loaded LDtk tilemap '%s' from '%s' (level: %s, layer: %s) - %dx%d tiles",
                 name.c_str(), fullPath.c_str(), levelName.c_str(), layerName.c_str(),
                 layerWidth, layerHeight);
    return tilemap;
}

// Tilemap management

std::shared_ptr<Tilemap> ResourceManager::getTilemap(const std::string& name) {
    auto it = tilemaps_.find(name);
    if (it != tilemaps_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ResourceManager::hasTilemap(const std::string& name) const {
    return tilemaps_.find(name) != tilemaps_.end();
}

void ResourceManager::unloadTilemap(const std::string& name) {
    auto it = tilemaps_.find(name);
    if (it != tilemaps_.end()) {
        LOG_DEBUG_FMT("Unloading tilemap '%s'", name.c_str());
        tilemaps_.erase(it);
    }
}

// Palette management

PalettePtr ResourceManager::loadPalette(const std::string& name, const std::string& relativePath) {
    // Check if already loaded
    auto it = palettes_.find(name);
    if (it != palettes_.end()) {
        LOG_DEBUG_FMT("Palette '%s' already loaded, returning cached version", name.c_str());
        return it->second;
    }

    // Create new palette and load
    auto palette = std::make_shared<Palette>();
    std::string fullPath = makeFullPath(relativePath);

    if (!palette->loadFromFile(fullPath)) {
        LOG_ERROR_FMT("Failed to load palette '%s' from '%s'", name.c_str(), fullPath.c_str());
        return nullptr;
    }

    // Cache and return
    palettes_[name] = palette;
    LOG_INFO_FMT("Loaded palette '%s' from '%s'", name.c_str(), fullPath.c_str());
    return palette;
}

PalettePtr ResourceManager::getPalette(const std::string& name) {
    auto it = palettes_.find(name);
    if (it != palettes_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ResourceManager::hasPalette(const std::string& name) const {
    return palettes_.find(name) != palettes_.end();
}

void ResourceManager::unloadPalette(const std::string& name) {
    auto it = palettes_.find(name);
    if (it != palettes_.end()) {
        LOG_DEBUG_FMT("Unloading palette '%s'", name.c_str());
        palettes_.erase(it);
    }
}

PalettePtr ResourceManager::createGrayscalePalette(const std::string& name) {
    auto palette = std::make_shared<Palette>(Palette::createGrayscale());
    palettes_[name] = palette;
    LOG_INFO_FMT("Created grayscale palette '%s'", name.c_str());
    return palette;
}

PalettePtr ResourceManager::createVGAPalette(const std::string& name) {
    auto palette = std::make_shared<Palette>(Palette::createVGA());
    palettes_[name] = palette;
    LOG_INFO_FMT("Created VGA palette '%s'", name.c_str());
    return palette;
}

PalettePtr ResourceManager::createFirePalette(const std::string& name) {
    auto palette = std::make_shared<Palette>(Palette::createFireGradient());
    palettes_[name] = palette;
    LOG_INFO_FMT("Created fire palette '%s'", name.c_str());
    return palette;
}

PalettePtr ResourceManager::createRainbowPalette(const std::string& name) {
    auto palette = std::make_shared<Palette>(Palette::createRainbow());
    palettes_[name] = palette;
    LOG_INFO_FMT("Created rainbow palette '%s'", name.c_str());
    return palette;
}

// PixelFont management

PixelFontPtr ResourceManager::loadPixelFont(const std::string& name,
                                             const std::string& relativePath,
                                             int charWidth,
                                             int charHeight,
                                             int charsPerRow,
                                             const std::string& charMap,
                                             int firstChar,
                                             int charCount) {
    // Check if already loaded
    auto it = pixelFonts_.find(name);
    if (it != pixelFonts_.end()) {
        LOG_DEBUG_FMT("PixelFont '%s' already loaded, returning cached version", name.c_str());
        return it->second;
    }

    // Create and load pixel font
    auto pixelFont = std::make_shared<PixelFont>();
    std::string fullPath = makeFullPath(relativePath);

    if (!pixelFont->loadFromFile(fullPath, charWidth, charHeight, charsPerRow, charMap, firstChar, charCount)) {
        LOG_ERROR_FMT("Failed to load pixel font '%s' from '%s'", name.c_str(), fullPath.c_str());
        return nullptr;
    }

    // Cache and return
    pixelFonts_[name] = pixelFont;
    LOG_INFO_FMT("Loaded pixel font '%s' from '%s' (%dx%d chars)",
                 name.c_str(), fullPath.c_str(), charWidth, charHeight);
    return pixelFont;
}

PixelFontPtr ResourceManager::loadPixelFontBinary(const std::string& name,
                                                   const std::string& relativePath,
                                                   int charWidth,
                                                   int charHeight,
                                                   const std::string& charMap,
                                                   int firstChar) {
    // Check if already loaded
    auto it = pixelFonts_.find(name);
    if (it != pixelFonts_.end()) {
        LOG_DEBUG_FMT("PixelFont '%s' already loaded, returning cached version", name.c_str());
        return it->second;
    }

    // Create and load pixel font from binary
    auto pixelFont = std::make_shared<PixelFont>();
    std::string fullPath = makeFullPath(relativePath);

    if (!pixelFont->loadFromBinary(fullPath, charWidth, charHeight, charMap, firstChar)) {
        LOG_ERROR_FMT("Failed to load binary pixel font '%s' from '%s'", name.c_str(), fullPath.c_str());
        return nullptr;
    }

    // Cache and return
    pixelFonts_[name] = pixelFont;
    LOG_INFO_FMT("Loaded binary pixel font '%s' from '%s' (%dx%d chars)",
                 name.c_str(), fullPath.c_str(), charWidth, charHeight);
    return pixelFont;
}

PixelFontPtr ResourceManager::getPixelFont(const std::string& name) {
    auto it = pixelFonts_.find(name);
    if (it != pixelFonts_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ResourceManager::hasPixelFont(const std::string& name) const {
    return pixelFonts_.find(name) != pixelFonts_.end();
}

void ResourceManager::unloadPixelFont(const std::string& name) {
    auto it = pixelFonts_.find(name);
    if (it != pixelFonts_.end()) {
        LOG_DEBUG_FMT("Unloading pixel font '%s'", name.c_str());
        pixelFonts_.erase(it);
    }
}

// Bulk operations

void ResourceManager::clearTextures() {
    LOG_DEBUG_FMT("Clearing %zu textures", textures_.size());
    textures_.clear();
}

void ResourceManager::clearFonts() {
    LOG_DEBUG_FMT("Clearing %zu fonts", fonts_.size());
    fonts_.clear();
}

void ResourceManager::clearTilemaps() {
    LOG_DEBUG_FMT("Clearing %zu tilemaps", tilemaps_.size());
    tilemaps_.clear();
}

void ResourceManager::clearPalettes() {
    LOG_DEBUG_FMT("Clearing %zu palettes", palettes_.size());
    palettes_.clear();
}

void ResourceManager::clearPixelFonts() {
    LOG_DEBUG_FMT("Clearing %zu pixel fonts", pixelFonts_.size());
    pixelFonts_.clear();
}

void ResourceManager::clearAll() {
    clearTextures();
    clearFonts();
    clearTilemaps();
    clearPalettes();
    clearPixelFonts();
}

} // namespace Engine
