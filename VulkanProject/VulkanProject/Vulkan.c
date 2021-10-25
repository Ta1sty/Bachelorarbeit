#include "Vulkan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "Util.h"
#include "Globals.h"
#include "VulkanStructs.h"
int init_vulkan(VkInfo* info, GLFWwindow** window)
{
	if (
		create_instance(info) &&
#ifdef NDEBUG
#else
		create_validation_layer(info) &&
#endif
		create_device(info) &&
		create_swapchain(info, window, VK_FALSE, WINDOW_WIDTH, WINDOW_HEIGHT)&&
		create_image_views(info)&&
		create_render_pass(info) &&
		create_pipeline(info)&&
		create_frame_buffers(info)&&
		create_command_buffers(info)&&
		create_semaphores(info)) return SUCCESS;
	return FAILURE;
}
int drawFrame(VkInfo* vk_info)
{
	Swapchain* swapchain = &vk_info->swapchain;
	uint32_t imageIndex;
	vkAcquireNextImageKHR(vk_info->device, swapchain->vk_swapchain, 
		UINT64_MAX, vk_info->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	const VkSemaphore waitSemaphores[] = { vk_info->imageAvailableSemaphore };
	const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore signalSemaphores[] = { vk_info->renderFinishedSemaphore };
	VkSubmitInfo submitInfo = {
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = waitSemaphores,
	.pWaitDstStageMask = waitStages,
	.commandBufferCount = 1,
	.pCommandBuffers = &swapchain->command_buffers[imageIndex],
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
void destroy_vulkan(VkInfo* vk_ptr)
{
	VkInfo vk = *vk_ptr;

	if (vk.imageAvailableSemaphore) vkDestroySemaphore(vk.device, vk.imageAvailableSemaphore, NULL);
	if (vk.renderFinishedSemaphore) vkDestroySemaphore(vk.device, vk.renderFinishedSemaphore, NULL);
	if (vk.pipeline) vkDestroyPipeline(vk.device, vk.pipeline, NULL);
	if (vk.pipelineLayout) vkDestroyPipelineLayout(vk.device, vk.pipelineLayout, NULL);
	if (vk.renderPass) vkDestroyRenderPass(vk.device, vk.renderPass, NULL);

	destroy_swapchain(vk_ptr);

	if (vk.graphics_queue) vkDestroyCommandPool(vk.device, vk.command_pool, NULL);
	free(vk.device_extension_names);
	free(vk.instance_extension_names);
	free(vk.queue_family_properties);
	free(vk.physical_devices);
	if (vk.vertex_shader.module) vkDestroyShaderModule(vk.device, vk.vertex_shader.module, NULL);
	free(vk.vertex_shader.code);
	if (vk.fragment_shader.module) vkDestroyShaderModule(vk.device, vk.fragment_shader.module, NULL);
	free(vk.fragment_shader.code);
	if (vk.device) vkDestroyDevice(vk.device, NULL);
	if (vk.instance) vkDestroyInstance(vk.instance, NULL);
	memset(vk_ptr, 0, sizeof(VkInfo));
}
void destroy_swapchain(VkInfo* vk_ptr)
{
	VkInfo vk = *vk_ptr;
	Swapchain sw = vk.swapchain;
	if (sw.frame_buffers)
	{
		for (uint32_t i = 0; i < sw.image_view_count; i++)
		{
			vkDestroyFramebuffer(vk.device, vk.swapchain.frame_buffers[i], NULL);
		}
		free(sw.frame_buffers);
	}
	if (sw.image_views)
	{
		for (uint32_t i = 0; i < sw.image_view_count; i++)
		{
			vkDestroyImageView(vk.device, sw.image_views[i], NULL);
		}
		free(sw.image_views);
	}
	free(sw.images);
	free(sw.command_buffers);
	free(sw.surface_formats);
	if(sw.vk_swapchain) vkDestroySwapchainKHR(vk.device, sw.vk_swapchain, NULL);
	if (sw.surface) vkDestroySurfaceKHR(vk.instance, sw.surface, NULL);
	memset(&vk.swapchain, 0, sizeof(Swapchain));
}