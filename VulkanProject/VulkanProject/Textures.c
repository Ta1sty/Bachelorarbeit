#include "Textures.h"

#include <stdlib.h>
#include <string.h>

#include "VulkanUtil.h"

void create_texture_descriptor(VkInfo* vk, Scene* scene, uint32_t binding)
{
	TextureContainer container = { 0 };

	VkDescriptorSetLayoutBinding sampler_binding = {
	.binding = binding,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
	.pImmutableSamplers = NULL,
	.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDescriptorSetLayoutBinding texture_binding = {
	.binding = binding+1,
	.descriptorCount = scene->texture_data.num_textures,
	.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	.pImmutableSamplers = NULL,
	.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDescriptorSetLayoutBinding bindings[] = { sampler_binding, texture_binding};

	VkDescriptorSetLayoutCreateInfo layout_create_info = { 0 };
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = 2;
	layout_create_info.pBindings = bindings;

	vkCreateDescriptorSetLayout(vk->device, &layout_create_info, NULL, &vk->texture_container.layout);
}

void create_texture_image(VkInfo* vk, Texture* texture, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkImageCreateInfo imageInfo = { 0 };
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = texture->image_width;
	imageInfo.extent.height = texture->image_height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateImage(vk->device, &imageInfo, NULL, &texture->texture_image);

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(vk->device, texture->texture_image, &mem_req);

	VkMemoryAllocateInfo allocInfo = {
		.allocationSize = mem_req.size,
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.memoryTypeIndex = findMemoryType(vk, mem_req.memoryTypeBits, properties)
	};

	vkAllocateMemory(vk->device, &allocInfo, NULL, &texture->texture_image_memory);

	vkBindImageMemory(vk->device, texture->texture_image, texture->texture_image_memory, 0);
}


void create_texture_buffers(VkInfo* vk, Scene* scene)
{
	for(uint32_t i = 0;i<scene->texture_data.num_textures;i++)
	{
		Texture* con = &scene->texture_data.textures[i];
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(vk, con->image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer, &stagingBufferMemory);

		void* data;
		vkMapMemory(vk->device, stagingBufferMemory, 0, con->image_size, 0, &data);
		memcpy(data, con->pixel_data, con->image_size);
		vkUnmapMemory(vk->device, stagingBufferMemory);

		// TODO free(pixel_data)

		create_texture_image(vk, con, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	}
}

void init_texture_descriptor(VkInfo* vk, Scene* scene)
{
	VkDescriptorImageInfo* infos = malloc(sizeof(VkDescriptorImageInfo) * scene->texture_data.num_textures);
	for(uint32_t i = 0;i < scene->texture_data.num_textures; i++)
	{
		infos[i].sampler = NULL;
		infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		infos[i].imageView = demoData.textures[i].view;
	}
}

