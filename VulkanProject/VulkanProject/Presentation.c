#include <corecrt_math.h>
#include <corecrt_math_defines.h>
#include <string.h>

#include "Globals.h"
#include "Util.h"
#include "Presentation.h"


void set_global_buffers(VkInfo* vk, Scene* scene)
{

	// Scene Data
	void* data1;
	check(vkMapMemory(vk->device, vk->global_buffers.buffer_containers[0].buffers[0].vk_buffer_memory,
		0, sizeof(SceneData), 0, &data1), "");
	memcpy(data1, &scene->scene_data, sizeof(SceneData));
	vkUnmapMemory(vk->device, vk->global_buffers.buffer_containers[0].buffers[0].vk_buffer_memory);

	// Vertex buffer
	void* data2;
	check(vkMapMemory(vk->device, vk->global_buffers.buffer_containers[0].buffers[1].vk_buffer_memory, 
		0, sizeof(Vertex) * scene->scene_data.numVertices, 0, &data2), "");
	memcpy(data2, scene->vertices, sizeof(Vertex) * scene->scene_data.numVertices);
	vkUnmapMemory(vk->device, vk->global_buffers.buffer_containers[0].buffers[1].vk_buffer_memory);

	// IndexBuffer
	void* data3;
	check(vkMapMemory(vk->device, vk->global_buffers.buffer_containers[0].buffers[2].vk_buffer_memory,
		0, sizeof(uint32_t) * scene->scene_data.numTriangles * 3, 0, &data3), "");
	memcpy(data3, scene->indices, sizeof(uint32_t) * scene->scene_data.numTriangles * 3);
	vkUnmapMemory(vk->device, vk->global_buffers.buffer_containers[0].buffers[2].vk_buffer_memory);

	void* data4;
	check(vkMapMemory(vk->device, vk->global_buffers.buffer_containers[0].buffers[3].vk_buffer_memory,
		0, sizeof(Material) * scene->texture_data.num_materials, 0, &data4), "");
	memcpy(data4, scene->texture_data.materials, sizeof(Material) * scene->texture_data.num_materials);
	vkUnmapMemory(vk->device, vk->global_buffers.buffer_containers[0].buffers[3].vk_buffer_memory);
}

void set_frame_buffers(VkInfo* vk, Scene* scene, uint32_t image_index) {
	FrameData frame = { 0 };
	Camera c = scene->camera;


	float x_radians = c.rotation_x * (float)M_PI / 180.0f;
	float y_radians = c.rotation_y * (float)M_PI / 180.0f;

	float cos_x = cosf(x_radians), sin_x = sinf(x_radians);
	float cos_y = cosf(y_radians), sin_y = sinf(y_radians);

	float rotation_x[3][3] = {
		{1.0f, 0.0f, 0.0f},
		{0.0f, cos_x, sin_x},
		{0.0f, -sin_x, cos_x}
	};
	float rotation_z[3][3] = {
		{cos_y, 0, sin_y},
		{0, 1, 0},
		{-sin_y, 0.0f, cos_y}
	};
	float rotation[3][3] = {0};
	for (uint32_t i = 0; i != 3; i++)
		for (uint32_t j = 0; j != 3; j++)
			for (uint32_t k = 0; k != 3; k++)
				rotation[i][j] += rotation_x[i][k] * rotation_z[k][j];


	float mat[4][4] = {
		{rotation[0][0], rotation[0][1], rotation[0][2], 0},
		{rotation[1][0], rotation[1][1], rotation[1][2], 0},
		{rotation[2][0], rotation[2][1], rotation[2][2], 0},
		{c.pos[0], c.pos[1], c.pos[2], 1.0f}
	};



	memcpy(&frame.view_to_world, &mat, sizeof(float) * 4 * 4);
	frame.width = WINDOW_WIDTH;
	frame.height = WINDOW_HEIGHT;
	frame.fov = (float)M_PI / 180.f * 45.f;

	void* data;
	check(vkMapMemory(vk->device, vk->per_frame_buffers.buffer_containers[image_index].buffers[0].vk_buffer_memory, 
		0, sizeof(FrameData), 0, &data), "");
	memcpy(data, &frame, sizeof(FrameData));
	vkUnmapMemory(vk->device, vk->per_frame_buffers.buffer_containers[image_index].buffers[0].vk_buffer_memory);
}

void drawFrame(VkInfo* vk_info, Scene* scene)
{
	Swapchain* swapchain = &vk_info->swapchain;
	uint32_t imageIndex;
	vkAcquireNextImageKHR(vk_info->device, swapchain->vk_swapchain,
		UINT64_MAX, vk_info->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	const VkSemaphore waitSemaphores[] = { vk_info->imageAvailableSemaphore };
	const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore signalSemaphores[] = { vk_info->renderFinishedSemaphore };

	set_frame_buffers(vk_info, scene, imageIndex);

	VkCommandBuffer buffers[] = { vk_info->command_buffers[imageIndex] , vk_info->imgui_command_buffers[imageIndex]};

	VkSubmitInfo submitInfo = {
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = waitSemaphores,
	.pWaitDstStageMask = waitStages,
	.commandBufferCount = 2,
	.pCommandBuffers = buffers,
	.signalSemaphoreCount = 1,
	.pSignalSemaphores = signalSemaphores,
	};

	check(vkQueueSubmit(vk_info->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE),"failed to submit draw command buffer!");
	VkSwapchainKHR swapChains[] = { swapchain->vk_swapchain };

	VkPresentInfoKHR presentInfo = {
	.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = signalSemaphores,
	.swapchainCount = 1,
	.pSwapchains = swapChains,
	.pImageIndices = &imageIndex,
	.pResults = NULL // Optional
	};
	vkQueuePresentKHR(vk_info->present_queue, &presentInfo);

	vkQueueWaitIdle(vk_info->present_queue);
}