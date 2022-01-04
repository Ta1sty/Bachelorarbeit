#pragma once
#include "Globals.h"
void init_vulkan(VkInfo* info, GLFWwindow** window, Scene* scene);
void create_or_resize_swapchain(VkInfo* vk, GLFWwindow** window, uint32_t width, uint32_t height, Scene* scene_data);
void destroy_vulkan(VkInfo* vk, Scene* scene, SceneSelection* scene_selection);
void destroy_swapchain(VkInfo* vk_ptr);