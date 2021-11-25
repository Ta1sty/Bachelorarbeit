#pragma once
#include "Globals.h"

#ifdef __cplusplus
extern "C" {
#endif
	void init_imgui(App* app, int width, int height);
	void resize_callback_imgui(VkInfo* vk);
	void create_imgui_RenderPass(VkInfo* vk);
	void init_imgui_command_buffers(VkInfo* vk);
#ifdef __cplusplus
}
#endif