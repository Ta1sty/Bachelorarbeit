#include "Textures.h"

#include <stdlib.h>
#include <string.h>

#include "VulkanUtil.h"

void create_texture_descriptors(VkInfo* vk, Scene* scene, uint32_t samplerBinding, uint32_t textureBinding)
{
	VkDescriptorSetLayoutBinding sampler_binding = {
	.binding = samplerBinding,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
	.pImmutableSamplers = NULL,
	.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	
	VkDescriptorSetLayoutBinding texture_binding = {
	.binding = textureBinding,
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

	check(vkCreateDescriptorSetLayout(vk->device, &layout_create_info, NULL, &vk->texture_container.layout), "");
	vk->texture_container.sampler_binding = samplerBinding;
	vk->texture_container.texture_binding = textureBinding;
}

void create_image(VkInfo* vk, Texture* texture, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
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

	check(vkCreateImage(vk->device, &imageInfo, NULL, &texture->texture_image), "");

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(vk->device, texture->texture_image, &mem_req);

	VkMemoryAllocateInfo allocInfo = {
		.allocationSize = mem_req.size,
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.memoryTypeIndex = findMemoryType(vk, mem_req.memoryTypeBits, properties)
	};

	check(vkAllocateMemory(vk->device, &allocInfo, NULL, &texture->texture_image_memory),"failed to allocate");

	check(vkBindImageMemory(vk->device, texture->texture_image, texture->texture_image_memory, 0), "failed to bind image memory");

}

void transitionImageLayout(VkInfo* vk, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(vk);

	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		error("Invalid layout transition");
		return;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, NULL,
		0, NULL,
		1, &barrier
	);

	endSingleTimeCommands(vk, commandBuffer);
}

void copyBufferToImage(VkInfo* vk, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(vk);

	VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(vk, commandBuffer);
}

void create_texture_image(VkInfo* vk, Texture* texture)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(vk, texture->image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	void* data;
	check(vkMapMemory(vk->device, stagingBufferMemory, 0, texture->image_size, 0, &data), "");
	memcpy(data, texture->pixel_data, texture->image_size);
	vkUnmapMemory(vk->device, stagingBufferMemory);

	// TODO free(pixel_data)

	create_image(vk, texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	transitionImageLayout(vk, texture->texture_image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(vk, stagingBuffer, texture->texture_image, texture->image_width, texture->image_height);
	transitionImageLayout(vk, texture->texture_image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(vk->device, stagingBuffer, NULL);
	vkFreeMemory(vk->device, stagingBufferMemory, NULL);
}

void create_texture_image_view(VkInfo* vk, Texture* texture)
{
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = texture->texture_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	check(vkCreateImageView(vk->device, &viewInfo, NULL, &texture->texture_image_view), "failed to create texture image view");
}

void create_texture_sampler(VkInfo* vk, Scene* scene)
{
	VkPhysicalDeviceProperties properties = { 0 };
	vkGetPhysicalDeviceProperties(vk->physical_device, &properties);

	VkSamplerCreateInfo samplerInfo = {0};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	check(vkCreateSampler(vk->device, &samplerInfo, NULL, &scene->sampler), "failed to create sampler");
}

void create_texture_buffers(VkInfo* vk, Scene* scene)
{
	create_texture_sampler(vk, scene);
	for(uint32_t i = 0;i<scene->texture_data.num_textures;i++)
	{
		Texture* tex = &scene->texture_data.textures[i];

		create_texture_image(vk, tex);
		create_texture_image_view(vk, tex);
	}
}

void init_texture_descriptor(VkInfo* vk, Scene* scene)
{
	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk->descriptor_pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &vk->texture_container.layout;

	check(vkAllocateDescriptorSets(vk->device, &allocInfo, &vk->texture_container.descriptor_set), "Failed to allocate descriptor sets");

	VkDescriptorImageInfo samplerInfo = {0};
	samplerInfo.sampler = scene->sampler;

	VkWriteDescriptorSet samplerWrite = {0};
	
	samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	samplerWrite.dstSet = vk->texture_container.descriptor_set;
	samplerWrite.dstBinding = vk->texture_container.sampler_binding;
	samplerWrite.dstArrayElement = 0;
	samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerWrite.descriptorCount = 1;
	samplerWrite.pBufferInfo = NULL;
	samplerWrite.pImageInfo = &samplerInfo;
	
	vkUpdateDescriptorSets(vk->device, 1, &samplerWrite, 0, NULL);

	//TODO replace with for

	VkDescriptorImageInfo imageInfo1 = {0};
	imageInfo1.sampler = NULL;
	imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo1.imageView = scene->texture_data.textures[0].texture_image_view;

	VkDescriptorImageInfo imageInfo2 = { 0 };
	imageInfo2.sampler = NULL;
	imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo2.imageView = scene->texture_data.textures[1].texture_image_view;

	VkDescriptorImageInfo infos[] = { imageInfo1, imageInfo2 };

	VkWriteDescriptorSet textureWrite = {0};
	textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	textureWrite.dstBinding = vk->texture_container.texture_binding;
	textureWrite.dstArrayElement = 0;
	textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	textureWrite.descriptorCount = scene->texture_data.num_textures;
	textureWrite.pBufferInfo = NULL;
	textureWrite.dstSet = vk->texture_container.descriptor_set;
	textureWrite.pImageInfo = infos;

	vkUpdateDescriptorSets(vk->device, 1, &textureWrite, 0, NULL);
}

