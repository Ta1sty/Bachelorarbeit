#pragma once
#include "Globals.h"
#include "Scene.h"
void set_global_buffers(VkInfo* vk, Scene* scene);
void set_frame_buffers(VkInfo* vk, Scene* scene, uint32_t image_index);
void drawFrame(VkInfo* vk_info, Scene* scene);