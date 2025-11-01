#include "engine/GLRenderer.h"
#include "engine/Sprite.h"
#include "engine/Tilemap.h"
#include "engine/Text.h"
#include "engine/PixelBuffer.h"
#include "engine/IndexedPixelBuffer.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <cmath>

// OpenGL headers
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <GL/glew.h>
#endif

namespace Engine {

// Vertex shader for textured quads
static const char* textureVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 model;

void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader for textured quads
static const char* textureFragmentShader = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D texture1;

void main() {
    FragColor = texture(texture1, TexCoord);
}
)";

// Fragment shader for indexed color with palette lookup
static const char* paletteFragmentShader = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D indexTexture;  // R8 texture with palette indices
uniform sampler2D paletteTexture;  // 256x1 RGBA texture with palette
uniform float opacity;  // Layer opacity (0.0 = fully transparent, 1.0 = fully opaque)

void main() {
    // Sample the index from the indexed texture
    // The half-texel offset from the vertex shader ensures we sample texel centers
    float index = texture(indexTexture, TexCoord).r;

    // Convert normalized index back to 0-255 range and sample at texel center
    // For pixel-perfect palette lookup, we need to hit the exact center of each palette entry
    float paletteIndex = index * 255.0;
    vec2 palCoord = vec2((paletteIndex + 0.5) / 256.0, 0.5);
    vec4 color = texture(paletteTexture, palCoord);

    // Apply layer opacity to the alpha channel
    FragColor = vec4(color.rgb, color.a * opacity);
}
)";

GLRenderer::~GLRenderer() {
    shutdown();
}

bool GLRenderer::init(const std::string& title, int width, int height) {
    windowWidth_ = width;
    windowHeight_ = height;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Create window
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (!window_) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // Create OpenGL context
    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window_);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // Enable VSync
    SDL_GL_SetSwapInterval(1);

    // Initialize OpenGL
    if (!initOpenGL()) {
        return false;
    }

    // Compile shaders
    if (!compileShaders()) {
        return false;
    }

    return true;
}

bool GLRenderer::initOpenGL() {
#ifndef __APPLE__
    // Initialize GLEW (not needed on macOS)
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
#endif

    // Get actual drawable size (important for Retina displays where it differs from window size)
    int drawableWidth, drawableHeight;
    SDL_GL_GetDrawableSize(window_, &drawableWidth, &drawableHeight);

    // Set viewport to actual drawable size for Retina support
    glViewport(0, 0, drawableWidth, drawableHeight);

    // Keep windowWidth_/windowHeight_ as logical coordinates for projection matrix
    // This maintains coordinate system consistency

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create quad VAO/VBO
    float quadVertices[] = {
        // pos      // tex
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindVertexArray(quadVAO_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

bool GLRenderer::compileShaders() {
    auto compileShader = [](const char* source, GLenum type) -> unsigned int {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
            return 0;
        }
        return shader;
    };

    auto linkProgram = [](unsigned int vertex, unsigned int fragment) -> unsigned int {
        unsigned int program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);

        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
            return 0;
        }
        return program;
    };

    // Compile texture shader
    unsigned int texVert = compileShader(textureVertexShader, GL_VERTEX_SHADER);
    unsigned int texFrag = compileShader(textureFragmentShader, GL_FRAGMENT_SHADER);
    if (!texVert || !texFrag) return false;
    textureShaderProgram_ = linkProgram(texVert, texFrag);
    glDeleteShader(texVert);
    glDeleteShader(texFrag);
    if (!textureShaderProgram_) return false;

    // Compile palette shader
    unsigned int palVert = compileShader(textureVertexShader, GL_VERTEX_SHADER);
    unsigned int palFrag = compileShader(paletteFragmentShader, GL_FRAGMENT_SHADER);
    if (!palVert || !palFrag) return false;
    paletteShaderProgram_ = linkProgram(palVert, palFrag);
    glDeleteShader(palVert);
    glDeleteShader(palFrag);
    if (!paletteShaderProgram_) return false;

    return true;
}

