#pragma once

#include "IRenderer.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace Engine {

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
};

} // namespace Engine
