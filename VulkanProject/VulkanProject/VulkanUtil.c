
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "Globals.h"
#include "Util.h"
#include "VulkanUtil.h"

uint32_t findMemoryType(VkInfo* vk, uint32_t type_filter, VkMemoryPropertyFlags properties) {

	for (uint32_t i = 0; i < vk->physical_device_memory_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && (vk->physical_device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return -1;
}
// see https://vulkan-tutorial.com/ &  https://github.com/MomentsInGraphics/vulkan_renderer, was combined to fit my needs
void createBuffer(VkInfo* vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
	VkBufferCreateInfo bufferInfo = { 0 };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	check(vkCreateBuffer(vk->device, &bufferInfo, NULL, buffer),
		"Failed to create buffer");

	VkMemoryAllocateFlags allocate_flags = 0;
	if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		allocate_flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vk->device, *buffer, &memRequirements);

	VkMemoryAllocateFlagsInfo allocation_flags = {
	.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
	.flags = allocate_flags,
	.deviceMask = 1,
	};

	VkMemoryAllocateInfo allocation_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = (allocate_flags != 0) ? &allocation_flags : NULL,
		.allocationSize = memRequirements.size
	};


	uint32_t memtype = findMemoryType(vk, memRequirements.memoryTypeBits, properties);
	if (memtype == (uint32_t)-1) error("Failed to find correct memory type");
	allocation_info.memoryTypeIndex = memtype;


	check(vkAllocateMemory(vk->device, &allocation_info, NULL, bufferMemory),
		"Failed to allocate buffer memory");

	check(vkBindBufferMemory(vk->device, *buffer, *bufferMemory, 0), "");
}

VkDeviceAddress getBufferDeviceAddress(VkInfo* info, VkBuffer buf)
{
	VkBufferDeviceAddressInfo addr_info = {
		.buffer = buf,
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO
	};
	return vkGetBufferDeviceAddress(info->device, &addr_info);
}

VkCommandBuffer beginSingleTimeCommands(VkInfo* vk) // see https://vulkan-tutorial.com/
{
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vk->command_pool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	check(vkAllocateCommandBuffers(vk->device, &allocInfo, &commandBuffer), "");

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "");

	return commandBuffer;
}

void endSingleTimeCommands(VkInfo* vk, VkCommandBuffer commandBuffer){ // see https://vulkan-tutorial.com/
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	check(vkQueueSubmit(vk->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE), "");
	check(vkQueueWaitIdle(vk->graphics_queue) , "");

	vkFreeCommandBuffers(vk->device, vk->command_pool, 1, &commandBuffer);
}

void copyBuffer(VkInfo* vk, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) { // see https://vulkan-tutorial.com/
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(vk);

	VkBufferCopy copyRegion = {0};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(vk, commandBuffer);
}