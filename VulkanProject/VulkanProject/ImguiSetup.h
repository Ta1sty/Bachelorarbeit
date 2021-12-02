#pragma once
#include "Globals.h"

#ifdef __cplusplus
extern "C" {
#endif
	void init_imgui(App* app, int width, int height);
	void resize_callback_imgui(VkInfo* vk, Scene* scene);
	void create_imgui_RenderPass(VkInfo* vk);
	void init_imgui_command_buffers(VkInfo* vk, Scene* scene);
	void update_imgui_commandBuffer(VkInfo* vk, Scene* scene, uint32_t index);
	void draw_imgui_frame(VkInfo* info, Scene* scene);
#ifdef __cplusplus
}
#endif