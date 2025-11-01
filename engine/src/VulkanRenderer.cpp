#include "engine/VulkanRenderer.h"
#include "engine/Sprite.h"
#include "engine/Tilemap.h"
#include "engine/Text.h"
#include "engine/PixelBuffer.h"
#include "engine/IndexedPixelBuffer.h"
#include "engine/Logger.h"
#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

VulkanRenderer::~VulkanRenderer() {
    shutdown();
}

bool VulkanRenderer::init(const std::string& title, int width, int height) {
    windowWidth_ = width;
    windowHeight_ = height;

    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR_STREAM("Failed to initialize SDL: " << SDL_GetError());
        return false;
    }

    // Create SDL window with Vulkan support
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
    );

    if (!window_) {
        LOG_ERROR_STREAM("Failed to create window: " << SDL_GetError());
        return false;
    }

    // Create Vulkan instance
    if (!createInstance()) {
        LOG_ERROR("Failed to create Vulkan instance");
        return false;
    }

    // Create surface
    if (!createSurface()) {
        LOG_ERROR("Failed to create Vulkan surface");
        return false;
    }

    // Pick physical device
    if (!pickPhysicalDevice()) {
        LOG_ERROR("Failed to find suitable GPU");
        return false;
    }

    // Create logical device and queues
    if (!createLogicalDevice()) {
        LOG_ERROR("Failed to create logical device");
        return false;
    }

    // Create swapchain
    if (!createSwapchain()) {
        LOG_ERROR("Failed to create swapchain");
        return false;
    }

    // Create render pass
    if (!createRenderPass()) {
        LOG_ERROR("Failed to create render pass");
        return false;
    }

    // Create framebuffers
    if (!createFramebuffers()) {
        LOG_ERROR("Failed to create framebuffers");
        return false;
    }

    // Create command pool and buffers
    if (!createCommandPool()) {
        LOG_ERROR("Failed to create command pool");
        return false;
    }

    if (!createCommandBuffers()) {
        LOG_ERROR("Failed to create command buffers");
        return false;
    }

    // Create synchronization objects
    if (!createSyncObjects()) {
        LOG_ERROR("Failed to create sync objects");
        return false;
    }

    // Create 2D rendering pipeline
    if (!createDescriptorSetLayout()) {
        LOG_ERROR("Failed to create descriptor set layout");
        return false;
    }

    if (!createGraphicsPipeline()) {
        LOG_ERROR("Failed to create graphics pipeline");
        return false;
    }

    if (!createDescriptorPool()) {
        LOG_ERROR("Failed to create descriptor pool");
        return false;
    }

    if (!createUniformBuffers()) {
        LOG_ERROR("Failed to create uniform buffers");
        return false;
    }

    if (!createQuadBuffers()) {
        LOG_ERROR("Failed to create quad buffers");
        return false;
    }

    LOG_INFO("Vulkan renderer initialized successfully");
    return true;
}

