#include <corecrt_math.h>
#include <corecrt_math_defines.h>
#include <string.h>
#include <time.h>

#include "Globals.h"
#include "Util.h"


int set_global_buffers(VkInfo* vk, Scene* scene)
{
	uint32_t numSpheres = 3;

	Sphere s1 = {
		.x = 0,
		.y = 0,
		.z = 0.5f,
		.r = 0.5f
	};
	Sphere s2 = {
		.x = 1,
		.y = 0,
		.z = 1,
		.r = 0.5f
	};
	Sphere s3 = {
	.x = 0,
	.y = -1,
	.z = 0,
	.r = 0.5f
	};
	Sphere spheres[] = { s1, s2, s3 };
	SphereData sphereData = {
		.spheres = spheres
	};
	SceneData sceneTest = {
		.sphere_count = numSpheres,
		.sphere_data = sphereData
	};

	void* data1;
	vkMapMemory(vk->device, vk->global_buffers.buffer_container[0].buffers[0].vk_buffer_memory, 0, sizeof(SceneData), 0, &data1);
	memcpy(data1, &sceneTest, sizeof(SceneData));
	vkUnmapMemory(vk->device, vk->global_buffers.buffer_container[0].buffers[0].vk_buffer_memory);

	void* data2;
	vkMapMemory(vk->device, vk->global_buffers.buffer_container[0].buffers[1].vk_buffer_memory, 0, sizeof(Sphere) * numSpheres, 0, &data2);
	memcpy(data2, spheres, sizeof(Sphere) * numSpheres);
	vkUnmapMemory(vk->device, vk->global_buffers.buffer_container[0].buffers[1].vk_buffer_memory);

	return SUCCESS;
}

int set_frame_buffers(VkInfo* vk, Scene* scene, uint32_t image_index) {
	FrameData frame = { 0 };
	Camera c = scene->camera;


	float x_radians = c.rotation_x * (float)M_PI / 180.0f;
	float y_radians = c.rotation_y * (float)M_PI / 180.0f;

	float cos_x = cosf(x_radians), sin_x = sinf(x_radians);
	float cos_y = cosf(y_radians), sin_y = sinf(y_radians);


	float mat[4][4] = {
		{cos_y,		-sin_y * sin_x,		sin_y * cos_x,		0},
		{0,			cos_x,				sin_x,				0},
		{-sin_y,	-cos_y * sin_x,		cos_y * cos_x,		0},
		{c.pos[0], c.pos[1],	c.pos[2],	1}
	};
	memcpy(&frame.view_to_world, &mat, sizeof(float) * 4 * 4);
	frame.width = WINDOW_WIDTH;
	frame.height = WINDOW_HEIGHT;
	frame.fov = (float)M_PI / 180.f * 45.f;

	void* data;
	if (vkMapMemory(vk->device, vk->per_frame_buffers.buffer_container[image_index].buffers[0].vk_buffer_memory, 0, sizeof(FrameData), 0, &data))
		return err("Failed to map memory");
	memcpy(data, &frame, sizeof(FrameData));
	vkUnmapMemory(vk->device, vk->per_frame_buffers.buffer_container[image_index].buffers[0].vk_buffer_memory);
	return SUCCESS;
}

int drawFrame(VkInfo* vk_info, Scene* scene)
{
	Swapchain* swapchain = &vk_info->swapchain;
	uint32_t imageIndex;
	vkAcquireNextImageKHR(vk_info->device, swapchain->vk_swapchain,
		UINT64_MAX, vk_info->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	const VkSemaphore waitSemaphores[] = { vk_info->imageAvailableSemaphore };
	const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore signalSemaphores[] = { vk_info->renderFinishedSemaphore };

	set_frame_buffers(vk_info, scene, imageIndex);

	VkSubmitInfo submitInfo = {
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = waitSemaphores,
	.pWaitDstStageMask = waitStages,
	.commandBufferCount = 1,
	.pCommandBuffers = &vk_info->command_buffers[imageIndex],
	.signalSemaphoreCount = 1,
	.pSignalSemaphores = signalSemaphores,
	};

	if (vkQueueSubmit(vk_info->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE)) return err("failed to submit draw command buffer!");
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

	return SUCCESS;
}