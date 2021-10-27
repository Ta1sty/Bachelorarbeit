#pragma once
#include "Globals.h"

typedef struct uniformBufferObject {
    float col[3];
} UniformBufferObject;

int compile_shaders();
int get_vertex_shader(VkInfo* vk_info, Shader* shader);
int get_fragment_shader(VkInfo* vk_info, Shader* shader);
int create_descriptor_set_layout(VkInfo* vk_info);
int create_uniform_buffers(VkInfo* vk);
uint32_t findMemoryType(VkInfo* vk, uint32_t typeFilter, VkMemoryPropertyFlags properties);
int createBuffer(VkInfo* vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
int update_uniform_buffers(VkInfo* vk, uint32_t image_index);
int createDescriptorPool(VkInfo* vk);
int createDescriptorSets(VkInfo* vk);
