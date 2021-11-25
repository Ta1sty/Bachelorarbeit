#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "Scene.h"

struct ImGuiWindow;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

typedef struct textureContainer
{
	VkDescriptorSetLayout layout;
	VkDescriptorSet descriptor_set;
	uint32_t sampler_binding;
	uint32_t texture_binding;
} TextureContainer;

typedef struct buffer
{
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags properties;
	VkBuffer vk_buffer;
	VkDeviceMemory vk_buffer_memory;
	size_t buffer_size;
} Buffer;

typedef struct bufferContainer
{
	uint32_t buffer_count;
	Buffer* buffers;
} BufferContainer;

typedef struct bufferInfo
{
	uint32_t binding;
	VkDescriptorType type;
	VkShaderStageFlags stage;
	size_t buffer_size;
	VkBufferUsageFlags buffer_usage;
	VkMemoryPropertyFlags memory_property;
} BufferInfo;

typedef struct descriptorSet
{
	VkDescriptorSetLayout set_layout;
	uint32_t sets_count; // number of sets 
	VkDescriptorSet* descriptor_sets; // || = sets_count
	uint32_t buffer_count; // number of buffers per set
	BufferInfo* buffer_infos; // buffer_count
	BufferContainer* buffer_containers; // || = sets_count
	uint32_t set_number;
	uint32_t completed; // indicates that this container is fully operational
} DescriptorSetContainer;


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
	VkFramebuffer* imgui_frame_buffers;

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
	VkBool32 rasterize;
	// command pool
	VkCommandPool command_pool;
	VkQueue graphics_queue;
	VkQueue present_queue;
	uint32_t buffer_count;
	VkCommandBuffer* command_buffers;
	VkCommandBuffer* imgui_command_buffers;

	Swapchain swapchain;
	// shader objects
	Shader vertex_shader;
	Shader fragment_shader;

	uint32_t numSets; // 3
	DescriptorSetContainer global_buffers; // set 0
	TextureContainer texture_container; // set 1
	DescriptorSetContainer per_frame_buffers; // set 2


	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkDescriptorPool descriptor_pool;

	VkRenderPass renderPass;
	VkRenderPass imguiPass;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
} VkInfo;



typedef struct app {
	VkInfo vk_info;
	GLFWwindow* window;
	Scene scene;
	void* imgui_window;
} App;