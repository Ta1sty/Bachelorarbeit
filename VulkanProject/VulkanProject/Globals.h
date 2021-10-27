#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

typedef struct shader
{
	VkShaderModule module;
	uint32_t* code;
	size_t size;
} Shader;

typedef struct swapchain
{
	VkSwapchainKHR vk_swapchain;
	VkSurfaceKHR surface;
	uint32_t surface_format_count;
	VkSurfaceFormatKHR* surface_formats;

	VkFormat format;
	VkExtent2D extent;

	uint32_t image_count;
	VkImage* images;

	VkImageView* image_views;
	VkFramebuffer* frame_buffers;
} Swapchain;

typedef struct vkInfo {
	// vk access
	VkInstance instance;
	// devices
	VkDevice device;
	VkPhysicalDevice physical_device;
	uint32_t physical_device_count;
	VkPhysicalDevice* physical_devices;
	// memory
	VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
	VkQueueFamilyProperties* queue_family_properties;
	uint32_t queue_family_index;
	// extensions
	uint32_t instance_extension_count;
	const char** instance_extension_names;
	uint32_t device_extension_count;
	const char** device_extension_names;

	VkBool32 ray_tracing;
	// command pool
	VkCommandPool command_pool;
	VkQueue graphics_queue;
	VkQueue present_queue;
	uint32_t buffer_count;
	VkCommandBuffer* command_buffers;

	Swapchain swapchain;
	// shader objects
	Shader vertex_shader;
	Shader fragment_shader;

	VkBuffer* uniformBuffers;
	VkDeviceMemory* uniformBufferMemory;

	VkDescriptorPool descriptorPool;
	VkDescriptorSet* descriptor_sets;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
} VkInfo;

typedef struct app {
	VkInfo vk_info;
	GLFWwindow* window;
} App;


int err(char* err);