void VulkanRenderer::shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);

        // Destroy texture cache
        for (auto& pair : textureCache_) {
            destroyVulkanTexture(pair.second);
        }
        textureCache_.clear();

        // Destroy quad buffers
        if (quadIndexBuffer_ != VK_NULL_HANDLE)
            vkDestroyBuffer(device_, quadIndexBuffer_, nullptr);
        if (quadIndexBufferMemory_ != VK_NULL_HANDLE)
            vkFreeMemory(device_, quadIndexBufferMemory_, nullptr);
        if (quadVertexBuffer_ != VK_NULL_HANDLE)
            vkDestroyBuffer(device_, quadVertexBuffer_, nullptr);
        if (quadVertexBufferMemory_ != VK_NULL_HANDLE)
            vkFreeMemory(device_, quadVertexBufferMemory_, nullptr);

        // Destroy uniform buffers
        for (size_t i = 0; i < uniformBuffers_.size(); i++) {
            if (uniformBuffers_[i] != VK_NULL_HANDLE)
                vkDestroyBuffer(device_, uniformBuffers_[i], nullptr);
            if (uniformBuffersMemory_[i] != VK_NULL_HANDLE)
                vkFreeMemory(device_, uniformBuffersMemory_[i], nullptr);
        }

        // Destroy descriptor pool
        if (descriptorPool_ != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

        // Destroy graphics pipeline
        if (graphicsPipeline_ != VK_NULL_HANDLE)
            vkDestroyPipeline(device_, graphicsPipeline_, nullptr);

        // Destroy pipeline layout
        if (pipelineLayout_ != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);

        // Destroy descriptor set layouts
        if (textureDescriptorSetLayout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(device_, textureDescriptorSetLayout_, nullptr);
        if (uniformDescriptorSetLayout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(device_, uniformDescriptorSetLayout_, nullptr);

        // Destroy sync objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (renderFinishedSemaphores_[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
            if (imageAvailableSemaphores_[i] != VK_NULL_HANDLE)
                vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
            if (inFlightFences_[i] != VK_NULL_HANDLE)
                vkDestroyFence(device_, inFlightFences_[i], nullptr);
        }

        // Destroy command pool
        if (commandPool_ != VK_NULL_HANDLE)
            vkDestroyCommandPool(device_, commandPool_, nullptr);

        // Destroy framebuffers
        for (auto framebuffer : swapchainFramebuffers_) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }

        // Destroy render pass
        if (renderPass_ != VK_NULL_HANDLE)
            vkDestroyRenderPass(device_, renderPass_, nullptr);

        // Destroy swapchain image views
        for (auto imageView : swapchainImageViews_) {
            vkDestroyImageView(device_, imageView, nullptr);
        }

        // Destroy swapchain
        if (swapchain_ != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(device_, swapchain_, nullptr);

        // Destroy logical device
        vkDestroyDevice(device_, nullptr);
    }

    // Destroy surface
    if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }

    // Destroy instance
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }

    // Destroy SDL window
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

void VulkanRenderer::clear() {
    // Clear will be handled in the render pass
}

