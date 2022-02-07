#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Globals.h"
#include "Raster.h"
#include "Shader.h"
#include "Util.h"
#include "VulkanStructs.h"

void create_instance(VkInfo* vk_info) // see https://github.com/MomentsInGraphics/vulkan_renderer
{
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Vulkan renderer",
		.pEngineName = "Vulkan renderer",
		.applicationVersion = 100,
		.engineVersion = 100,
		.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0)
	};

	const int layer_count = 1;
	const char* layer_names[] = {"VK_LAYER_KHRONOS_validation"};

	uint32_t surface_extension_count;
	const char** surface_extension_names = glfwGetRequiredInstanceExtensions(&surface_extension_count);
	vk_info->instance_extension_count = surface_extension_count;
	vk_info->instance_extension_names = malloc(sizeof(char*) * vk_info->instance_extension_count);
	for (uint32_t i = 0; i != surface_extension_count; ++i)
		vk_info->instance_extension_names[i] = surface_extension_names[i];


	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = vk_info->instance_extension_count,
		.ppEnabledExtensionNames = vk_info->instance_extension_names,
#ifdef NDEBUG
	.enabledLayerCount = 0,
#else
		.enabledLayerCount = layer_count,
#endif
		.ppEnabledLayerNames = layer_names
	};

	check(vkCreateInstance(&create_info, NULL, &vk_info->instance),"Failed to create Vulkan instance");

	uint32_t extensionCount = 0;
	check(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL), "Failed to enumerate extension properties");

	if (extensionCount > 0)
	{
		VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
		check(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions), "Failed to enumerate extension properties");
		printf("Supported Extensions:\n");
		for (uint32_t i = 0; i < extensionCount; ++i)
		{
			VkExtensionProperties prop = extensions[i];
			printf("%s\n", prop.extensionName);
		}
		free(extensions);
	}

	vk_info->physical_device_count = 0;
	check(vkEnumeratePhysicalDevices(vk_info->instance, &vk_info->physical_device_count, NULL), "Failed to enumerate physical devices");
	if (vk_info->physical_device_count == 0) error("No GPU found with Vulkan support");
	vk_info->physical_devices = malloc(sizeof(VkPhysicalDevice) * vk_info->physical_device_count);
	check(vkEnumeratePhysicalDevices(vk_info->instance, &vk_info->physical_device_count, vk_info->physical_devices), "Failed to enumerate physical devices");
	printf("The following physical devices (GPUs) are available to Vulkan:\n");
	for (uint32_t i = 0; i != vk_info->physical_device_count; ++i)
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(vk_info->physical_devices[i], &device_properties);
		printf("%u - %s\n", i, device_properties.deviceName);
	}
	vk_info->physical_device = vk_info->physical_devices[0]; // TODO selection
	//
	// Enumerate available memory types
	vkGetPhysicalDeviceMemoryProperties(vk_info->physical_device, &vk_info->physical_device_memory_properties);
	// Learn about available queues
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vk_info->physical_device, &queue_family_count, NULL);
	if (!queue_family_count) error("No Vulkan queue family available");
	vk_info->queue_family_properties = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(vk_info->physical_device, &queue_family_count,
	                                         vk_info->queue_family_properties);
	// Pick a queue that supports graphics and compute
	const uint32_t required_queue_flags = VK_QUEUE_GRAPHICS_BIT & VK_QUEUE_COMPUTE_BIT;
	for (vk_info->queue_family_index = 0;
	     vk_info->queue_family_index < queue_family_count
	     && (vk_info->queue_family_properties[vk_info->queue_family_index].queueFlags & required_queue_flags);
	     ++vk_info->queue_family_index)
	{
	}
	if (vk_info->queue_family_index == queue_family_count)
		error("No Vulkan queue family supports graphics and compute.");
	// Figure out whether ray queries are supported
	uint32_t extension_count = 0;
	vkEnumerateDeviceExtensionProperties(vk_info->physical_device, NULL, &extension_count, NULL);
	VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * extension_count);
	if (vkEnumerateDeviceExtensionProperties(vk_info->physical_device, NULL, &extension_count, extensions))
		extension_count = 0;
	for (uint32_t i = 0; i != extension_count; ++i)
		if (strcmp(extensions[i].extensionName, VK_KHR_RAY_QUERY_EXTENSION_NAME) == 0)
			vk_info->ray_tracing = VK_TRUE;
	free(extensions);	
}

