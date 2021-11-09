#pragma once
#include <stdint.h>

#include "Globals.h"

void create_texture_descriptor(VkInfo* vk, Scene* scene, uint32_t binding);
void create_texture_buffers(VkInfo* vk, Scene* scene);
void init_texture_descriptor(VkInfo* vk, Scene* scene_data);
