#pragma once
#include "Globals.h"
#include "Scene.h"
void set_global_buffers(VkInfo* vk, Scene* scene);
void set_frame_buffers(VkInfo* vk, Scene* scene, uint32_t image_index);
void printSceneSizes(Scene* scene);
void drawFrame(VkInfo* info, Scene* scene, SceneSelection* scene_selection);
