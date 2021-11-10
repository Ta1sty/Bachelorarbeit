
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
int createBuffer(VkInfo* vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
	VkBufferCreateInfo bufferInfo = { 0 };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(vk->device, &bufferInfo, NULL, buffer))
		return err("Failed to create buffer");
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vk->device, *buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = { 0 };
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

VkCommandBuffer beginSingleTimeCommands(VkInfo* vk)
{
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vk->command_pool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vk->device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void endSingleTimeCommands(VkInfo* vk, VkCommandBuffer commandBuffer){
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vk->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk->graphics_queue);

	vkFreeCommandBuffers(vk->device, vk->command_pool, 1, &commandBuffer);
}

void copyBuffer(VkInfo* vk, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(vk);

	VkBufferCopy copyRegion = {0};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(vk, commandBuffer);
}