bool VulkanRenderer::beginFrame() {
    // Already in a frame, don't begin again
    if (frameInProgress_) {
        return true;
    }

    // Wait for previous frame
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    // Acquire next image
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
        imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &currentImageIndex_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swapchain needs recreation (window resized, etc.)
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("Failed to acquire swapchain image");
        return false;
    }

    // Reset fence
    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    // Get command buffer for this frame
    currentCommandBuffer_ = commandBuffers_[currentFrame_];
    vkResetCommandBuffer(currentCommandBuffer_, 0);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(currentCommandBuffer_, &beginInfo) != VK_SUCCESS) {
        LOG_ERROR("Failed to begin recording command buffer");
        return false;
    }

    // Update uniform buffer with projection matrix
    updateUniformBuffer(currentFrame_);

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = swapchainFramebuffers_[currentImageIndex_];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent_;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(currentCommandBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind graphics pipeline
    vkCmdBindPipeline(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

    // Bind vertex and index buffers
    VkBuffer vertexBuffers[] = {quadVertexBuffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(currentCommandBuffer_, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(currentCommandBuffer_, quadIndexBuffer_, 0, VK_INDEX_TYPE_UINT16);

    // Bind uniform descriptor set (set 0) for this frame
    vkCmdBindDescriptorSets(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipelineLayout_, 0, 1, &uniformDescriptorSets_[currentFrame_],
                           0, nullptr);

    frameInProgress_ = true;
    return true;
}

void VulkanRenderer::endFrame() {
    if (!frameInProgress_) {
        return;
    }

    // End render pass
    vkCmdEndRenderPass(currentCommandBuffer_);

    // End command buffer
    if (vkEndCommandBuffer(currentCommandBuffer_) != VK_SUCCESS) {
        LOG_ERROR("Failed to record command buffer");
        frameInProgress_ = false;
        return;
    }

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentCommandBuffer_;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
        LOG_ERROR("Failed to submit draw command buffer");
        frameInProgress_ = false;
        return;
    }

    frameInProgress_ = false;
    currentCommandBuffer_ = VK_NULL_HANDLE;
}

void VulkanRenderer::present() {
    // If no frame was started, there's nothing to present
    // This can happen when clear() is called but no renderables draw anything
    bool hadFrame = frameInProgress_;

    // End the frame (submit command buffer) if one is in progress
    endFrame();

    // Only present if we actually had a frame to render
    if (!hadFrame) {
        return;
    }

    // Present the image
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &currentImageIndex_;

    VkResult result = vkQueuePresentKHR(presentQueue_, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain needs recreation
    } else if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to present swapchain image");
    }

    // Move to next frame
    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Rendering implementations
void VulkanRenderer::renderSprite(const Sprite& sprite, const Vec2& layerOffset, float opacity) {
    // Skip invisible sprites
    if (!sprite.isVisible()) {
        return;
    }

    // Get texture
    auto texture = sprite.getTexture();
    if (!texture || !texture->isValid()) {
        // TODO: Create and use a default white 1x1 texture for textureless sprites
        return;
    }

    // Begin frame if not already started
    if (!beginFrame()) {
        return;
    }

    // Get or create Vulkan texture
    VulkanTexture* vkTexture = getOrCreateVulkanTexture(texture.get());
    if (!vkTexture) {
        return;  // Texture not available yet
    }

    // Calculate sprite dimensions
    float width = static_cast<float>(vkTexture->width);
    float height = static_cast<float>(vkTexture->height);

    // Handle source rectangle (sprite sheets)
    if (sprite.hasSourceRect()) {
        const auto& srcRect = sprite.getSourceRect();
        width = srcRect.w;
        height = srcRect.h;
        // TODO: Adjust texture coordinates for source rectangle
    }

    // Build model matrix: translate -> rotate -> scale
    // Note: We scale by texture dimensions to convert unit quad to actual size
    glm::mat4 model = glm::mat4(1.0f);

    // Translate to position (including layer offset)
    Vec2 finalPos = sprite.getPosition() + layerOffset;
    model = glm::translate(model, glm::vec3(finalPos.x, finalPos.y, 0.0f));

    // Rotate around center
    if (sprite.getRotation() != 0.0f) {
        model = glm::translate(model, glm::vec3(width * 0.5f, height * 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(sprite.getRotation()), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-width * 0.5f, -height * 0.5f, 0.0f));
    }

    // Scale by sprite scale and texture dimensions
    Vec2 scale = sprite.getScale();
    model = glm::scale(model, glm::vec3(width * scale.x, height * scale.y, 1.0f));

    // Set push constants
    PushConstants pushConstants{};
    pushConstants.model = model;
    pushConstants.tintColor = glm::vec4(1.0f, 1.0f, 1.0f, opacity);  // White tint with opacity

    vkCmdPushConstants(currentCommandBuffer_, pipelineLayout_,
                      VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(PushConstants), &pushConstants);

    // Bind texture descriptor set (set 1)
    vkCmdBindDescriptorSets(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipelineLayout_, 1, 1, &vkTexture->descriptorSet,
                           0, nullptr);

    // Draw the quad (6 indices = 2 triangles)
    vkCmdDrawIndexed(currentCommandBuffer_, 6, 1, 0, 0, 0);
}

void VulkanRenderer::renderTilemap(const Tilemap& tilemap, const Vec2& layerOffset, float opacity) {
    if (!tilemap.isVisible() || !tilemap.getTileset() || !tilemap.getTileset()->isValid()) {
        return;
    }

    auto tileset = tilemap.getTileset();
    Vec2 pos = tilemap.getPosition() + layerOffset;
    int tileWidth = tilemap.getTileWidth();
    int tileHeight = tilemap.getTileHeight();
    int tilesPerRow = tilemap.getTilesPerRow();

    if (tilesPerRow <= 0) return;

    // Begin frame if not already started
    if (!beginFrame()) {
        return;
    }

    // Get or create Vulkan texture for tileset
    VulkanTexture* vkTexture = getOrCreateVulkanTexture(tileset.get());
    if (!vkTexture) {
        return;
    }

    // Bind texture descriptor set once for all tiles
    vkCmdBindDescriptorSets(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipelineLayout_, 1, 1, &vkTexture->descriptorSet,
                           0, nullptr);

    // Render each tile
    for (int y = 0; y < tilemap.getHeight(); ++y) {
        for (int x = 0; x < tilemap.getWidth(); ++x) {
            int tileId = tilemap.getTile(x, y);
            if (tileId < 0) continue;  // Skip empty tiles

            // Calculate source rectangle in tileset (texture coordinates)
            int srcX = (tileId % tilesPerRow) * tileWidth;
            int srcY = (tileId / tilesPerRow) * tileHeight;

            // For now, we'll use the full quad and adjust UVs via model matrix
            // TODO: Support source rectangles by modifying vertex data or using different approach

            // Build model matrix for this tile
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(pos.x + x * tileWidth, pos.y + y * tileHeight, 0.0f));
            model = glm::scale(model, glm::vec3(static_cast<float>(tileWidth), static_cast<float>(tileHeight), 1.0f));

            // Set push constants
            PushConstants pushConstants{};
            pushConstants.model = model;
            pushConstants.tintColor = glm::vec4(1.0f, 1.0f, 1.0f, opacity);

            vkCmdPushConstants(currentCommandBuffer_, pipelineLayout_,
                              VK_SHADER_STAGE_VERTEX_BIT, 0,
                              sizeof(PushConstants), &pushConstants);

            // Draw the tile
            vkCmdDrawIndexed(currentCommandBuffer_, 6, 1, 0, 0, 0);
        }
    }
}

void VulkanRenderer::renderText(const Text& text, const Vec2& position, float opacity) {
    // TODO: Implement text rendering
    // The Text class currently uses SDL_Texture directly, which is not backend-agnostic.
    // To support text rendering in Vulkan, we need to:
    // 1. Refactor Text class to use backend-agnostic Texture
    // 2. Or provide a way to convert SDL_TTF rendered surfaces to Vulkan textures
    // For now, text rendering is not supported in the Vulkan renderer.

    if (!text.isValid()) {
        return;
    }

    LOG_WARNING("Text rendering is not yet supported in Vulkan renderer");
}

void VulkanRenderer::renderPixelBuffer(const PixelBuffer& buffer, const Vec2& layerOffset, float opacity) {
    if (!buffer.isVisible() || !buffer.getTexture()) {
        return;
    }

    // Upload pixels if dirty
    const_cast<PixelBuffer&>(buffer).upload(*this);

    // Begin frame if not already started
    if (!beginFrame()) {
        return;
    }

    // Get texture
    auto texture = buffer.getTexture();
    VulkanTexture* vkTexture = getOrCreateVulkanTexture(texture.get());
    if (!vkTexture) {
        return;
    }

    // Calculate position with layer offset and scale
    Vec2 pos = buffer.getPosition() + layerOffset;
    float scale = buffer.getScale();
    float width = static_cast<float>(buffer.getWidth()) * scale;
    float height = static_cast<float>(buffer.getHeight()) * scale;

    // Build model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    // Set push constants
    PushConstants pushConstants{};
    pushConstants.model = model;
    pushConstants.tintColor = glm::vec4(1.0f, 1.0f, 1.0f, opacity);

    vkCmdPushConstants(currentCommandBuffer_, pipelineLayout_,
                      VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(PushConstants), &pushConstants);

    // Bind texture descriptor set
    vkCmdBindDescriptorSets(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipelineLayout_, 1, 1, &vkTexture->descriptorSet,
                           0, nullptr);

    // Draw the quad
    vkCmdDrawIndexed(currentCommandBuffer_, 6, 1, 0, 0, 0);
}

void VulkanRenderer::renderIndexedPixelBuffer(const IndexedPixelBuffer& buffer, const Vec2& layerOffset, float opacity) {
    if (!buffer.isVisible() || !buffer.getTexture()) {
        return;
    }

    // Upload pixels if dirty (converts indexed colors to RGBA using palette)
    const_cast<IndexedPixelBuffer&>(buffer).upload(*this);

    // Begin frame if not already started
    if (!beginFrame()) {
        return;
    }

    // Get texture
    auto texture = buffer.getTexture();
    VulkanTexture* vkTexture = getOrCreateVulkanTexture(texture.get());
    if (!vkTexture) {
        return;
    }

    // Calculate position with layer offset and scale
    Vec2 pos = buffer.getPosition() + layerOffset;
    float scale = buffer.getScale();
    float width = static_cast<float>(buffer.getWidth()) * scale;
    float height = static_cast<float>(buffer.getHeight()) * scale;

    // Build model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    // Set push constants
    PushConstants pushConstants{};
    pushConstants.model = model;
    pushConstants.tintColor = glm::vec4(1.0f, 1.0f, 1.0f, opacity);

    vkCmdPushConstants(currentCommandBuffer_, pipelineLayout_,
                      VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(PushConstants), &pushConstants);

    // Bind texture descriptor set
    vkCmdBindDescriptorSets(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           pipelineLayout_, 1, 1, &vkTexture->descriptorSet,
                           0, nullptr);

    // Draw the quad
    vkCmdDrawIndexed(currentCommandBuffer_, 6, 1, 0, 0, 0);
}

TexturePtr VulkanRenderer::createStreamingTexture(int width, int height) {
    // Create a new VulkanTexture entry in our cache
    VulkanTexture vkTexture{};
    vkTexture.width = width;
    vkTexture.height = height;

    // Allocate a unique handle for this texture (use address of VulkanTexture)
    // We'll store it in cache first, then use the cache entry's address as the handle
    void* handle = reinterpret_cast<void*>(static_cast<uintptr_t>(textureCache_.size() + 1));

    // Store in cache
    textureCache_[handle] = vkTexture;

    // Create Texture object with custom deleter for Vulkan
    auto deleter = [this, handle](void* /*backendHandle*/) {
        // Clean up VulkanTexture when Texture is destroyed
        auto it = textureCache_.find(handle);
        if (it != textureCache_.end()) {
            destroyVulkanTexture(it->second);
            textureCache_.erase(it);
        }
    };

    auto texture = std::make_shared<Texture>();
    texture->setHandle(handle, deleter);
    texture->setDimensions(width, height);
    return texture;
}

void VulkanRenderer::updateTexture(Texture& texture, const Color* pixels, int width, int height) {
    if (!texture.isValid() || !pixels) {
        return;
    }

    void* handle = texture.getHandle();
    auto it = textureCache_.find(handle);

    if (it == textureCache_.end()) {
        LOG_ERROR("Texture not found in Vulkan cache");
        return;
    }

    VulkanTexture& vkTexture = it->second;

    // If texture already exists and dimensions match, we can update it
    // Otherwise, destroy and recreate
    if (vkTexture.image != VK_NULL_HANDLE) {
        if (vkTexture.width != width || vkTexture.height != height) {
            // Dimensions changed, need to recreate
            destroyVulkanTexture(vkTexture);
            vkTexture = VulkanTexture{};  // Reset
        }
    }

    // Create or update the texture from pixels
    if (!createTextureFromPixels(pixels, width, height, vkTexture)) {
        LOG_ERROR("Failed to create/update Vulkan texture from pixels");
    }
}

// Implementation of initialization helpers
bool VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Engine2D";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Engine2D";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get required extensions
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Enable validation layers in debug mode
    if (enableValidationLayers_ && checkValidationLayerSupport()) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    return vkCreateInstance(&createInfo, nullptr, &instance_) == VK_SUCCESS;
}

