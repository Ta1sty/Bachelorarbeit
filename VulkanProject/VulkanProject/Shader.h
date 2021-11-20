#pragma once
#include "Globals.h"
void compile_shaders();
void get_vertex_shader(VkInfo* vk_info, Shader* shader);
void get_fragment_shader(VkInfo* vk_info, Shader* shader);
void create_descriptor_containers(VkInfo* info, Scene* scene);
void init_descriptor_containers(VkInfo* info, Scene* scene);
void destroy_shaders(VkInfo* vk, Scene* scene);

