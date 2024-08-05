#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <imgui.h>
#include <fstream>
#include <vector>
#include <iostream>

#include "include/layer.h"
#include "include/gif_loader.h"

GifLoader::GifLoader()
    :m_CurrentFrame(0)
   , m_TotalFrames(0)
   , m_FrameDelay(0.1f)
   , m_LastFrameTime(0.0f)
{}

GifLoader::~GifLoader() {
    DestroyTextures();
}

bool GifLoader::LoadGif(const char* filename) {
    // Read file content
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Failed to read file: " << filename << std::endl;
        return false;
    }

    int* delays = nullptr;
    int x, y, z, comp, req_comp = 4;

    // Try to decode as GIF
    stbi_uc* gif_data = stbi_load_gif_from_memory(reinterpret_cast<stbi_uc*>(buffer.data()), size, &delays, &x, &y, &z, &comp, req_comp);

    if (!gif_data) {
        std::cerr << "Failed to decode GIF: " << filename << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        return false;
    }

    std::cout << "GIF decoded successfully. Frames: " << z << ", Size: " << x << "x" << y << ", Components: " << comp << std::endl;

    // Create Vulkan textures
    VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
    VkExtent2D extent = { x, y };
    CreateTextures(format, extent, z);

    // Copy data to textures
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(g_Device, m_Image, &memRequirements);

    void* data;
    vkMapMemory(g_Device, m_ImageMemory, 0, memRequirements.size, 0, &data);

    VkDeviceSize layerSize = extent.width * extent.height * req_comp;
    for (uint32_t i = 0; i < z; i++) {
        memcpy((uint8_t*)data + i * layerSize, gif_data + i * layerSize, layerSize);
    }

    vkUnmapMemory(g_Device, m_ImageMemory);

    m_TotalFrames = z;
    if (m_TotalFrames!= m_Frames.size()) {
        std::cerr << "Error: m_TotalFrames (" << m_TotalFrames << ") does not match m_Frames.size() (" << m_Frames.size() << ")" << std::endl;
    }
    m_FrameDelay = delays[0] / 1000.0f; // Convert from milliseconds to seconds

    stbi_image_free(gif_data);
    STBI_FREE(delays);
    return true;
}

void GifLoader::UpdateFrame() {
    float currentTime = ImGui::GetTime();
    if (currentTime - m_LastFrameTime >= m_FrameDelay) {
        m_CurrentFrame = (m_CurrentFrame + 1) % m_TotalFrames;
        m_LastFrameTime = currentTime;
    }
}

void GifLoader::RenderFrame(float x, float y) {
    UpdateFrame();
    try {
        if (!m_Frames.empty()) {
            if (m_CurrentFrame >= m_Frames.size()) {
                std::cerr << "Error: m_CurrentFrame (" << m_CurrentFrame << ") exceeds m_Frames.size() (" << m_Frames.size() << ")" << std::endl;
                return;
            }
            
            std::cout << "Rendering frame " << m_CurrentFrame << " of " << m_Frames.size() << std::endl;

            
                if (m_CurrentFrame < m_Frames.size() && m_Frames[m_CurrentFrame]!= VK_NULL_HANDLE) {
                    // ImGui::Image(m_Frames[m_CurrentFrame], ImVec2(150, 150));    // this sucker crashes the whole thing
                    // std::cout << m_Frames[m_CurrentFrame] << "\n";
                } else {
                    throw std::runtime_error("Invalid frame index or frame is null!");
                }
            std::cout << "Rendering frame Complete \n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in ImGui::Image: " << e.what() << " in frame " << m_CurrentFrame << std::endl;
    }
}

void GifLoader::CreateTextures(VkFormat format, VkExtent2D extent, uint32_t frameCount) {
    VkImageCreateFlags flags = 0;
    VkImageCreateInfo imageInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        VK_IMAGE_TYPE_2D,
        format,
        VkExtent3D{ extent.width, extent.height, 1 },
        1,
        frameCount,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    if (vkCreateImage(g_Device, &imageInfo, nullptr, &m_Image)!= VK_SUCCESS) {
        std::cerr << "failed to create image!\n";
    }

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    AllocateMemory(&m_ImageMemory, &imageInfo, properties);

    vkBindImageMemory(g_Device, m_Image, m_ImageMemory, 0);

    CreateImageView(&m_ImageView, format);

    for (uint32_t i = 0; i < frameCount; i++) {
        VkImageView imageView;
        CreateImageView(&imageView, format);
        m_Frames.push_back(reinterpret_cast<ImTextureID>(imageView));
    }
}

void GifLoader::CreateImageView(VkImageView* imageView, VkFormat format) {
    VkImageViewCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        m_Image,
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        format,
        { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
    };

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = m_TotalFrames;

    if (vkCreateImageView(g_Device, &createInfo, nullptr, imageView)!= VK_SUCCESS) {
        std::cerr << "failed to create image views!\n";
    }
}

void GifLoader::AllocateMemory(VkDeviceMemory* memory, VkImageCreateInfo* imageInfo, VkMemoryPropertyFlags properties) {
    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        0,
        0
    };

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(g_Device, m_Image, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(g_Device, &allocInfo, nullptr, memory)!= VK_SUCCESS) {
        std::cerr << "failed to allocate memory!\n";
    }
}

uint32_t GifLoader::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(g_PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    std::cerr << "failed to find suitable memory type!\n";
    return 0;
}

void GifLoader::DestroyTextures() {
    for (auto& imageView : m_Frames) {
        vkDestroyImageView(g_Device, reinterpret_cast<VkImageView>(imageView), nullptr);
    }
    vkDestroyImageView(g_Device, m_ImageView, nullptr);
    vkDestroyImage(g_Device, m_Image, nullptr);
    vkFreeMemory(g_Device, m_ImageMemory, nullptr);
}
