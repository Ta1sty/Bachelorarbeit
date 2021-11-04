#pragma once
#include "Globals.h"
int compile_shaders();
int get_vertex_shader(VkInfo* vk_info, Shader* shader);
int get_fragment_shader(VkInfo* vk_info, Shader* shader);
int set_frame_buffers(VkInfo* vk, uint32_t image_index);


int create_descriptor_containers(VkInfo* info);
int init_descriptor_containers(VkInfo* info);