void GLRenderer::shutdown() {
    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
    if (quadVBO_) {
        glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }
    if (textureShaderProgram_) {
        glDeleteProgram(textureShaderProgram_);
        textureShaderProgram_ = 0;
    }
    if (paletteShaderProgram_) {
        glDeleteProgram(paletteShaderProgram_);
        paletteShaderProgram_ = 0;
    }
    if (glContext_) {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void GLRenderer::clear() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderer::present() {
    SDL_GL_SwapWindow(window_);
}

// Stub implementations for now - will implement these next
void GLRenderer::renderSprite(const Sprite& sprite, const Vec2& layerOffset, float opacity) {
    // TODO: Implement OpenGL sprite rendering
}

void GLRenderer::renderTilemap(const Tilemap& tilemap, const Vec2& layerOffset, float opacity) {
    // TODO: Implement OpenGL tilemap rendering
}

void GLRenderer::renderText(const Text& text, const Vec2& position, float opacity) {
    // TODO: Implement OpenGL text rendering
}

void GLRenderer::renderPixelBuffer(const PixelBuffer& buffer, const Vec2& layerOffset, float opacity) {
    // TODO: Implement OpenGL pixel buffer rendering
}

void GLRenderer::renderIndexedPixelBuffer(const IndexedPixelBuffer& buffer, const Vec2& layerOffset, float opacity) {
    if (!buffer.isVisible()) {
        return;
    }

    // Get mutable reference for texture management
    auto& mutableBuffer = const_cast<IndexedPixelBuffer&>(buffer);

    // Create GL textures if they don't exist
    if (mutableBuffer.getGLIndexTexture() == 0) {
        unsigned int indexTex = createIndexedTexture(buffer.getWidth(), buffer.getHeight());
        mutableBuffer.setGLIndexTexture(indexTex);
    }
    if (mutableBuffer.getGLPaletteTexture() == 0) {
        unsigned int paletteTex = createPaletteTexture();
        mutableBuffer.setGLPaletteTexture(paletteTex);
    }

    // Upload pixel data if dirty
    if (mutableBuffer.arePixelsDirty()) {
        updateIndexedTexture(
            mutableBuffer.getGLIndexTexture(),
            buffer.getPixelData(),
            buffer.getWidth(),
            buffer.getHeight()
        );
        mutableBuffer.markPixelsClean();
    }

    // Upload palette data if dirty (this is the KEY optimization!)
    if (mutableBuffer.isPaletteDirty()) {
        updatePaletteTexture(
            mutableBuffer.getGLPaletteTexture(),
            buffer.getPaletteData()
        );
        mutableBuffer.markPaletteClean();
    }

    // Mark buffer as clean
    mutableBuffer.markClean();

    // Render using the palette shader with opacity
    renderIndexedQuad(
        mutableBuffer.getGLIndexTexture(),
        mutableBuffer.getGLPaletteTexture(),
        buffer.getPosition() + layerOffset,
        Vec2{buffer.getWidth() * buffer.getScale(), buffer.getHeight() * buffer.getScale()},
        opacity
    );
}

TexturePtr GLRenderer::createStreamingTexture(int width, int height) {
    // TODO: Implement OpenGL streaming texture creation
    // When implementing, use setHandle with a deleter like:
    // texture->setHandle(reinterpret_cast<void*>(glTexId), [](void* handle) {
    //     GLuint id = reinterpret_cast<GLuint>(handle);
    //     glDeleteTextures(1, &id);
    // });
    return nullptr;
}

void GLRenderer::updateTexture(Texture& texture, const Color* pixels, int width, int height) {
    // TODO: Implement OpenGL texture update
}

unsigned int GLRenderer::createIndexedTexture(int width, int height) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Create R8 texture for indices (single channel, 0-255)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

unsigned int GLRenderer::createPaletteTexture() {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Create 256x1 RGBA texture for palette
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void GLRenderer::updateIndexedTexture(unsigned int textureId, const uint8_t* indices, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, indices);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::updatePaletteTexture(unsigned int textureId, const Color* palette) {
    // Convert Color array to RGBA bytes
    uint8_t paletteData[256 * 4];
    for (int i = 0; i < 256; ++i) {
        paletteData[i * 4 + 0] = palette[i].r;
        paletteData[i * 4 + 1] = palette[i].g;
        paletteData[i * 4 + 2] = palette[i].b;
        paletteData[i * 4 + 3] = palette[i].a;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, paletteData);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::renderIndexedQuad(unsigned int indexTexture, unsigned int paletteTexture,
                                   const Vec2& position, const Vec2& size, float opacity) {
    // Use the palette shader program
    glUseProgram(paletteShaderProgram_);

    // Set up orthographic projection matrix
    float left = 0.0f;
    float right = static_cast<float>(windowWidth_);
    float bottom = static_cast<float>(windowHeight_);
    float top = 0.0f;

    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f
    };

    // Set up model matrix (translation and scale)
    float model[16] = {
        size.x, 0.0f, 0.0f, 0.0f,
        0.0f, size.y, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        position.x, position.y, 0.0f, 1.0f
    };

    // Set uniforms
    int projLoc = glGetUniformLocation(paletteShaderProgram_, "projection");
    int modelLoc = glGetUniformLocation(paletteShaderProgram_, "model");
    int opacityLoc = glGetUniformLocation(paletteShaderProgram_, "opacity");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
    glUniform1f(opacityLoc, opacity);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, indexTexture);
    glUniform1i(glGetUniformLocation(paletteShaderProgram_, "indexTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, paletteTexture);
    glUniform1i(glGetUniformLocation(paletteShaderProgram_, "paletteTexture"), 1);

    // Draw quad
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace Engine
