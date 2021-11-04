#include "Vulkan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "Util.h"
#include "Globals.h"
#include "Raster.h"
#include "Shader.h"
#include "VulkanStructs.h"
int init_vulkan(VkInfo* info, GLFWwindow** window)
{
	if(info->rasterize == VK_TRUE)
	{
		initVertexArray();
	}
	if (
		create_instance(info) &&
#ifdef NDEBUG
#else
		create_validation_layer(info) &&
#endif
		create_device(info) &&
		create_or_resize_swapchain(info, window, WINDOW_WIDTH, WINDOW_HEIGHT) &&
		create_semaphores(info)) return SUCCESS;
	return FAILURE;
}
int create_or_resize_swapchain(VkInfo* vk, GLFWwindow** window, uint32_t width, uint32_t height)
{
	vkDeviceWaitIdle(vk->device);

	destroy_swapchain(vk); // destroys the old one

	if (width == 0 || height == 0) return SUCCESS;

	if (create_swapchain(vk, window, width, height) &&
		create_image_views(vk) &&
		create_render_pass(vk) &&
		create_descriptor_containers(vk) && // out
		create_pipeline(vk) &&
		create_frame_buffers(vk) &&
		create_vertex_buffer(vk) && // out
		init_descriptor_containers(vk) && // out
		create_command_buffers(vk)
		)
		return SUCCESS;
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

	set_frame_buffers(vk_info, imageIndex);

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
void destroy_vulkan(VkInfo* vk)
{
	if (vk->imageAvailableSemaphore) vkDestroySemaphore(vk->device, vk->imageAvailableSemaphore, NULL);
	if (vk->renderFinishedSemaphore) vkDestroySemaphore(vk->device, vk->renderFinishedSemaphore, NULL);

	destroy_swapchain(vk);

	if (vk->command_pool) vkDestroyCommandPool(vk->device, vk->command_pool, NULL);
	free(vk->device_extension_names);
	free(vk->instance_extension_names);
	free(vk->queue_family_properties);
	free(vk->physical_devices);
	if (vk->device) vkDestroyDevice(vk->device, NULL);
	if (vk->instance) vkDestroyInstance(vk->instance, NULL);
	freeVertexArray();
	memset(vk, 0, sizeof(VkInfo));
}

// everything that needs to be (re)created when the swapchain is (re)created
void destroy_swapchain(VkInfo* vk)
{
	Swapchain* sw = &vk->swapchain;

	// destroy the swapchain struct
	if (sw->frame_buffers)
	{
		for (uint32_t i = 0; i < sw->image_count; i++)
		{
			vkDestroyFramebuffer(vk->device, vk->swapchain.frame_buffers[i], NULL);
		}
		free(sw->frame_buffers);
	}
	if (sw->image_views)
	{
		for (uint32_t i = 0; i < sw->image_count; i++)
		{
			vkDestroyImageView(vk->device, sw->image_views[i], NULL);
		}
		free(sw->image_views);
	}

	free(sw->images);
	free(sw->surface_formats);
	if(sw->vk_swapchain) vkDestroySwapchainKHR(vk->device, sw->vk_swapchain, NULL);
	if (sw->surface) vkDestroySurfaceKHR(vk->instance, sw->surface, NULL);
	memset(&vk->swapchain, 0, sizeof(Swapchain));


	// everything in VK_info thats associated with the swapchain


	if (vk->vertexBuffer != NULL)
	{
		vkDestroyBuffer(vk->device, vk->vertexBuffer, NULL);
		vk->vertexBuffer = NULL;
	}

	if (vk->vertexBufferMemory != NULL)
	{
		vkFreeMemory(vk->device, vk->vertexBufferMemory, NULL);
		vk->vertexBufferMemory = NULL;
	}


	if (vk->pipeline)
		vkDestroyPipeline(vk->device, vk->pipeline, NULL);
	vk->pipeline = NULL;

	if (vk->pipeline_layout)
		vkDestroyPipelineLayout(vk->device, vk->pipeline_layout, NULL);
	vk->pipeline_layout = NULL;

	if (vk->renderPass)
		vkDestroyRenderPass(vk->device, vk->renderPass, NULL);
	vk->renderPass = NULL;

	if (vk->vertex_shader.module)
		vkDestroyShaderModule(vk->device, vk->vertex_shader.module, NULL);
	vk->vertex_shader.module = NULL;
	free(vk->vertex_shader.code);
	vk->vertex_shader.code = 0;

	if (vk->fragment_shader.module)
		vkDestroyShaderModule(vk->device, vk->fragment_shader.module, NULL);
	vk->fragment_shader.module = NULL;
	free(vk->fragment_shader.code);
	vk->fragment_shader.code = 0;




	free(vk->command_buffers);
	vk->command_buffers = 0;
}