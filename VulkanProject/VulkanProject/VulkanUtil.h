﻿#pragma once
#include <vulkan/vulkan_core.h>

#include "Globals.h"
uint32_t findMemoryType(VkInfo* vk, uint32_t type_filter, VkMemoryPropertyFlags properties);
void createBuffer(VkInfo* vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
VkDeviceAddress getBufferDeviceAddress(VkInfo* info, VkBuffer buf);
VkCommandBuffer beginSingleTimeCommands(VkInfo* vk);
void endSingleTimeCommands(VkInfo* vk, VkCommandBuffer commandBuffer);
void copyBuffer(VkInfo* vk, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);