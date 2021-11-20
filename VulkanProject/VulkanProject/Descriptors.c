#include "Descriptors.h"

#include <stdlib.h>
#include <string.h>

#include "Globals.h"
#include "Util.h"
#include "VulkanUtil.h"

BufferInfo create_buffer_info(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags)
{
	const BufferInfo layout_binding = {
		.binding = binding,
		.stage = stage,
		.type = type,
		.buffer_usage = usage,
		.buffer_size = size,
		.memory_property = memFlags
	};
	return layout_binding;
}

DescriptorSetContainer create_descriptor_set(VkInfo* vk, uint32_t set_number, BufferInfo* buffer_infos, uint32_t buffer_count, uint32_t sets_count)
{
	DescriptorSetContainer set = { 0 };

	VkDescriptorSetLayoutBinding* bindings = malloc(sizeof(VkDescriptorSetLayoutBinding) * buffer_count);
	for (uint32_t i = 0;i<buffer_count;i++)
	{
		bindings[i].binding = buffer_infos[i].binding;
		bindings[i].descriptorType = buffer_infos[i].type;
		bindings[i].stageFlags = buffer_infos[i].stage;
		bindings[i].descriptorCount = 1;
		bindings[i].pImmutableSamplers = NULL;
	}

	VkDescriptorSetLayoutCreateInfo layout_create_info = { 0 };
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = buffer_count;
	layout_create_info.pBindings = bindings;

	check(vkCreateDescriptorSetLayout(vk->device, &layout_create_info, NULL, &set.set_layout), "");
	set.buffer_infos = buffer_infos;
	set.buffer_count = buffer_count;
	set.sets_count = sets_count;
	set.set_number = set_number;
	set.completed = 0;

	free(bindings);
	return set;
}
void create_buffers(VkInfo* vk, DescriptorSetContainer* set)
{
	set->buffer_containers = malloc(sizeof(BufferContainer) * set->sets_count); // one container per set
	for (uint32_t set_number = 0; set_number <set->sets_count; set_number++)
	{
		BufferContainer* container = &set->buffer_containers[set_number];
		container->buffer_count = set->buffer_count;
		container->buffers = malloc(sizeof(Buffer) * container->buffer_count);
		for (uint32_t buffer_number = 0; buffer_number< set->buffer_count; buffer_number++)
		{
			Buffer* buffer = &container->buffers[buffer_number];
			BufferInfo* info = &set->buffer_infos[buffer_number];
			createBuffer(vk,
				info->buffer_size,
				info->buffer_usage, info->memory_property,
				&buffer->vk_buffer,
				&buffer->vk_buffer_memory);
			buffer->buffer_size = info->buffer_size;
			buffer->usage = info->buffer_usage;
			buffer->properties = info->memory_property;
		}
	}
}

void create_descriptor_pool(VkInfo* vk)
{
	VkDescriptorPoolSize uniCount = { 0 };
	uniCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniCount.descriptorCount = 10;

	VkDescriptorPoolSize uniDynCount = { 0 };
	uniDynCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	uniDynCount.descriptorCount = 10;

	VkDescriptorPoolSize sampler_image = { 0 };
	sampler_image.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_image.descriptorCount = 10;

	VkDescriptorPoolSize sampler = { 0 };
	sampler.type = VK_DESCRIPTOR_TYPE_SAMPLER;
	sampler.descriptorCount = 10;

	VkDescriptorPoolSize texture_image = { 0 };
	texture_image.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	texture_image.descriptorCount = 10;

	VkDescriptorPoolSize poolSizes[] = { uniCount, uniDynCount , sampler_image, sampler, texture_image };

	VkDescriptorPoolCreateInfo poolInfo = { 0 };
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 5;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 30;

	check(vkCreateDescriptorPool(vk->device, &poolInfo, NULL, &vk->descriptor_pool), "");
}

void create_descriptor_sets(VkInfo* info, DescriptorSetContainer* container)
{
	VkDescriptorSetLayout* layouts = malloc(container->sets_count * sizeof(VkDescriptorSetLayout));
	for (uint32_t i = 0; i < container->sets_count; i++)
	{
		VkDescriptorSetLayout copy = { 0 };
		memcpy(&copy, &container->set_layout, sizeof(VkDescriptorSetLayout));
		layouts[i] = copy;
	}
	VkDescriptorSetAllocateInfo allocInfo = { 0 };
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = info->descriptor_pool;
	allocInfo.descriptorSetCount = container->sets_count;
	allocInfo.pSetLayouts = layouts;

	container->descriptor_sets = malloc(sizeof(VkDescriptorSet) * container->sets_count);

	check(vkAllocateDescriptorSets(info->device, &allocInfo, container->descriptor_sets), "");

	for (size_t set = 0; set < container->sets_count; set++) {
		BufferContainer* buf_con = &container->buffer_containers[set];
		for(uint32_t buf = 0;buf<container->buffer_count;buf++)
		{
			BufferInfo buf_info = container->buffer_infos[buf];
			Buffer* buffer = &buf_con->buffers[buf];

			VkDescriptorBufferInfo bufferInfo = { 0 };
			bufferInfo.buffer = buffer->vk_buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = buffer->buffer_size;

			VkWriteDescriptorSet descriptorWrite = { 0 };
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = container->descriptor_sets[set];
			descriptorWrite.dstBinding = buf_info.binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = buf_info.type;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(info->device, 1, &descriptorWrite, 0, NULL);
		}
	}
	free(layouts);
	container->completed = 1;
}
void destroy_buffer(VkInfo* vk, DescriptorSetContainer* container)
{
	for (uint32_t con = 0;con<container->sets_count;con++)
	{
		BufferContainer bufContainer = container->buffer_containers[con];
		for(uint32_t buf = 0;buf < bufContainer.buffer_count;buf++)
		{
			Buffer buffer = bufContainer.buffers[buf];
			vkFreeMemory(vk->device, buffer.vk_buffer_memory, NULL);
			vkDestroyBuffer(vk->device, buffer.vk_buffer, NULL);
		}
		free(bufContainer.buffers);
	}
	free(container->buffer_infos);
	free(container->descriptor_sets);
	free(container->buffer_containers);
	vkDestroyDescriptorSetLayout(vk->device, container->set_layout, NULL);
}
