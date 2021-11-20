#pragma once
void create_instance(VkInfo* vk_info);
void create_device(VkInfo* info);
void create_validation_layer(VkInfo* vk_info);
void create_swapchain(VkInfo* vk_info, GLFWwindow** window, uint32_t width, uint32_t height);
void create_image_views(VkInfo* info);
void create_pipeline(VkInfo* info);
void create_render_pass(VkInfo* info);
void create_frame_buffers(VkInfo* info);
void create_command_buffers(VkInfo* info);
void create_semaphores(VkInfo* info);

	
