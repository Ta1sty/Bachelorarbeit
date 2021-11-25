#include "Raytrace.h"

#include <stdlib.h>

#include "VulkanUtil.h"

#include <vulkan/vulkan_core.h>

#define VK_LOAD(FUNCTION_NAME) PFN_##FUNCTION_NAME p##FUNCTION_NAME = (PFN_##FUNCTION_NAME) glfwGetInstanceProcAddress(info->instance, #FUNCTION_NAME);


void build_acceleration_structures(VkInfo* info, Scene* scene)
{
	VkBool32 useMultiLevel = VK_FALSE;

	if (useMultiLevel == VK_TRUE) 
		return;
	else {
		collapse_parent_nodes(scene);
		build_acceleration_structure_for_node(info, scene, &scene->scene_nodes[scene->scene_data.numSceneNodes - 1]);
	}
}

TLAS build_acceleration_structure_for_node(VkInfo* info, Scene* scene, SceneNode* node)
{
	BLAS* blases = malloc(sizeof(BLAS) * node->NumChildren);
	for (int i = 0; i < node->NumChildren; i++) {
		SceneNode* child = &scene->scene_nodes[scene->node_indices[node->childrenIndex + i]];
		BLAS child_blas = build_blas(info, scene, child);
	}
}
BLAS build_blas(VkInfo* info, Scene* scene, SceneNode* node) {
	VkDeviceAddress vertexAddr = getBufferDeviceAddress(info, info->global_buffers.buffer_containers[0].buffers[1].vk_buffer);
	VkDeviceAddress indexAddr = getBufferDeviceAddress(info, info->global_buffers.buffer_containers[0].buffers[2].vk_buffer);

	VK_LOAD(vkGetAccelerationStructureBuildSizesKHR)
	VK_LOAD(vkCreateAccelerationStructureKHR)
	VK_LOAD(vkGetAccelerationStructureDeviceAddressKHR)
	VK_LOAD(vkCmdBuildAccelerationStructuresKHR)

	VkAccelerationStructureGeometryTrianglesDataKHR triangles = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
		.vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT,
		.vertexData.deviceAddress = vertexAddr,
		.vertexStride = sizeof(Vertex),
		.maxVertex = node->NumTriangles - 1,
		.indexType = VK_INDEX_TYPE_UINT32,
		.indexData.deviceAddress = indexAddr,
		.transformData.deviceAddress = 0
	};
	VkAccelerationStructureGeometryKHR geometry = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.geometry.triangles = triangles,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	VkAccelerationStructureBuildRangeInfoKHR offset = {
		.primitiveOffset = node->IndexBufferIndex,
		.primitiveCount = node->NumTriangles,
		.transformOffset = 0,
		.firstVertex = 0
	};


	VkAccelerationStructureBuildGeometryInfoKHR build_info = {
	.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	.geometryCount = 1,
	.pGeometries = &geometry
	};
	VkAccelerationStructureBuildSizesInfoKHR bottom_size = { 
	.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
	};
	pvkGetAccelerationStructureBuildSizesKHR(info->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &node->NumTriangles, &bottom_size);

	VkBuffer scratchBuffer = 0;
	VkDeviceMemory scratchBufferMemeory = 0;

	createBuffer(info->device, bottom_size.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, &scratchBuffer, &scratchBufferMemeory);
	build_info.scratchData.deviceAddress = getBufferDeviceAddress(info, scratchBuffer);

	VkCommandBuffer cmd = beginSingleTimeCommands(info);

	VkBuffer blasBuffer = 0;
	VkDeviceMemory blasMemory = 0;
	createBuffer(info->device, bottom_size.accelerationStructureSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &blasBuffer, &blasMemory);
	VkAccelerationStructureKHR structrue = { 0 };
	VkAccelerationStructureCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = blasBuffer,
		.offset = 0, 
		.size = bottom_size.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	};
	check(pvkCreateAccelerationStructureKHR(info->device, &create_info, NULL, &structrue));
	BLAS b = { 0 };
	return b;
}
