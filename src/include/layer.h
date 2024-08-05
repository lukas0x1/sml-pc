#pragma once
#include "windows.h"
#include <vulkan/vulkan.h>

namespace layer{
	void setup(HWND hwnd);
}

// made then external for git_loader
extern VkDevice g_Device;
extern VkPhysicalDevice g_PhysicalDevice;
extern VkQueue g_GraphicsQueue;
extern VkCommandBuffer g_CommandBuffer;
extern VkDescriptorPool g_DescriptorPool;

