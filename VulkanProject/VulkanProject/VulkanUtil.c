
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "Globals.h"
#include "Util.h"

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