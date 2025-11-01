#pragma once

#include "IRenderer.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

namespace Engine {

// Vertex structure for 2D rendering
struct Vertex2D {
    glm::vec2 position;
    glm::vec2 texCoord;
    glm::vec4 color;
};

// Uniform buffer object for projection matrix
struct UniformBufferObject {
    glm::mat4 projection;
};

// Push constants for per-draw data
struct PushConstants {
    glm::mat4 model;
    glm::vec4 tintColor;
};

// Vulkan-based renderer implementation using SDL for window management
class VulkanRenderer : public IRenderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override;

    // Non-copyable, non-movable (owns Vulkan resources)
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    // IRenderer interface
    bool init(const std::string& title, int width, int height) override;
    void shutdown() override;
    bool isInitialized() const override { return device_ != VK_NULL_HANDLE; }

    void clear() override;
    void present() override;

    void renderSprite(const Sprite& sprite, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) override;
    void renderTilemap(const Tilemap& tilemap, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) override;
    void renderText(const Text& text, const Vec2& position, float opacity = 1.0f) override;
    void renderPixelBuffer(const PixelBuffer& buffer, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) override;
    void renderIndexedPixelBuffer(const IndexedPixelBuffer& buffer, const Vec2& layerOffset = Vec2{0.0f, 0.0f}, float opacity = 1.0f) override;

    TexturePtr createStreamingTexture(int width, int height) override;
    void updateTexture(Texture& texture, const Color* pixels, int width, int height) override;

    void* getBackendContext() override { return &device_; }

    // Vulkan-specific accessors
    SDL_Window* getWindow() const { return window_; }
    VkInstance getInstance() const { return instance_; }
    VkDevice getDevice() const { return device_; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }

    // Viewport dimensions
    int getViewportWidth() const override { return windowWidth_; }
    int getViewportHeight() const override { return windowHeight_; }

private:
    // Initialization helpers
    bool createInstance();
    bool createSurface();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain();
    bool createRenderPass();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();

    // Helper functions
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    bool isDeviceSuitable(VkPhysicalDevice device);
    struct QueueFamilyIndices {
        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t presentFamily = UINT32_MAX;
        bool isComplete() { return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX; }
    };
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    // SDL window
    SDL_Window* window_ = nullptr;
    int windowWidth_ = 0;
    int windowHeight_ = 0;

    // Vulkan core objects
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkFormat swapchainImageFormat_;
    VkExtent2D swapchainExtent_;

    // Render pass and framebuffers
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapchainFramebuffers_;

    // Command buffers
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    uint32_t currentFrame_ = 0;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    // Frame state tracking
    bool frameInProgress_ = false;
    uint32_t currentImageIndex_ = 0;
    VkCommandBuffer currentCommandBuffer_ = VK_NULL_HANDLE;

    // Validation layers (debug mode)
#ifdef _DEBUG
    bool enableValidationLayers_ = true;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
#else
    bool enableValidationLayers_ = false;
#endif
    const std::vector<const char*> validationLayers_ = {
        "VK_LAYER_KHRONOS_validation"
    };

    // 2D Rendering Pipeline
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;

    // Uniform buffers (one per frame in flight)
    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VkDeviceMemory> uniformBuffersMemory_;
    std::vector<void*> uniformBuffersMapped_;

    // Vertex and index buffers for quad rendering
    VkBuffer quadVertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory quadVertexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer quadIndexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory quadIndexBufferMemory_ = VK_NULL_HANDLE;

    // Texture cache (maps Texture* to VkImage/VkImageView/VkDescriptorSet)
    struct VulkanTexture {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        int width = 0;
        int height = 0;
    };
    std::unordered_map<void*, VulkanTexture> textureCache_;

    // Helper methods for 2D rendering
    bool createGraphicsPipeline();
    bool createDescriptorSetLayout();
    bool createDescriptorPool();
    bool createUniformBuffers();
    bool createQuadBuffers();

    void updateUniformBuffer(uint32_t currentImage);

    // Frame management
    bool beginFrame();
    void endFrame();

    // Shader loading
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readShaderFile(const std::string& filename);

    // Buffer and image helpers
    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer& buffer,
                     VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    bool createImage(uint32_t width, uint32_t height, VkFormat format,
                    VkImageTiling tiling, VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkImage& image,
                    VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format);
    void transitionImageLayout(VkImage image, VkFormat format,
                              VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    // Texture management
    VulkanTexture* getOrCreateVulkanTexture(Texture* texture);
    bool createTextureFromPixels(const Color* pixels, int width, int height, VulkanTexture& vkTexture);
    void destroyVulkanTexture(VulkanTexture& vkTexture);
};

} // namespace Engine
