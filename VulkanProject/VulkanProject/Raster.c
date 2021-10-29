#include "Raster.h"

#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "Util.h"
#include "VulkanUtil.h"
uint32_t vertex_count = 3;

void initVertexArray()
{
	Vertex v1 = {
		.pos = {-1.0f, -3.0f},
		.color = {1.0f, 1.0f, 1.0f}
	};
	Vertex v2 = {
	.pos = {3.0f, 1.0f},
	.color = {0.0f, 1.0f, 0.0f}
	};
	Vertex v3 = {
		.pos = {-1.0f, 1.0f},
		.color = {0.0f, 0.0f, 1.0f}
	};
	vertices = malloc(sizeof(Vertex) * vertex_count);
	vertices[0] = v1;
	vertices[1] = v2;
	vertices[2] = v3;
}
void freeVertexArray()
{
	free(vertices);
}

VkVertexInputBindingDescription getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription;
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}


VkVertexInputAttributeDescription* getAttributeDescriptions() {
	VkVertexInputAttributeDescription* desc = malloc(2 * sizeof(VkVertexInputAttributeDescription));
	desc[0].binding = 0;
	desc[0].location = 0;
	desc[0].format = VK_FORMAT_R32G32_SFLOAT;
	desc[0].offset = offsetof(Vertex, pos);

	desc[1].binding = 0;
	desc[1].location = 1;
	desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	desc[1].offset = offsetof(Vertex, color);

	return desc;
}

int create_vertex_buffer(VkInfo* vk)
{
	if (vk->rasterize == VK_FALSE) return SUCCESS;
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(Vertex) * vertex_count;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(vk->device, &bufferInfo, NULL, &vk->vertexBuffer))
		return err("Failed to create vertex buffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vk->device, vk->vertexBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(vk, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(vk->device, &allocInfo, NULL, &vk->vertexBufferMemory)) return err("Failed to allocate vertex memory");

	vkBindBufferMemory(vk->device, vk->vertexBuffer, vk->vertexBufferMemory, 0);

	void* data;
	vkMapMemory(vk->device, vk->vertexBufferMemory, 0, bufferInfo.size, 0, &data);
	memcpy(data, vertices, bufferInfo.size);
	vkUnmapMemory(vk->device, vk->vertexBufferMemory);

	return SUCCESS;
}

