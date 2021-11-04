#pragma once
#include <vulkan/vulkan_core.h>

#include "Globals.h"

int create_descriptor_sets(VkInfo* info, DescriptorSetContainer* container);
int create_descriptor_pool(VkInfo* vk);
int create_buffers(VkInfo* vk, DescriptorSetContainer* set);
DescriptorSetContainer create_descriptor_set(VkInfo* vk, uint32_t set_number, BufferInfo* buffer_infos, uint32_t buffer_count, uint32_t sets_count);
BufferInfo create_buffer_info(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags);