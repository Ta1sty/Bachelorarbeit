#include "Shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES // for C
#include <math.h>
#include <time.h>

#include "Descriptors.h"
#include "Globals.h"
#include "Util.h"
#include "VulkanUtil.h"
#include "Scene.h"

int compile_shaders()
{
	printf("compiling shaders\n");
	int vert = system("C:/VulkanSDK/1.2.189.2/Bin/glslc.exe shader.vert -o shader.vert.spv");
	int frag = system("C:/VulkanSDK/1.2.189.2/Bin/glslc.exe shader.frag -o shader.frag.spv");

	if (vert || frag)
		return FAILURE;
}
int get_vertex_shader(VkInfo* vk_info, Shader* shader)
{
	FILE* file;
	fopen_s(&file, "shader.vert.spv", "rb");

	if (!file)
		return FAILURE;

	if (fseek(file, 0, SEEK_END) || (shader->size = ftell(file)) < 0) {
		printf("Vertex shader file size could not be determined");
		fclose(file);
		return FAILURE;
	}
	shader->code = malloc(shader->size);
	fseek(file, 0, SEEK_SET);
	shader->size = fread(shader->code, sizeof(char), shader->size, file);
	fclose(file);

	VkShaderModuleCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader->size;
	create_info.pCode = shader->code;

	if(vkCreateShaderModule(vk_info->device, &create_info, NULL, &shader->module))
	{
		return FAILURE;
	}

	return SUCCESS;
}
int get_fragment_shader(VkInfo* vk_info, Shader* shader)
{
	FILE* file;
	fopen_s(&file, "shader.frag.spv", "rb");

	if (!file)
		return FAILURE;

	if (fseek(file, 0, SEEK_END) || (shader->size = ftell(file)) < 0) {
		printf("Fragment shader file size could not be determined");
		fclose(file);
		return FAILURE;
	}
	shader->code = malloc(shader->size);
	fseek(file, 0, SEEK_SET);
	shader->size = fread(shader->code, sizeof(char), shader->size, file);
	fclose(file);

	VkShaderModuleCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader->size;
	create_info.pCode = shader->code;

	if (vkCreateShaderModule(vk_info->device, &create_info, NULL, &shader->module))
	{
		return FAILURE;
	}

	return SUCCESS;
}

int create_descriptor_containers(VkInfo* info, SceneData* data)
{
	if (info->global_buffers.completed == 1 && info->global_buffers.completed)
		return SUCCESS;

	const uint32_t global_buffer_count = 3;

	BufferInfo* globalInfos = malloc(sizeof(BufferInfo) * global_buffer_count);
	BufferInfo sceneInfo = create_buffer_info(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	BufferInfo vertexBuffer = create_buffer_info(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(Vertex) * data->numVertices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	BufferInfo indexBuffer = create_buffer_info(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(uint32_t) * data->numTriangles * 3, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	globalInfos[0] = sceneInfo;
	globalInfos[1] = vertexBuffer;
	globalInfos[2] = indexBuffer;
	info->global_buffers = create_descriptor_set(info, 0, globalInfos, global_buffer_count, 1);

	BufferInfo* frameInfos = malloc(sizeof(BufferInfo) * 1);
	BufferInfo frameInfo = create_buffer_info(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(FrameData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	frameInfos[0] = frameInfo;
	info->per_frame_buffers = create_descriptor_set(info, 1, frameInfos, 1, info->swapchain.image_count);
	return SUCCESS;
}

int init_descriptor_containers(VkInfo* info)
{
	if (info->global_buffers.completed == 1 && info->global_buffers.completed)
		return SUCCESS;
	create_buffers(info, &info->global_buffers);
	create_buffers(info, &info->per_frame_buffers);
	create_descriptor_pool(info);
	create_descriptor_sets(info, &info->global_buffers);
	create_descriptor_sets(info, &info->per_frame_buffers);
	return SUCCESS;
}