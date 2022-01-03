#include "Vulkan.h"

#include <stdlib.h>
#include <string.h>
#include <GLFW/glfw3.h>

#include "Util.h"
#include "Globals.h"
#include "ImguiSetup.h"
#include "Raster.h"
#include "Shader.h"
#include "VulkanStructs.h"
void init_vulkan(VkInfo* info, GLFWwindow** window, Scene* scene)
{
	if(info->rasterize == VK_TRUE)
	{
		initVertexArray();
	}
	create_instance(info);
#ifdef NDEBUG
#else
	create_validation_layer(info);
#endif
	create_device(info);
}
void create_or_resize_swapchain(VkInfo* vk, GLFWwindow** window, uint32_t width, uint32_t height, Scene* scene)
{
	destroy_swapchain(vk); // destroys the old one

	if (width == 0 || height == 0) return;

	create_swapchain(vk, window, width, height);
	create_image_views(vk);
	create_render_pass(vk);
	create_descriptor_containers(vk, scene); // out
	create_pipeline(vk);
	create_frame_buffers(vk);
	create_vertex_buffer(vk); // out
	init_descriptor_containers(vk, scene); // out
	create_command_buffers(vk);
	create_semaphores(vk);
}
void destroy_vulkan(VkInfo* vk, Scene* scene, SceneSelection* scene_selection)
{
	vkDeviceWaitIdle(vk->device);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(vk->device, vk->imageAvailableSemaphore[i], NULL);
		vkDestroySemaphore(vk->device, vk->renderFinishedSemaphore[i], NULL);
		vkDestroyFence(vk->device, vk->inFlightFences[i], NULL);
	}
	free(vk->imageAvailableSemaphore);
	free(vk->renderFinishedSemaphore);
	free(vk->inFlightFences);
	free(vk->imagesInFlight);


	destroy_shaders(vk, scene);
	destroy_imgui(vk, scene_selection);
	destroy_swapchain(vk);
	vkDestroySampler(vk->device, scene->sampler, NULL);

	vkDestroyDescriptorPool(vk->device, vk->descriptor_pool, NULL);

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