void create_device(VkInfo* vk_info) // see https://github.com/MomentsInGraphics/vulkan_renderer
{
	// Select device extensions
	const uint32_t ext_base_num = 4;
	const char* base_device_extension_names[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
	};
	const uint32_t ext_ray_num = 4;
	const char* ray_tracing_device_extension_names[] = {
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
	};
	vk_info->device_extension_count = ext_base_num;
	if (vk_info->ray_tracing)
		vk_info->device_extension_count += ext_ray_num;
	vk_info->device_extension_names = malloc(sizeof(char*) * vk_info->device_extension_count);
	for (uint32_t i = 0; i != ext_base_num; ++i)
		vk_info->device_extension_names[i] = base_device_extension_names[i];
	if (vk_info->ray_tracing)
		for (uint32_t i = 0; i != ext_ray_num; ++i)
			vk_info->device_extension_names[ext_base_num + i] = ray_tracing_device_extension_names[i];
	float queue_priorities[1] = {0.0f};
	VkDeviceQueueCreateInfo queue_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueCount = 1,
		.pQueuePriorities = queue_priorities,
		.queueFamilyIndex = vk_info->queue_family_index
	};
	VkPhysicalDeviceFeatures enabled_features = {
		.shaderSampledImageArrayDynamicIndexing = VK_TRUE,
		.samplerAnisotropy = VK_TRUE,
		.fragmentStoresAndAtomics = VK_TRUE,
	};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
		.accelerationStructure = VK_TRUE,
	};
	VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
		.pNext = &acceleration_structure_features,
		.rayQuery = VK_TRUE,
	};
	VkPhysicalDeviceVulkan12Features enabled_new_features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = vk_info->ray_tracing ? &ray_query_features : NULL,
		.descriptorIndexing = VK_TRUE,
		.uniformAndStorageBuffer8BitAccess = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE
	};
	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &enabled_new_features,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_info,
		.enabledExtensionCount = vk_info->device_extension_count,
		.ppEnabledExtensionNames = vk_info->device_extension_names,
		.pEnabledFeatures = &enabled_features,
	};
	if (vkCreateDevice(vk_info->physical_device, &device_info, NULL, &vk_info->device))
	{
		printf("Failed to create a Vulkan device with the following extensions:\n");
		for (uint32_t i = 0; i != device_info.enabledExtensionCount; ++i)
		{
			printf("%s\n", device_info.ppEnabledExtensionNames[i]);
		}
		error("");
	}
	// Create a command pool for each queue
	VkCommandPoolCreateInfo command_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = vk_info->queue_family_index,
		.flags = 0
	};
	check(vkCreateCommandPool(vk_info->device, &command_pool_info, NULL, &vk_info->command_pool), "Failed to create command pool for queue");
	// Grab the selected queue
	vkGetDeviceQueue(vk_info->device, vk_info->queue_family_index, 0, &vk_info->graphics_queue);
	vkGetDeviceQueue(vk_info->device, vk_info->queue_family_index, 0, &vk_info->present_queue);
	// Give feedback about ray tracing
	if (vk_info->ray_tracing)
		printf("Ray tracing is available.\n");
	else
		printf("No ray tracing for u\n");

	VkPhysicalDeviceProperties2 prop2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 
	};
	VkPhysicalDeviceAccelerationStructurePropertiesKHR accProp = { 
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR 
	};
	prop2.pNext = &accProp;
	vkGetPhysicalDeviceProperties2(vk_info->physical_device, &prop2);
	int a = 0;
}

void create_validation_layer(VkInfo* vk_info)
{
	printf("Currently debugging\n");
	//TODO validation if I feel like it
}

void create_swapchain(VkInfo* vk_info, GLFWwindow** window, uint32_t width, uint32_t height) // see https://vulkan-tutorial.com/
{
	Swapchain* swapchain = &vk_info->swapchain;
	memset(swapchain, 0, sizeof(Swapchain));
	check(glfwCreateWindowSurface(vk_info->instance, *window, NULL, &swapchain->surface),"Failed to create surface");

	VkBool32 presentation_supported;
	check(vkGetPhysicalDeviceSurfaceSupportKHR(vk_info->physical_device, vk_info->queue_family_index, 
		swapchain->surface, &presentation_supported), "");
	check_b(presentation_supported, "Physical device does not support presentation");

	check(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_info->physical_device, 
			swapchain->surface,&swapchain->surface_format_count, NULL),
		"Failed to get physical device surface formats");
	swapchain->surface_formats = malloc(sizeof(VkSurfaceFormatKHR) * swapchain->surface_format_count);
	check(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_info->physical_device, swapchain->surface,
			&swapchain->surface_format_count, swapchain->surface_formats),
		"Failed to get physical device surface formats");

	if (swapchain->surface_format_count == 1 && swapchain->surface_formats[0].format == VK_FORMAT_UNDEFINED)
		swapchain->format = VK_FORMAT_B8G8R8A8_UNORM;
	for (uint32_t i = 0; i != swapchain->surface_format_count; ++i)
	{
		if (swapchain->surface_formats[i].colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			continue;
		VkFormat format = swapchain->surface_formats[i].format;
		if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_R8G8B8A8_SRGB
			|| format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SRGB
			|| format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_A2R10G10B10_UNORM_PACK32)
		{
			swapchain->format = format;
		}
	}
	// TODO select present mode
	VkPresentModeKHR present_mode;
	if(vk_info->vsync==0) 
		present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	else
		present_mode = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D extent = {
		.height = height,
		.width = width
	};
	swapchain->extent = extent;

	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = swapchain->surface,
		.minImageCount = 2,
		.imageFormat = swapchain->format,
		.imageExtent = swapchain->extent,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.imageArrayLayers = 1,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	};
	check(vkCreateSwapchainKHR(vk_info->device, &create_info, NULL, &swapchain->vk_swapchain), "Failed to create swapchain");
}

