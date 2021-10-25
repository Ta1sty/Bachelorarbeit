#pragma once
#include "Globals.h"
int compile_shaders();
int get_vertex_shader(VkInfo* vk_info, Shader* shader);
int get_fragment_shader(VkInfo* vk_info, Shader* shader);
