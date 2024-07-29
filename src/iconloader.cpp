#include "include/iconloader.h"
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <ktx.h>

std::unordered_map<std::string, SkyImage*> IconLoader::images;
std::unordered_map<std::string, SkyImage*> IconLoader::atlas_images;
std::unordered_map<std::string, PrivateUIIcon*> IconLoader::icons;
VkDevice IconLoader::device = VK_NULL_HANDLE;
VkPhysicalDevice IconLoader::physicalDevice = VK_NULL_HANDLE;
VkQueue IconLoader::graphicsQueue = VK_NULL_HANDLE;
VkCommandPool IconLoader::commandPool = VK_NULL_HANDLE;

void IconLoader::initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool) {
    IconLoader::device = device;
    IconLoader::physicalDevice = physicalDevice;
    IconLoader::graphicsQueue = graphicsQueue;
    IconLoader::commandPool = commandPool;
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(IconLoader::physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    std::cerr << "failed to find suitable memory type!" << std::endl;
    return 0;
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(IconLoader::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        std::cerr << "failed to create buffer!" << std::endl;
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(IconLoader::device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(IconLoader::device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        std::cerr << "failed to allocate buffer memory!" << std::endl;
        return;
    }

    vkBindBufferMemory(IconLoader::device, buffer, bufferMemory, 0);
}

void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
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

    if (vkCreateImage(IconLoader::device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        std::cerr << "failed to create image!" << std::endl;
        return;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(IconLoader::device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(IconLoader::device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        std::cerr << "failed to allocate image memory!" << std::endl;
        return;
    }

    vkBindImageMemory(IconLoader::device, image, imageMemory, 0);
}

VkCommandBuffer beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = IconLoader::commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(IconLoader::device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(IconLoader::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(IconLoader::graphicsQueue);

    vkFreeCommandBuffers(IconLoader::device, IconLoader::commandPool, 1, &commandBuffer);
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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
        std::cerr << "unsupported layout transition!" << std::endl;
        return;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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

    endSingleTimeCommands(commandBuffer);
}

SkyImage& IconLoader::uploadImageKtx(const char* name, const bool& is_atlas) {
    static SkyImage errorImage; //static SkyImage for error cases
    errorImage.textureId = (ImTextureID)-1;
    errorImage.size = ImVec2(0, 0);
    errorImage.vkTexture = nullptr;

    // Load KTX file
    std::ifstream file("data/assets/initial/Data/Images/Bin/BC" + std::string(name), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        file.open("data/assets/images/Data/Images/Bin/BC/" + std::string(name), std::ios::binary | std::ios::ate);
        if (!file.is_open()){
            std::cerr << "failed to open file: " + std::string(name) << std::endl;
            return errorImage;
        }
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)){
        std::cerr << "failed to read file: " + std::string(name) << std::endl;
        return errorImage;
    }

    ktxTexture* kTexture;
    KTX_error_code result = ktxTexture_CreateFromMemory(
        reinterpret_cast<const ktx_uint8_t*>(buffer.data()),
        buffer.size(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &kTexture
    );

    if (result != KTX_SUCCESS) {
        std::cerr << "failed to create KTX texture from file: " + std::string(name) << std::endl;
        return errorImage;
    }

    VkDeviceSize imageSize = ktxTexture_GetDataSize(kTexture);
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB; // Not sure

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, ktxTexture_GetData(kTexture), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    VulkanTexture* vkTexture = new VulkanTexture();
    createImage(kTexture->baseWidth, kTexture->baseHeight, imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkTexture->image, vkTexture->memory);

    transitionImageLayout(vkTexture->image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyBufferToImage(stagingBuffer, vkTexture->image, kTexture->baseWidth, kTexture->baseHeight);

    transitionImageLayout(vkTexture->image, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vkTexture->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &vkTexture->view) != VK_SUCCESS) {
        std::cerr << "failed to create texture image view!" << std::endl;
        return errorImage;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &vkTexture->sampler) != VK_SUCCESS) {
        std::cerr << "failed to create texture sampler!" << std::endl;
        return errorImage;
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    SkyImage* image = new SkyImage();
    image->textureId = reinterpret_cast<ImTextureID>(vkTexture->view);
    image->size = ImVec2(static_cast<float>(kTexture->baseWidth), static_cast<float>(kTexture->baseHeight));
    image->vkTexture = vkTexture;

    // Store the image
    if (is_atlas) {
        atlas_images[name] = image;
    } else {
        images[name] = image;
    }

    ktxTexture_Destroy(kTexture);

    return *image;
}

PrivateUIIcon* IconLoader::getIcon(const std::string& icon) {
    auto it = icons.find(icon);
    if (it != icons.end()) {
        PrivateUIIcon* ricon = it->second;
        if (ricon->atlasTexture != IL_NO_TEXTURE) {
            return ricon;
        } else {
            if (ricon->atlasTexture == (ImTextureID)-2) {
                ricon->atlasTexture = getAtlasImage(ricon->atlasName).textureId;
            }
            if (ricon->atlasTexture == IL_NO_TEXTURE) {
                return nullptr;
            }
            return ricon;
        }
    }
    return nullptr;
}

void IconLoader::icon(const std::string& name, const float& size, const ImVec4& color) {
    ImVec2 size2 = ImVec2(size, size);
    PrivateUIIcon* icon = getIcon(name);
    if (icon != nullptr) {
        ImGui::Image(icon->atlasTexture, size2, icon->uv0, icon->uv1, color);
    } else {
        ImGui::Dummy(size2);
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetItemRectMin(),
            ImGui::GetItemRectMax(),
            ImGui::ColorConvertFloat4ToU32(ImVec4(1, 0, 1, 1))
        );
    }
}

bool IconLoader::iconButton(const std::string& name, const float& size, const ImVec4& color) {
    ImVec2 size2 = ImVec2(size, size);
    PrivateUIIcon* icon = getIcon(name);
    if (icon != nullptr) {
        return ImGui::ImageButton(
            name.c_str(),
            icon->atlasTexture,
            size2,
            icon->uv0,
            icon->uv1,
            ImVec4(0, 0, 0, 0),
            color
        );
    } else {
        return ImGui::Button(name.c_str(), size2);
    }
}

SkyImage& IconLoader::getImage(const std::string& name) {
    auto it = images.find(name);
    if (it != images.end()) {
        return *it->second;
    } else {
        return uploadImageKtx(name.c_str(), false);
    }
}

SkyImage& IconLoader::getAtlasImage(const std::string& name) {
    auto it = atlas_images.find(name);
    if (it != atlas_images.end()) {
        return *it->second;
    } else {
        return uploadImageKtx(name.c_str(), true);
    }
}

void IconLoader::getUIIcon(const std::string& name, UIIcon* publicIcon) {
    PrivateUIIcon* icon = getIcon(name);
    if (icon != nullptr) {
        publicIcon->textureId = icon->atlasTexture;
        publicIcon->uv0 = icon->uv0;
        publicIcon->uv1 = icon->uv1;
    } else {
        publicIcon->textureId = IL_NO_TEXTURE;
    }
}

void IconLoader::cleanup() {
    for (auto& pair : images) {
        VulkanTexture* vkTexture = pair.second->vkTexture;
        vkDestroySampler(device, vkTexture->sampler, nullptr);
        vkDestroyImageView(device, vkTexture->view, nullptr);
        vkDestroyImage(device, vkTexture->image, nullptr);
        vkFreeMemory(device, vkTexture->memory, nullptr);
        delete vkTexture;
        delete pair.second;
    }
    images.clear();

    for (auto& pair : atlas_images) {
        VulkanTexture* vkTexture = pair.second->vkTexture;
        vkDestroySampler(device, vkTexture->sampler, nullptr);
        vkDestroyImageView(device, vkTexture->view, nullptr);
        vkDestroyImage(device, vkTexture->image, nullptr);
        vkFreeMemory(device, vkTexture->memory, nullptr);
        delete vkTexture;
        delete pair.second;
    }
    atlas_images.clear();

    for (auto& pair : icons) {
        delete pair.second;
    }
    icons.clear();
}

void IconLoader::addIcon(const std::string& name, const std::string& atlas_name, float u0, float v0, float u1, float v1) {
    auto* icon = new PrivateUIIcon;
    icon->uv0 = ImVec2(u0, v0);
    icon->uv1 = ImVec2(u1, v1);
    icon->atlasName = atlas_name;
    icon->atlasTexture = (ImTextureID)-2;
    icons.insert(std::make_pair(name, icon));
}