bool VulkanRenderer::createSurface() {
    return SDL_Vulkan_CreateSurface(window_, instance_, &surface_) == SDL_TRUE;
}

bool VulkanRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

    if (deviceCount == 0) {
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice_ = device;
            return true;
        }
    }

    return false;
}

bool VulkanRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers_) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);

    return true;
}

bool VulkanRenderer::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, presentModes.data());

    // Choose swapchain format
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }

    // Choose present mode (prefer mailbox for low latency)
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }

    // Choose swap extent
    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = std::clamp(static_cast<uint32_t>(windowWidth_),
            capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(windowHeight_),
            capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_) != VK_SUCCESS) {
        return false;
    }

    // Get swapchain images
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapchainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());

    swapchainImageFormat_ = surfaceFormat.format;
    swapchainExtent_ = extent;

    // Create image views
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainImageFormat_;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device_, &viewInfo, nullptr, &swapchainImageViews_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    return vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) == VK_SUCCESS;
}

bool VulkanRenderer::createFramebuffers() {
    swapchainFramebuffers_.resize(swapchainImageViews_.size());

    for (size_t i = 0; i < swapchainImageViews_.size(); i++) {
        VkImageView attachments[] = {
            swapchainImageViews_[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent_.width;
        framebufferInfo.height = swapchainExtent_.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapchainFramebuffers_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice_);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    return vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) == VK_SUCCESS;
}

bool VulkanRenderer::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    return vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) == VK_SUCCESS;
}

