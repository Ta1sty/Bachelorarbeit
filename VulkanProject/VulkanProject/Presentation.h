#pragma once
#include "Globals.h"
#include "Scene.h"
int set_global_buffers(VkInfo* vk, Scene* scene);
int set_texture_buffers(VkInfo* vk, Scene* scene);
int set_frame_buffers(VkInfo* vk, Scene* scene, uint32_t image_index);
int drawFrame(VkInfo* vk_info, Scene* scene);