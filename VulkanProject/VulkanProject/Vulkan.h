#pragma once
#include "Globals.h"
int init_vulkan(VkInfo* info, GLFWwindow** window);
void destroy_vulkan(VkInfo* vk);
void destroy_swapchain(VkInfo* vk_ptr);
int drawFrame(VkInfo* vk_info);