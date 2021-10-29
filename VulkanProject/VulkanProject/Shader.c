#include "Shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int create_descriptor_layouts(VkInfo* vk_info)
{
	// Per-Frame
	VkDescriptorSetLayoutBinding uboLayoutBinding = { 0 };
	uboLayoutBinding.binding = 2;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = NULL; // Optional

	VkDescriptorSetLayoutCreateInfo framelayoutInfo = { 0 };
	framelayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	framelayoutInfo.bindingCount = 1;
	framelayoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(vk_info->device, &framelayoutInfo, NULL, &vk_info->frame_descriptor_layout))
		return err("Failed to create per frame descriptor set layout");


	// Global
	VkDescriptorSetLayoutBinding sceneDataLayout = {0};
	sceneDataLayout.binding = 0;
	sceneDataLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sceneDataLayout.descriptorCount = 1;
	sceneDataLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	sceneDataLayout.pImmutableSamplers = NULL; // Optional

	VkDescriptorSetLayoutBinding sphereBufferLayout = {0};
	sphereBufferLayout.binding = 1;
	sphereBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	sphereBufferLayout.descriptorCount = 1;
	sphereBufferLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	sphereBufferLayout.pImmutableSamplers = NULL; // Optional
	

	VkDescriptorSetLayoutCreateInfo globalLayoutInfo = { 0 };
	
	VkDescriptorSetLayoutBinding bindings[] = { sceneDataLayout, sphereBufferLayout };
	globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	globalLayoutInfo.bindingCount = 2;
	globalLayoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(vk_info->device, &globalLayoutInfo, NULL, &vk_info->global_descriptor_layout))
		return err("Failed to create global descriptor set layout");

	return SUCCESS;
}

int create_uniform_buffers(VkInfo* vk)
{
	VkDeviceSize bufferSize = sizeof(FrameData);
	vk->uniformBuffers = malloc(vk->buffer_count * sizeof(VkBuffer));
	vk->uniformBufferMemory = malloc(vk->buffer_count * sizeof(VkDeviceMemory));

	for (int32_t i = 0; i < vk->buffer_count; i++) {
		if (createBuffer(vk, 
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vk->uniformBuffers[i],
			&vk->uniformBufferMemory[i]) != SUCCESS)
			return err("Failed to create buffer");
	}

	
	createBuffer(vk, sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vk->sceneDataBuffer,
		&vk->sceneDataMemory);

	createBuffer(vk, 50000, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vk->sphereDataBuffer,
		&vk->sphereDataBufferMemory);

	uint32_t numSpheres = 2;

	Sphere s1 = {
		.x = 0,
		.y = 0,
		.z = 1,
		.r = 0.5f
	};
	Sphere s2 = {
		.x = 1,
		.y = 0,
		.z = 2,
		.r = 0.5f
	};
	Sphere spheres[] = { s1, s2 };
	SphereData sphereData = {
		.spheres = spheres
	};
	SceneData sceneTest = {
		.sphere_count = numSpheres,
		.sphere_data = sphereData
	};

	void* data1;
	vkMapMemory(vk->device, vk->sceneDataMemory, 0, sizeof(SceneData), 0, &data1);
	memcpy(data1, &sceneTest, sizeof(SceneData));
	vkUnmapMemory(vk->device, vk->sceneDataMemory);

	void* data2;
	vkMapMemory(vk->device, vk->sphereDataBufferMemory, 0, sizeof(Sphere) * numSpheres, 0, &data2);
	memcpy(data2, spheres, sizeof(Sphere) * numSpheres);
	vkUnmapMemory(vk->device, vk->sphereDataBufferMemory);
	
	return SUCCESS;
}
int update_frame_buffers(VkInfo* vk, uint32_t image_index) {
	FrameData frame = {0};
	frame.cameraPos[0] = 0;
	frame.cameraPos[1] = 0;
	frame.cameraPos[2] = 0;
	frame.cameraDir[0] = 0;
	frame.cameraDir[1] = 0;
	frame.cameraDir[2] = 1;
	frame.width = WINDOW_WIDTH;
	frame.height = WINDOW_HEIGHT;
	frame.fov = 90;

	void* data;
	if (vkMapMemory(vk->device, vk->uniformBufferMemory[image_index], 0, sizeof(FrameData), 0, &data))
		return err("Failed to map memory");
	memcpy(data, &frame, sizeof(FrameData));
	vkUnmapMemory(vk->device, vk->uniformBufferMemory[image_index]);
	return SUCCESS;
}

