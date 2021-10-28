#pragma once
#include "Globals.h"



typedef struct uniformBufferObject {
	float color[3];
	float pad;
} UniformBufferObject;

int compile_shaders();
int get_vertex_shader(VkInfo* vk_info, Shader* shader);
int get_fragment_shader(VkInfo* vk_info, Shader* shader);
int create_descriptor_layouts(VkInfo* vk_info);
int create_uniform_buffers(VkInfo* vk);
int update_uniform_buffers(VkInfo* vk, uint32_t image_index);
int createDescriptorPool(VkInfo* vk);
int createDescriptorSets(VkInfo* vk);