void create_image_views(VkInfo* info) // see https://vulkan-tutorial.com/
{
	Swapchain* swapchain = &info->swapchain;
	swapchain->image_count = 0;
	check(vkGetSwapchainImagesKHR(info->device, swapchain->vk_swapchain, &swapchain->image_count, NULL),
		"Failed to get swapchain images");
	info->buffer_count = swapchain->image_count;
	swapchain->images = malloc(sizeof(VkImage) * swapchain->image_count);
	check(vkGetSwapchainImagesKHR(info->device, swapchain->vk_swapchain, &swapchain->image_count, swapchain->images),
		"Failed to get swapchain images");
	swapchain->image_views = malloc(sizeof(VkImageView) * swapchain->image_count);
	for (uint32_t i = 0; i < swapchain->image_count; i++)
	{
		VkImageViewCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapchain->images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchain->format,
			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1,
		};
		check(vkCreateImageView(info->device, &createInfo, NULL, &swapchain->image_views[i]),
			"Failed to create image view");
	}
}

void create_pipeline(VkInfo* info) // see https://vulkan-tutorial.com/
{
	Swapchain* swapchain = &info->swapchain;
	compile_shaders(info->ray_tracing);
	get_vertex_shader(info, &info->vertex_shader);
	get_fragment_shader(info, &info->fragment_shader);

	VkPipelineShaderStageCreateInfo vert_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = info->vertex_shader.module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo frag_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = info->fragment_shader.module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_create_info, frag_create_info};
	VkPipelineVertexInputStateCreateInfo vertex_input_info = { 0 };
	VkVertexInputBindingDescription bindingDescription;
	VkVertexInputAttributeDescription* attributeDescriptions = NULL;
	if(info->rasterize == VK_FALSE)
	{
		VkPipelineVertexInputStateCreateInfo noRasterInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL, // Optional
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL, // Optional
		};
		vertex_input_info = noRasterInfo;
	} else
	{
		bindingDescription = getBindingDescription();
		attributeDescriptions = getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo rasterInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = attributeDescriptions,
		};
		vertex_input_info = rasterInfo;
	}


	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkViewport viewport = {
		viewport.x = 0.0f,
		viewport.y = 0.0f,
		viewport.width = (float)swapchain->extent.width,
		viewport.height = (float)swapchain->extent.height,
		viewport.minDepth = 0.0f,
		viewport.maxDepth = 1.0f
	};

	VkOffset2D offset = {.x = 0, .y = 0};
	VkRect2D scissor = {
		scissor.offset = offset,
		scissor.extent = swapchain->extent,
	};

	VkPipelineViewportStateCreateInfo viewport_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f, // Optional
		.depthBiasClamp = 0.0f, // Optional
		.depthBiasSlopeFactor = 0.0f, // Optional
	};

	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f, // Optional
		.pSampleMask = NULL, // Optional
		.alphaToCoverageEnable = VK_FALSE, // Optional
		.alphaToOneEnable = VK_FALSE, // Optional
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.colorBlendOp = VK_BLEND_OP_ADD, // Optional
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.alphaBlendOp = VK_BLEND_OP_ADD, // Optional
	};

	VkPipelineColorBlendStateCreateInfo color_blending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY, // Optional
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment,
		.blendConstants[0] = 0.0f, // Optional
		.blendConstants[1] = 0.0f, // Optional
		.blendConstants[2] = 0.0f, // Optional
		.blendConstants[3] = 0.0f, // Optional
	};


	VkDescriptorSetLayout layouts[] = {
		info->global_buffers.set_layout,
		info->texture_container.layout ,
		info->per_frame_buffers.set_layout,
		info->ray_descriptor.set_layout
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = info->numSets;
	pipeline_layout_info.pSetLayouts = layouts;

	check(vkCreatePipelineLayout(info->device, &pipeline_layout_info, NULL,&info->pipeline_layout), 
		"Failed to create pipeline layout");

	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shader_stages,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = NULL, // Optional
		.pColorBlendState = &color_blending,
		.pDynamicState = NULL, // Optional
		.layout = info->pipeline_layout,
		.renderPass = info->renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
	};
	check(vkCreateGraphicsPipelines(info->device, VK_NULL_HANDLE, 
		1, &pipeline_info, NULL,&info->pipeline),"Failed to create pipeline");

	free(attributeDescriptions);
}