bool VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

// Helper functions
bool VulkanRenderer::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers_) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
    uint32_t sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window_, &sdlExtensionCount, nullptr);

    std::vector<const char*> extensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(window_, &sdlExtensionCount, extensions.data());

    return extensions;
}

bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    // Check for swapchain support
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    bool swapChainAdequate = false;
    if (requiredExtensions.empty()) {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

        swapChainAdequate = formatCount != 0 && presentModeCount != 0;
    }

    return indices.isComplete() && swapChainAdequate;
}

VulkanRenderer::QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

// ============================================================================
// 2D Rendering Pipeline Creation
// ============================================================================

bool VulkanRenderer::createDescriptorSetLayout() {
    // Create Set 0: Uniform buffer binding (projection matrix)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo uniformLayoutInfo{};
    uniformLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uniformLayoutInfo.bindingCount = 1;
    uniformLayoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device_, &uniformLayoutInfo, nullptr, &uniformDescriptorSetLayout_) != VK_SUCCESS) {
        LOG_ERROR("Failed to create uniform descriptor set layout");
        return false;
    }

    // Create Set 1: Texture sampler binding
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;  // Binding 0 within set 1
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(device_, &textureLayoutInfo, nullptr, &textureDescriptorSetLayout_) != VK_SUCCESS) {
        LOG_ERROR("Failed to create texture descriptor set layout");
        return false;
    }

    return true;
}