int createDescriptorPool(VkInfo* vk)
{
	VkDescriptorPoolSize uniCount = {0};
	uniCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniCount.descriptorCount = 10;

	VkDescriptorPoolSize uniDynCount = {0};
	uniDynCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	uniDynCount.descriptorCount = 10;

	VkDescriptorPoolSize poolSizes[] = { uniCount, uniDynCount };

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 10;

	if (vkCreateDescriptorPool(vk->device, &poolInfo, NULL, &vk->descriptor_pool) != VK_SUCCESS) {
		return FAILURE;
	}

	return SUCCESS;
}

int createDescriptorSets(VkInfo* vk)
{

	// global
	VkDescriptorSetAllocateInfo globalallocInfo = { 0 };
	globalallocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	globalallocInfo.descriptorPool = vk->descriptor_pool;
	globalallocInfo.descriptorSetCount = 1;
	globalallocInfo.pSetLayouts = &vk->global_descriptor_layout;

	if (vkAllocateDescriptorSets(vk->device, &globalallocInfo, &vk->global_descriptor_set) != VK_SUCCESS) {
		return err("failed to allocate descriptor sets");
	}
	VkDescriptorBufferInfo sceneDataInfo = { 0 };
	sceneDataInfo.buffer = vk->sceneDataBuffer;
	sceneDataInfo.offset = 0;
	sceneDataInfo.range = sizeof(SceneData);

	VkWriteDescriptorSet sceneDataDescriptor = { 0 };
	sceneDataDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	sceneDataDescriptor.dstSet = vk->global_descriptor_set;
	sceneDataDescriptor.dstBinding = 0;
	sceneDataDescriptor.dstArrayElement = 0;
	sceneDataDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	sceneDataDescriptor.descriptorCount = 1;
	sceneDataDescriptor.pBufferInfo = &sceneDataInfo;

	vkUpdateDescriptorSets(vk->device, 1, &sceneDataDescriptor, 0, NULL);

	VkDescriptorBufferInfo sphereDataInfo = { 0 };
	sphereDataInfo.buffer = vk->sphereDataBuffer;
	sphereDataInfo.offset = 0;
	sphereDataInfo.range = 50000;

	VkWriteDescriptorSet sphereDataDescriptor = { 0 };
	sphereDataDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	sphereDataDescriptor.dstSet = vk->global_descriptor_set;
	sphereDataDescriptor.dstBinding = 1;
	sphereDataDescriptor.dstArrayElement = 0;
	sphereDataDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	sphereDataDescriptor.descriptorCount = 1;
	sphereDataDescriptor.pBufferInfo = &sphereDataInfo;

	vkUpdateDescriptorSets(vk->device, 1, &sphereDataDescriptor, 0, NULL);

	// per frame
	VkDescriptorSetLayout* layouts = malloc(vk->buffer_count * sizeof(VkDescriptorSetLayout));
	for(uint32_t i = 0;i<vk->buffer_count;i++)
	{
		VkDescriptorSetLayout copy = { 0 };
		memcpy(&copy, &vk->frame_descriptor_layout, sizeof(VkDescriptorSetLayout));
		layouts[i] = copy;
	}
	VkDescriptorSetAllocateInfo frameallocInfo = {0};
	frameallocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	frameallocInfo.descriptorPool = vk->descriptor_pool;
	frameallocInfo.descriptorSetCount = vk->buffer_count;
	frameallocInfo.pSetLayouts = layouts;

	vk->frame_descriptor_sets = malloc(sizeof(VkDescriptorSet) * vk->buffer_count);

	if (vkAllocateDescriptorSets(vk->device, &frameallocInfo, vk->frame_descriptor_sets) != VK_SUCCESS) {
		return err("failed to allocate descriptor sets");
	}
	for (size_t i = 0; i < vk->buffer_count; i++) {
		VkDescriptorBufferInfo bufferInfo = {0};
		bufferInfo.buffer = vk->uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(FrameData);

		VkWriteDescriptorSet descriptorWrite = {0};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = vk->frame_descriptor_sets[i];
		descriptorWrite.dstBinding = 2;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
	}
	free(layouts);
	return SUCCESS;
}
