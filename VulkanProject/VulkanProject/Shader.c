#include "Shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Globals.h"
#include "Util.h"

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

int create_descriptor_set_layout(VkInfo* vk_info)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = { 0 };
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = NULL; // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(vk_info->device, &layoutInfo, NULL, &vk_info->descriptor_set_layout))
		return err("Failed to create descriptor set layout");
	return SUCCESS;
}

int create_uniform_buffers(VkInfo* vk)
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	vk->uniformBuffers = malloc(vk->buffer_count * sizeof(VkBuffer));
	vk->uniformBufferMemory = malloc(vk->buffer_count * sizeof(VkDeviceMemory));

	for (size_t i = 0; i < vk->buffer_count; i++) {
		if (createBuffer(vk, 
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vk->uniformBuffers[i],
			&vk->uniformBufferMemory[i]) != SUCCESS)
			return err("Failed to create buffer");
	}
	return SUCCESS;
}

int createBuffer(VkInfo* vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(vk->device, &bufferInfo, NULL, buffer)) 
		return err("Failed to create buffer");
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vk->device, *buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	uint32_t memtype = findMemoryType(vk, memRequirements.memoryTypeBits, properties);
	if (memtype == (uint32_t)-1) return err("Failed to find correct memory type");
	allocInfo.memoryTypeIndex = memtype;

	if (vkAllocateMemory(vk->device, &allocInfo, NULL, bufferMemory))
		return err("Failed to allocate buffer memory");

	vkBindBufferMemory(vk->device, *buffer, *bufferMemory, 0);
	return SUCCESS;
}

uint32_t findMemoryType(VkInfo* vk, uint32_t typeFilter, VkMemoryPropertyFlags properties) {

	for (uint32_t i = 0; i < vk->physical_device_memory_properties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (vk->physical_device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return -1;
}
int update_uniform_buffers(VkInfo* vk, uint32_t image_index) {
	UniformBufferObject obj = {0};
	obj.col[0] = 1;
	obj.col[1] = 1;
	obj.col[2] = 1;
	void* data;
	if (vkMapMemory(vk->device, vk->uniformBufferMemory[image_index], 0, sizeof(UniformBufferObject), 0, &data))
		return err("Failed to map memory");
	memcpy(data, &obj, sizeof(image_index));
	vkUnmapMemory(vk->device, vk->uniformBufferMemory[image_index]);
	return SUCCESS;
}

int createDescriptorPool(VkInfo* vk)
{
	VkDescriptorPoolSize poolSize = {0};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = vk->buffer_count;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = vk->buffer_count;

	if (vkCreateDescriptorPool(vk->device, &poolInfo, NULL, &vk->descriptorPool) != VK_SUCCESS) {
		return FAILURE;
	}

	return SUCCESS;
}

int createDescriptorSets(VkInfo* vk)
{
	VkDescriptorSetLayout* layouts = malloc(vk->buffer_count * sizeof(VkDescriptorSetLayout));
	for(uint32_t i = 0;i<vk->buffer_count;i++)
	{
		VkDescriptorSetLayout copy = { 0 };
		memcpy(&copy, &vk->descriptor_set_layout, sizeof(VkDescriptorSetLayout));
		layouts[i] = copy;
	}
	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk->descriptorPool;
	allocInfo.descriptorSetCount = vk->buffer_count;
	allocInfo.pSetLayouts = layouts;

	vk->descriptor_sets = malloc(sizeof(VkDescriptorSet) * vk->buffer_count);

	if (vkAllocateDescriptorSets(vk->device, &allocInfo, vk->descriptor_sets) != VK_SUCCESS) {
		return err("failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < vk->buffer_count; i++) {
		VkDescriptorBufferInfo bufferInfo = {0};
		bufferInfo.buffer = vk->uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite = {0};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = vk->descriptor_sets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(vk->device, 1, &descriptorWrite, 0, NULL);
	}
	return SUCCESS;
}