bool VulkanRenderer::createGraphicsPipeline() {
    // Load shader modules
    auto vertShaderCode = readShaderFile("shaders/sprite_vert.spv");
    auto fragShaderCode = readShaderFile("shaders/sprite_frag.spv");

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        LOG_ERROR("Failed to load shader files");
        return false;
    }

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        return false;
    }

    // Shader stage creation
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex input
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex2D);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    // Position
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex2D, position);
    // TexCoord
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex2D, texCoord);
    // Color
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex2D, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent_.width);
    viewport.height = static_cast<float>(swapchainExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // No culling for 2D
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending (alpha blending for transparency)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Push constants
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    // Pipeline layout - use both descriptor set layouts
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {
        uniformDescriptorSetLayout_,   // Set 0: Uniform buffer
        textureDescriptorSetLayout_    // Set 1: Texture sampler
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        LOG_ERROR("Failed to create pipeline layout");
        return false;
    }

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_) != VK_SUCCESS) {
        LOG_ERROR("Failed to create graphics pipeline");
        return false;
    }

    // Clean up shader modules
    vkDestroyShaderModule(device_, fragShaderModule, nullptr);
    vkDestroyShaderModule(device_, vertShaderModule, nullptr);

    LOG_INFO("Graphics pipeline created successfully");
    return true;
}

