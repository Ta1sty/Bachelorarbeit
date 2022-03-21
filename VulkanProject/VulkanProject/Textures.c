#include "Textures.h"

#include <stdlib.h>
#include <string.h>

#include "Bindings.h"
#include "Util.h"
#include "VulkanUtil.h"

void create_texture_descriptors(VkInfo* vk, Scene* scene) // see https://vulkan-tutorial.com/
{
	VkDescriptorSetLayoutBinding sampler_binding = {
	.binding = SAMPLER_BINDING,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
	.pImmutableSamplers = NULL,
	.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	
	VkDescriptorSetLayoutBinding texture_binding = {
	.binding = TEXTURE_BINDING,
	.descriptorCount = scene->texture_data.num_textures,
	.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	.pImmutableSamplers = NULL,
	.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDescriptorSetLayoutBinding skybox_binding = {
		.binding = SKYBOX_BINDING,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImmutableSamplers = NULL,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDescriptorSetLayoutBinding bindings[] = { sampler_binding, texture_binding, skybox_binding};
	
	VkDescriptorSetLayoutCreateInfo layout_create_info = { 0 };
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = 3;
	layout_create_info.pBindings = bindings;

	check(vkCreateDescriptorSetLayout(vk->device, &layout_create_info, NULL, &vk->texture_container.layout), "");
	vk->texture_container.sampler_binding = SAMPLER_BINDING;
	vk->texture_container.texture_binding = TEXTURE_BINDING;
	vk->texture_container.skybox_binding = SKYBOX_BINDING;
}

void create_image(VkInfo* vk, Texture* texture, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) // see https://vulkan-tutorial.com/
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

void transitionImageLayout(VkInfo* vk, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount) { // see https://vulkan-tutorial.com/
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
	barrier.subresourceRange.layerCount = layerCount;

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

void copyBufferToImage(VkInfo* vk, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,uint32_t layerCount) { // see https://vulkan-tutorial.com/
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(vk);

	VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(vk, commandBuffer);
}

void create_texture_image(VkInfo* vk, Texture* texture) // see https://vulkan-tutorial.com/
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(vk, texture->image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	void* data;
	check(vkMapMemory(vk->device, stagingBufferMemory, 0, texture->image_size, 0, &data), "");
	memcpy(data, texture->pixel_data, texture->image_size);
	vkUnmapMemory(vk->device, stagingBufferMemory);

	create_image(vk, texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	transitionImageLayout(vk, texture->texture_image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1);
	copyBufferToImage(vk, stagingBuffer, texture->texture_image, texture->image_width, texture->image_height, 1);
	transitionImageLayout(vk, texture->texture_image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,1);

	vkDestroyBuffer(vk->device, stagingBuffer, NULL);
	vkFreeMemory(vk->device, stagingBufferMemory, NULL);
}

void create_texture_image_view(VkInfo* vk, Texture* texture) // see https://vulkan-tutorial.com/
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

void create_texture_sampler(VkInfo* vk, Scene* scene) // see https://vulkan-tutorial.com/
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
	create_skybox(vk, scene);
}

// https://satellitnorden.wordpress.com/2018/01/23/vulkan-adventures-cube-map-tutorial/
void create_skybox(VkInfo* vk, Scene* scene)
{
	FILE* file;
	fopen_s(&file, "shaders/skybox.cubetex", "rb");

	int32_t width = 1;
	int32_t height = 1;
	fread(&width, sizeof(int32_t), 1, file);
	fread(&height, sizeof(int32_t), 1, file);

	VkDeviceSize imageSize = width * height * 4 * 6;
	VkDeviceSize layerSize = imageSize / 6;

	uint32_t* textureData = malloc(imageSize);
	fread(textureData, 1, imageSize, file);
	fclose(file);

	/*uint32_t textureData2[] = {
	0x00000000,0x000000FF,
	0x0000FF00,0x0000FFFF,
	0x00FF0000,0x00FF00FF };

	textureData = textureData2;

	width = 1;
	height = 1;*/

	imageSize = width * height * 4 * 6;
	layerSize = imageSize / 6;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferDeviceMemory;

	createBuffer(vk, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferDeviceMemory);

	uint32_t* data;
	vkMapMemory(vk->device, stagingBufferDeviceMemory, 0, imageSize, 0, &data);

	for (uint32_t i = 0; i < 6; ++i)
	{
		memcpy(&data[width * height * i], &textureData[width * height * i], layerSize);
	}
	vkUnmapMemory(vk->device, stagingBufferDeviceMemory);

	VkImageCreateInfo imageInfo = { 0 };
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 6;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	check(vkCreateImage(vk->device, &imageInfo, NULL, &vk->skyboxImage), "");

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(vk->device, vk->skyboxImage, &mem_req);

	VkMemoryAllocateInfo allocInfo = {
		.allocationSize = mem_req.size,
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.memoryTypeIndex = findMemoryType(vk, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	check(vkAllocateMemory(vk->device, &allocInfo, NULL, &vk->skyboxMemory), "failed to allocate");

	check(vkBindImageMemory(vk->device, vk->skyboxImage, vk->skyboxMemory, 0), "failed to bind image memory");


	transitionImageLayout(vk, vk->skyboxImage, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);
	copyBufferToImage(vk, stagingBuffer, vk->skyboxImage, width, height, 6);
	transitionImageLayout(vk, vk->skyboxImage, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

	vkDestroyBuffer(vk->device, stagingBuffer, NULL);
	vkFreeMemory(vk->device, stagingBufferDeviceMemory, NULL);


	VkImageViewCreateInfo viewInfo = { 0 };
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = vk->skyboxImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	check(vkCreateImageView(vk->device, &viewInfo, NULL, &vk->skyboxView), "failed to create texture image view");

	VkPhysicalDeviceProperties properties = { 0 };
	vkGetPhysicalDeviceProperties(vk->physical_device, &properties);

	VkSamplerCreateInfo samplerInfo = { 0 };
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	check(vkCreateSampler(vk->device, &samplerInfo, NULL, &vk->skyboxSampler), "failed to create sampler");
}

// see https://vulkan-tutorial.com/
// notice that the tutorial uses only one texture, I use an array of textures (not to be confused with a textureArray)
// for an explanation see http://kylehalladay.com/blog/tutorial/vulkan/2018/01/28/Textue-Arrays-Vulkan.html
// the method was adjusted for an array of dynamic size on allocation
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

	VkDescriptorImageInfo* infos = malloc(sizeof(VkDescriptorImageInfo) * scene->texture_data.num_textures);

	for (uint32_t i = 0; i < scene->texture_data.num_textures; i++) {
		VkDescriptorImageInfo imageInfo = { 0 };
		imageInfo.sampler = NULL;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = scene->texture_data.textures[i].texture_image_view;

		infos[i] = imageInfo;
	}

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
	free(infos);

	VkDescriptorImageInfo skyboxInfo = {
		.sampler = vk->skyboxSampler,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.imageView = vk->skyboxView
	};

	VkWriteDescriptorSet skyboxWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstBinding = SKYBOX_BINDING,
		.dstArrayElement = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.pBufferInfo = NULL,
		.dstSet = vk->texture_container.descriptor_set,
		.pImageInfo = &skyboxInfo
	};

	vkUpdateDescriptorSets(vk->device, 1, &skyboxWrite, 0, NULL);
}

