#pragma once
int create_instance(VkInfo* vk_info);
int create_device(VkInfo* info);
int create_validation_layer(VkInfo* vk_info);
int create_swapchain(VkInfo* vk_info, GLFWwindow** window, VkBool32 resize, uint32_t width, uint32_t height);
int create_image_views(VkInfo* info);
int create_pipeline(VkInfo* info);
int create_render_pass(VkInfo* info);
int create_frame_buffers(VkInfo* info);
int create_command_buffers(VkInfo* info);
int create_semaphores(VkInfo* info);

	
