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

int set_global_buffers(VkInfo* vk)
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
int set_frame_buffers(VkInfo* vk, uint32_t image_index) {
	FrameData frame = {0};
	Camera c = {
	.pos = {0,0,0},
	.rotation_x = 0,
	.rotation_y = 0
	};
	float sec = (time(NULL) % 20) / 20.f;
	c.rotation_x = 20.f * sinf(2 * (float) M_PI * sec);
	c.rotation_y = 20.f * cosf(2 * (float)M_PI * sec);


	float x_radians = c.rotation_x* (float) M_PI / 180.0f;
	float y_radians = c.rotation_y * (float)M_PI / 180.0f;

	float cos_x = cosf(x_radians), sin_x = sinf(x_radians);
	float cos_y = cosf(y_radians), sin_y = sinf(y_radians);


	float mat[4][4] = {
		{cos_y,		-sin_y * sin_x,		sin_y*cos_x,		0},
		{0,			cos_x,				sin_x,				0},
		{-sin_y,	-cos_y*sin_x,		cos_y*cos_x,		0},
		{0, 0,	-2,	1}
	};
	memcpy(&frame.view_to_world, &mat, sizeof(float) * 4 * 4);
	frame.width = WINDOW_WIDTH;
	frame.height = WINDOW_HEIGHT;
	frame.fov = (float) M_PI  / 180.f * 45.f;

	void* data;
	if (vkMapMemory(vk->device, vk->per_frame_buffers.buffer_container[image_index].buffers[0].vk_buffer_memory, 0, sizeof(FrameData), 0, &data))
		return err("Failed to map memory");
	memcpy(data, &frame, sizeof(FrameData));
	vkUnmapMemory(vk->device, vk->per_frame_buffers.buffer_container[image_index].buffers[0].vk_buffer_memory);
	return SUCCESS;
}

int create_descriptor_containers(VkInfo* info)
{
	if (info->global_buffers.completed == 1 && info->global_buffers.completed)
		return SUCCESS;
	
	BufferInfo* globalInfos = malloc(sizeof(BufferInfo) * 2);
	BufferInfo sceneInfo = create_buffer_info(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	BufferInfo sphereInfo = create_buffer_info(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
		50000, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	globalInfos[0] = sceneInfo;
	globalInfos[1] = sphereInfo;
	info->global_buffers = create_descriptor_set(info, 0, globalInfos, 2, 1);

	BufferInfo* frameInfos = malloc(sizeof(BufferInfo) * 1);
	BufferInfo frameInfo = create_buffer_info(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT,
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
	set_global_buffers(info);
	create_descriptor_pool(info);
	create_descriptor_sets(info, &info->global_buffers);
	create_descriptor_sets(info, &info->per_frame_buffers);
	return SUCCESS;
}