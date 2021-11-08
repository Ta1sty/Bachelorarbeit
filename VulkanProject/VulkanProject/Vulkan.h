#pragma once
#include "Globals.h"
int init_vulkan(VkInfo* info, GLFWwindow** window, SceneData* data);
int create_or_resize_swapchain(VkInfo* vk, GLFWwindow** window, uint32_t width, uint32_t height, SceneData* scene_data);
void destroy_vulkan(VkInfo* vk);
void destroy_swapchain(VkInfo* vk_ptr);