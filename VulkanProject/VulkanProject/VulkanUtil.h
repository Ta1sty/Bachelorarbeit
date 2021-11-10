#pragma once
#include <vulkan/vulkan_core.h>

#include "Globals.h"
uint32_t findMemoryType(VkInfo* vk, uint32_t type_filter, VkMemoryPropertyFlags properties);
int createBuffer(VkInfo* vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
VkCommandBuffer beginSingleTimeCommands(VkInfo* vk);
void endSingleTimeCommands(VkInfo* vk, VkCommandBuffer commandBuffer);
void copyBuffer(VkInfo* vk, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);