bool VulkanRenderer::createDescriptorPool() {
    // Create a large pool to handle many textures
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1000;  // Support up to 1000 unique textures

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1000 + MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        LOG_ERROR("Failed to create descriptor pool");
        return false;
    }

    return true;
}

bool VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformDescriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (!createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers_[i], uniformBuffersMemory_[i])) {
            return false;
        }

        // Map the buffer memory so we can update it later
        vkMapMemory(device_, uniformBuffersMemory_[i], 0, bufferSize, 0, &uniformBuffersMapped_[i]);
    }

    // Create descriptor sets for uniform buffers (one per frame)
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool_;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &uniformDescriptorSetLayout_;

        if (vkAllocateDescriptorSets(device_, &allocInfo, &uniformDescriptorSets_[i]) != VK_SUCCESS) {
            LOG_ERROR("Failed to allocate uniform descriptor set");
            return false;
        }

        // Update descriptor set with buffer info
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers_[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = uniformDescriptorSets_[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
    }

    return true;
}

bool VulkanRenderer::createQuadBuffers() {
    // Define a unit quad (0,0 to 1,1) that we'll transform per sprite
    std::vector<Vertex2D> vertices = {
        {{0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},  // Top-left
        {{1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},  // Top-right
        {{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},  // Bottom-right
        {{0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}   // Bottom-left
    };

    std::vector<uint16_t> indices = {
        0, 1, 2,  // First triangle
        2, 3, 0   // Second triangle
    };

    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

    // Create staging buffers
    VkBuffer vertexStagingBuffer, indexStagingBuffer;
    VkDeviceMemory vertexStagingBufferMemory, indexStagingBufferMemory;

    // Vertex staging buffer
    if (!createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vertexStagingBuffer, vertexStagingBufferMemory)) {
        return false;
    }

    void* data;
    vkMapMemory(device_, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)vertexBufferSize);
    vkUnmapMemory(device_, vertexStagingBufferMemory);

    // Create vertex buffer
    if (!createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     quadVertexBuffer_, quadVertexBufferMemory_)) {
        return false;
    }

    copyBuffer(vertexStagingBuffer, quadVertexBuffer_, vertexBufferSize);

    // Index staging buffer
    if (!createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     indexStagingBuffer, indexStagingBufferMemory)) {
        return false;
    }

    vkMapMemory(device_, indexStagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)indexBufferSize);
    vkUnmapMemory(device_, indexStagingBufferMemory);

    // Create index buffer
    if (!createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     quadIndexBuffer_, quadIndexBufferMemory_)) {
        return false;
    }

    copyBuffer(indexStagingBuffer, quadIndexBuffer_, indexBufferSize);

    // Clean up staging buffers
    vkDestroyBuffer(device_, indexStagingBuffer, nullptr);
    vkFreeMemory(device_, indexStagingBufferMemory, nullptr);
    vkDestroyBuffer(device_, vertexStagingBuffer, nullptr);
    vkFreeMemory(device_, vertexStagingBufferMemory, nullptr);

    return true;
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
    UniformBufferObject ubo{};
    // Create orthographic projection matrix (0,0 at top-left, like SDL)
    ubo.projection = glm::ortho(0.0f, static_cast<float>(windowWidth_),
                               static_cast<float>(windowHeight_), 0.0f,
                               -1.0f, 1.0f);

    // Vulkan's clip space Y axis is inverted compared to OpenGL
    // Flip the Y component to match SDL/OpenGL coordinate system
    ubo.projection[1][1] *= -1.0f;

    memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));
}

// ============================================================================
// Shader Loading
// ============================================================================

std::vector<char> VulkanRenderer::readShaderFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        LOG_ERROR_STREAM("Failed to open shader file: " << filename);
        return {};
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        LOG_ERROR("Failed to create shader module");
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

// ============================================================================
// Buffer and Image Helpers
// ============================================================================

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    LOG_ERROR("Failed to find suitable memory type");
    return 0;
}