void create_render_pass(VkInfo* info) // see https://vulkan-tutorial.com/
{
	VkAttachmentDescription color_attachment = {
		.format = info->swapchain.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
	};
	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
	};
	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};
	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};
	check(vkCreateRenderPass(info->device, &render_pass_info, NULL, &info->renderPass), "failed to create render pass");
}

void create_frame_buffers(VkInfo* info) // see https://vulkan-tutorial.com/
{
	Swapchain* swapchain = &info->swapchain;
	swapchain->frame_buffers = malloc(sizeof(VkFramebuffer) * swapchain->image_count);
	for (size_t i = 0; i < swapchain->image_count; i++)
	{
		VkImageView attachments[] = {
			swapchain->image_views[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = info->renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = swapchain->extent.width,
			.height = swapchain->extent.height,
			.layers = 1,
		};
		check(vkCreateFramebuffer(info->device, &framebufferInfo, NULL, &swapchain->frame_buffers[i]),
			"Failed to create Framebuffer");
	}
}

void create_command_buffers(VkInfo* info) // see https://vulkan-tutorial.com/ for general structure, was adjusted accordingly
{
	Swapchain* swapchain = &info->swapchain;
	VkCommandBufferAllocateInfo allocInfo = {
	.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	.commandPool = info->command_pool,
	.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	.commandBufferCount = swapchain->image_count,
	};
	info->command_buffers = malloc(sizeof(VkCommandBuffer) * info->buffer_count);
	check(vkAllocateCommandBuffers(info->device, &allocInfo, info->command_buffers),
		"failed to allocate command buffers");

	for (size_t i = 0; i < swapchain->image_count; i++) {
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0, // Optional
		.pInheritanceInfo = NULL, // Optional
		};

		check(vkBeginCommandBuffer(info->command_buffers[i], &beginInfo),
			"failed to begin command buffer");
		VkOffset2D offset = { .x = 0, .y = 0 };
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

		VkRenderPassBeginInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = info->renderPass,
		.framebuffer = swapchain->frame_buffers[i],
		.renderArea.offset = offset,
		.renderArea.extent = swapchain->extent,
		.clearValueCount = 1,
		.pClearValues = &clearColor
		};

		vkCmdBeginRenderPass(info->command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(info->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, info->pipeline);
		if(info->rasterize == VK_TRUE)
		{
			VkBuffer vertexBuffers[] = { info->vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(info->command_buffers[i], 0, 1, vertexBuffers, offsets);
		}
		vkCmdBindDescriptorSets(info->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, info->pipeline_layout,
			0, 1, &info->global_buffers.descriptor_sets[0], 0, NULL);
		vkCmdBindDescriptorSets(info->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, info->pipeline_layout,
			1, 1, &info->texture_container.descriptor_set, 0, NULL);
		vkCmdBindDescriptorSets(info->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, info->pipeline_layout,
			2, 1, &info->per_frame_buffers.descriptor_sets[i], 0, NULL);
		if (info->ray_tracing) {
			vkCmdBindDescriptorSets(info->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, info->pipeline_layout,
				3, 1, &info->ray_descriptor.descriptor_set, 0, NULL);
		}
		vkCmdDraw(info->command_buffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(info->command_buffers[i]);

		check(vkEndCommandBuffer(info->command_buffers[i]), "failed to end command buffer");
	}
}

void create_semaphores(VkInfo* info) // see https://vulkan-tutorial.com/
{
	if (info->renderFinishedSemaphore)
		return;

	VkSemaphoreCreateInfo semaphore_info = {
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL
	};
	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};


	info->renderFinishedSemaphore = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	info->imageAvailableSemaphore = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	info->inFlightFences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
	info->imagesInFlight = malloc(sizeof(VkFence) * info->swapchain.image_count);
	memset(info->imagesInFlight, 0, sizeof(VkFence) * info->swapchain.image_count);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		check(vkCreateSemaphore(info->device, &semaphore_info, NULL, &info->imageAvailableSemaphore[i]),
			"Failed to create semaphore");
		check(vkCreateSemaphore(info->device, &semaphore_info, NULL, &info->renderFinishedSemaphore[i]),
			"Failed to create semaphores");
		check(vkCreateFence(info->device, &fenceInfo, NULL, &info->inFlightFences[i]), "Failed to create Fence");
	}
}
