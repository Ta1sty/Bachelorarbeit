#include "Shader.h"

#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES // for C

#include "Descriptors.h"
#include "Globals.h"
#include "Util.h"
#include "Scene.h"
#include "Textures.h"
#include "Raytrace.h"
#include "Bindings.h"

void compile_shaders(VkBool32 ray_trace)
{
	printf("compiling shaders\n");
	int vert = system("%VK_SDK_PATH%/Bin/glslc.exe shader.vert -o shader.vert.spv --target-env=vulkan1.2");
	int frag;
	if (ray_trace) {
		frag = system("%VK_SDK_PATH%/Bin/glslc.exe shader.frag -o shader.frag.spv --target-env=vulkan1.2 -DRAY_TRACE");
	}
	else {
		frag = system("%VK_SDK_PATH%/Bin/glslc.exe shader.frag -o shader.frag.spv --target-env=vulkan1.2");
	}
	
	if (vert || frag)
		error("Failed to compile shaders");
}
void get_vertex_shader(VkInfo* vk_info, Shader* shader)
{
	FILE* file;
	fopen_s(&file, "shader.vert.spv", "rb");

	if (!file)
		error("Failed to open vertexshader");

	if (fseek(file, 0, SEEK_END) || (shader->size = ftell(file)) < 0) {
		fclose(file);
		error("Vertex shader file size could not be determined");
	}
	shader->code = malloc(shader->size);
	fseek(file, 0, SEEK_SET);
	shader->size = fread(shader->code, sizeof(char), shader->size, file);
	fclose(file);

	VkShaderModuleCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader->size;
	create_info.pCode = shader->code;

	check(vkCreateShaderModule(vk_info->device, &create_info, NULL, &shader->module) ,"");
}
void get_fragment_shader(VkInfo* vk_info, Shader* shader)
{
	FILE* file;
	fopen_s(&file, "shader.frag.spv", "rb");

	if (!file)
		error("Failed to open fragment shader");

	if (fseek(file, 0, SEEK_END) || (shader->size = ftell(file)) < 0) {
		fclose(file);
		error("Fragment shader file size could not be determined");
	}

	shader->code = malloc(shader->size);
	fseek(file, 0, SEEK_SET);
	shader->size = fread(shader->code, sizeof(char), shader->size, file);
	fclose(file);

	VkShaderModuleCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader->size;
	create_info.pCode = shader->code;

	check(vkCreateShaderModule(vk_info->device, &create_info, NULL, &shader->module), "");
}

void create_descriptor_containers(VkInfo* info, Scene* scene)
{
	if (info->global_buffers.completed == 1 && info->global_buffers.completed)
		return;

	info->numSets = info->ray_tracing ? 4 : 3;
	
	// set 0 - global buffers
	BufferInfo* globalInfos = malloc(sizeof(BufferInfo) * GLOBAL_BUFFER_COUNT);
	BufferInfo sceneInfo = create_buffer_info(SCENE_DATA_BINDING, 
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(SceneData), 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	BufferInfo vertexBuffer = create_buffer_info(VERTEX_BUFFER_BINDING, 
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(Vertex) * scene->scene_data.numVertices, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	BufferInfo indexBuffer = create_buffer_info(INDEX_BUFFER_BINDING, 
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(uint32_t) * scene->scene_data.numTriangles * 3, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	BufferInfo materialBuffer = create_buffer_info(MATERIAL_BUFFER_BINDING, 
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(Material) * scene->texture_data.num_materials, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	BufferInfo lightBuffer = create_buffer_info(LIGHT_BUFFER_BINDING,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(Light) * scene->scene_data.numLights,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	BufferInfo nodeBuffer = create_buffer_info(NODE_BUFFER_BINDING,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(SceneNode) * scene->scene_data.numSceneNodes,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


	BufferInfo nodeIndices = create_buffer_info(NODE_CHILDREN_BINDING,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(uint32_t) * scene->scene_data.numNodeIndices,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	globalInfos[0] = sceneInfo;
	globalInfos[1] = vertexBuffer;
	globalInfos[2] = indexBuffer;
	globalInfos[3] = materialBuffer;
	globalInfos[4] = lightBuffer;
	globalInfos[5] = nodeBuffer;
	globalInfos[6] = nodeIndices;
	
	info->global_buffers = create_descriptor_set(info, 0, globalInfos, GLOBAL_BUFFER_COUNT, 1);

	// set 1 - samplers and textures
	create_texture_descriptors(info, scene, SAMPLER_BINDING, TEXTURE_BINDING);

	// set 2 - frame buffers

	BufferInfo* frameInfos = malloc(sizeof(BufferInfo) * 1);
	BufferInfo frameInfo = create_buffer_info(FRAME_DATA_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(FrameData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	frameInfos[0] = frameInfo;
	info->per_frame_buffers = create_descriptor_set(info, 2, frameInfos, 1, info->swapchain.image_count);

	// set 3 - ray TLAS
	if (info->ray_tracing) {
		build_all_acceleration_structures(info, scene);
		create_ray_descriptors(info, scene, TLAS_BINDING);
	}
}

void init_descriptor_containers(VkInfo* info, Scene* scene)
{
	if (info->global_buffers.completed == 1 && info->per_frame_buffers.completed)
		return;
	create_buffers(info, &info->global_buffers);
	create_buffers(info, &info->per_frame_buffers);
	create_texture_buffers(info, scene);
	if(info->descriptor_pool)
	{
		vkResetDescriptorPool(info->device, info->descriptor_pool, 0);
	} else
	{
		create_descriptor_pool(info);
	}
	create_descriptor_sets(info, &info->global_buffers);
	init_texture_descriptor(info, scene);
	create_descriptor_sets(info, &info->per_frame_buffers);

	if (info->ray_tracing) {
		init_ray_descriptors(info, scene);
	}
}

void destroy_shaders(VkInfo* vk, Scene* scene)
{
	destroy_buffer(vk, &vk->per_frame_buffers);
	destroy_buffer(vk, &vk->global_buffers);

	// destroy texture container, destroying the textures them self is responsibility of the scene
	vkDestroyDescriptorSetLayout(vk->device, vk->texture_container.layout, NULL);

	for(uint32_t i = 0;i<scene->texture_data.num_textures;i++)
	{
		Texture* t = &scene->texture_data.textures[i];
		vkDestroyImageView(vk->device, t->texture_image_view, NULL);
		vkFreeMemory(vk->device, t->texture_image_memory, NULL);
		vkDestroyImage(vk->device, t->texture_image, NULL);
	}
	vkDestroySampler(vk->device, scene->sampler, NULL);
}