bool VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                  VkMemoryPropertyFlags properties, VkBuffer& buffer,
                                  VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        LOG_ERROR("Failed to create buffer");
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate buffer memory");
        return false;
    }

    vkBindBufferMemory(device_, buffer, bufferMemory, 0);
    return true;
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

bool VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format,
                                 VkImageTiling tiling, VkImageUsageFlags usage,
                                 VkMemoryPropertyFlags properties, VkImage& image,
                                 VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image");
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device_, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate image memory");
        return false;
    }

    vkBindImageMemory(device_, image, imageMemory, 0);
    return true;
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device_, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image view");
        return VK_NULL_HANDLE;
    }

    return imageView;
}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format,
                                          VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        LOG_ERROR("Unsupported layout transition");
        return;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

// ============================================================================
// Texture Management
// ============================================================================

bool VulkanRenderer::createTextureFromPixels(const Color* pixels, int width, int height, VulkanTexture& vkTexture) {
    VkDeviceSize imageSize = width * height * 4;  // 4 bytes per pixel (RGBA)

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    if (!createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory)) {
        return false;
    }

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device_, stagingBufferMemory);

    // Create image
    if (!createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkTexture.image, vkTexture.memory)) {
        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkFreeMemory(device_, stagingBufferMemory, nullptr);
        return false;
    }

    // Transition image layout and copy buffer
    transitionImageLayout(vkTexture.image, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, vkTexture.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    transitionImageLayout(vkTexture.image, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Clean up staging buffer
    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);

    // Create image view
    vkTexture.imageView = createImageView(vkTexture.image, VK_FORMAT_R8G8B8A8_UNORM);
    if (vkTexture.imageView == VK_NULL_HANDLE) {
        return false;
    }

    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;  // Pixel-perfect for retro 2D
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    if (vkCreateSampler(device_, &samplerInfo, nullptr, &vkTexture.sampler) != VK_SUCCESS) {
        LOG_ERROR("Failed to create texture sampler");
        return false;
    }

    // Create descriptor set for this texture (using texture descriptor set layout)
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureDescriptorSetLayout_;

    if (vkAllocateDescriptorSets(device_, &allocInfo, &vkTexture.descriptorSet) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate descriptor set for texture");
        return false;
    }

    // Update descriptor set
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = vkTexture.imageView;
    imageInfo.sampler = vkTexture.sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = vkTexture.descriptorSet;
    descriptorWrite.dstBinding = 0;  // Binding 0 within set 1
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);

    vkTexture.width = width;
    vkTexture.height = height;

    return true;
}

VulkanRenderer::VulkanTexture* VulkanRenderer::getOrCreateVulkanTexture(Texture* texture) {
    if (!texture || !texture->isValid()) {
        return nullptr;
    }

    void* handle = texture->getHandle();

    // Check if already in cache
    auto it = textureCache_.find(handle);
    if (it != textureCache_.end()) {
        return &it->second;
    }

    // Create new Vulkan texture
    // For now, we'll need to load the texture data from the SDL texture
    // This is a simplified version - in production you'd want to handle this better
    LOG_WARNING("Texture not in Vulkan cache, creating on-demand (not yet implemented)");
    return nullptr;
}

void VulkanRenderer::destroyVulkanTexture(VulkanTexture& vkTexture) {
    if (vkTexture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device_, vkTexture.sampler, nullptr);
        vkTexture.sampler = VK_NULL_HANDLE;
    }
    if (vkTexture.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, vkTexture.imageView, nullptr);
        vkTexture.imageView = VK_NULL_HANDLE;
    }
    if (vkTexture.image != VK_NULL_HANDLE) {
        vkDestroyImage(device_, vkTexture.image, nullptr);
        vkTexture.image = VK_NULL_HANDLE;
    }
    if (vkTexture.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device_, vkTexture.memory, nullptr);
        vkTexture.memory = VK_NULL_HANDLE;
    }
}

} // namespace Engine
