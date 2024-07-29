#pragma once

#include <string>
#include <unordered_map>
#include <imgui.h>
#include <vulkan/vulkan.h>

#define IL_NO_TEXTURE (ImTextureID)nullptr

struct VulkanTexture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    VkFormat format;
    uint32_t width;
    uint32_t height;
};

class PrivateUIIcon {
public:
    std::string atlasName;
    ImTextureID atlasTexture;
    ImVec2 uv0;
    ImVec2 uv1;
};

class UIIcon {
public:
    ImTextureID textureId;
    ImVec2 uv0;
    ImVec2 uv1;
};

class SkyImage {
public:
    ImTextureID textureId;
    ImVec2 size;
    VulkanTexture* vkTexture;
};

class IconLoader {
private:
    static std::unordered_map<std::string, SkyImage*> images;
    static std::unordered_map<std::string, SkyImage*> atlas_images;
    static std::unordered_map<std::string, PrivateUIIcon*> icons;

    static SkyImage& uploadImageKtx(const char* name, const bool& is_atlas);
    static SkyImage& getAtlasImage(const std::string& name);
    static PrivateUIIcon* getIcon(const std::string& icon);

public:
    static VkDevice device;
    static VkPhysicalDevice physicalDevice;
    static VkQueue graphicsQueue;
    static VkCommandPool commandPool;

    static void initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool);
    static SkyImage& getImage(const std::string& name);
    static void icon(const std::string& name, const float& size, const ImVec4& color = ImVec4(1, 1, 1, 1));
    static bool iconButton(const std::string& name, const float& size, const ImVec4& color = ImVec4(1, 1, 1, 1));
    static void getUIIcon(const std::string& name, UIIcon* icon);
    static void cleanup();
    static void addIcon(const std::string& name, const std::string& atlas_name, float u0, float v0, float u1, float v1);
};