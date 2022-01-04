#include "Raytrace.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

#include "VulkanUtil.h"
#include "Util.h"
#include <vulkan/vulkan_core.h>
// see https://github.com/MomentsInGraphics/vulkan_renderer, this macro creates a function pointer for dynamic function handles of vulkan
#define VK_LOAD(FUNCTION_NAME) PFN_##FUNCTION_NAME p##FUNCTION_NAME = (PFN_##FUNCTION_NAME) glfwGetInstanceProcAddress(info->instance, #FUNCTION_NAME);

void prepare_scene(Scene* scene, VkBool32 useMultiLevel)
{
	if (useMultiLevel == VK_TRUE)
	{
		GET_ROOT(scene);
		root->Level = 0;
		depth_recursion(scene, root);
		for (uint32_t i = 0; i < scene->scene_data.numSceneNodes; i++)
		{
			SceneNode* node = &scene->scene_nodes[i];
			node->NumEven = 0;
			node->NumOdd = 0;
			uint32_t evenOffset = 0;
			uint32_t oddOffset = node->NumChildren - 1;
			uint32_t* buffer = malloc(sizeof(uint32_t) * node->NumChildren);
			for (int32_t j = 0; j < node->NumChildren; j++)
			{
				GET_CHILD_IDX(scene, node, j);
				GET_CHILD(scene, node, j);
				if (child->Level % 2 == 0)
				{
					buffer[evenOffset] = childIdx;
					evenOffset++;
					node->NumEven++;
				}
				else
				{
					buffer[oddOffset] = childIdx;
					oddOffset--;
					node->NumOdd++;
				}
			}
			for (int32_t j = 0; j < node->NumChildren; j++)
			{
				scene->node_indices[node->ChildrenIndex + j] = buffer[j];
			}
			free(buffer);
		}
	}
	else
	{
		collapse_parent_nodes(scene);
		// write the levels. 0 for top, 1 for everyone else
	}
}

// this method writes the depth levels for every scene node
void depth_recursion(Scene* scene, SceneNode* node)
{
	for (int32_t i = 0; i < node->NumChildren; i++)
	{
		GET_CHILD(scene, node, i);
		child->Level = max(node->Level + 1, child->Level);
		depth_recursion(scene, child);
	}
}

void build_all_acceleration_structures(VkInfo* info, Scene* scene)
{
	scene->TLASs = malloc(sizeof(VkAccelerationStructureKHR) * scene->scene_data.numSceneNodes);
	scene->numTLAS = 0;
	GET_ROOT(scene);
	build_node_acceleration_structure(info, scene, root);
}

void build_node_acceleration_structure(VkInfo* info, Scene* scene, SceneNode* node)
{
	//TLAS tlas; the TLAS for this SceneNode, it is set if this node has an even level
	//BLAS blas; the BLAS for this SceneNode, it is set if this node has an odd level or if this node has an even level but it contains geometry

	// check if they were already built
	if (scene->acceleration_structures[node->Index].tlas.structure != NULL
		|| scene->acceleration_structures[node->Index].blas.structure != NULL)
		return;

	// if not we build all child ASs
	for (int i = 0; i < node->NumChildren; i++)
	{
		GET_CHILD(scene, node, i);
		build_node_acceleration_structure(info, scene, child);
	}
	// and then we combine them
	if (node->Level % 2 == 0) // Even level = TLAS
	{
		// if this node references geometry, we extract it into its own BLAS and also every child with even level
		build_tlas(info, scene, node);
	}
	else // Odd level = BLAS with AABBs when referencing children
	{
		build_blas(info, scene, node);
	}
}

// for the basic creation of acceleration structures you may also
// see https://github.com/MomentsInGraphics/vulkan_renderer
void build_tlas(VkInfo* info, Scene* scene, SceneNode* node)
{
	// credits to christopher
	VK_LOAD(vkGetAccelerationStructureBuildSizesKHR)
	VK_LOAD(vkCreateAccelerationStructureKHR)
	VK_LOAD(vkGetAccelerationStructureDeviceAddressKHR)
	VK_LOAD(vkCmdBuildAccelerationStructuresKHR)

	// 2 things to do
	// if node references geometry calls build BLAS for just the geometry
	// if node references children call buildBLAS for every child node

	uint32_t instance_count = node->NumOdd; // regular odd level BLAS children
	if (node->NumEven > 0) instance_count++; // even level children get put into one node
	if (node->NumTriangles > 0) instance_count++; // geometry also gets its own child
	uint32_t instanceOffset = 0;

	VkBuffer stagingBuffer = 0;
	VkDeviceMemory stagingMemory = 0;
	VkAccelerationStructureInstanceKHR* staging_data;
	createBuffer(info, sizeof(VkAccelerationStructureInstanceKHR) * instance_count,
				 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
				 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &stagingBuffer, &stagingMemory);

	check(vkMapMemory(info->device, stagingMemory, 0, sizeof(VkAccelerationStructureInstanceKHR) * instance_count, 0,
					  (void**) & staging_data), "");


	node->AABB_min[0] = -FLT_MAX;
	node->AABB_min[1] = -FLT_MAX;
	node->AABB_min[2] = -FLT_MAX;
	node->AABB_max[0] = FLT_MAX;
	node->AABB_max[1] = FLT_MAX;
	node->AABB_max[2] = FLT_MAX;

	SceneNode* children = malloc(sizeof(SceneNode) * instance_count);
	if (node->NumEven > 0 || node->NumTriangles > 0) 
	// handle the even children and geometry
	{
		SceneNode dummyChild = {
				.NumChildren = (int32_t) node->NumEven,
				.ChildrenIndex = node->ChildrenIndex,
				.NumTriangles = node->NumTriangles,
				.IndexBufferIndex = node->IndexBufferIndex,
				.Index = node->Index,
				.object_to_world = {
					{1, 0, 0, 0},
					{0, 1, 0, 0},
					{0, 0, 1, 0},
					{0, 0, 0, 1}
				},
			.world_to_object = {
					{1, 0, 0, 0},
					{0, 1, 0, 0},
					{0, 0, 1, 0},
					{0, 0, 0, 1}
				},
			.Level = node->Level + 1,
			.NumEven = node->NumEven,
			.NumOdd = 0,
		};
		build_blas(info, scene, &dummyChild);
		children[instanceOffset] = dummyChild;
		instanceOffset++;
	}

	if (node->NumOdd > 0) // handle the normal case for odd level nodes
	{
		for (uint32_t i = 0; i < node->NumOdd; i++)
		{
			GET_CHILD(scene, node, node->NumEven + i);
			// since this is depth first the BLAS should already be built
			children[instanceOffset + i] = *child;
		}
	}

	// now build the instance geometry

	for (uint32_t i = 0; i < instance_count; i++)
	{
		SceneNode child = children[i];
		// Specify the only instance
		VkAccelerationStructureDeviceAddressInfoKHR address_request = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
			.accelerationStructure = scene->acceleration_structures[child.Index].blas.structure,
		};
		VkAccelerationStructureInstanceKHR instance = {
			.transform = {
				.matrix = {
					{1.0f, 0.0f, 0.0f, 0.0f},
					{0.0f, 1.0f, 0.0f, 0.0f},
					{0.0f, 0.0f, 1.0f, 0.0f},
				}
			},
			.mask = 0xFF,
			.flags = VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR |
			VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
			.accelerationStructureReference = pvkGetAccelerationStructureDeviceAddressKHR(
				info->device, &address_request),
			.instanceCustomIndex = child.Index >= 0 ? child.Index : 0xFFFFFFFF
			// this is the reference to use in case this is an odd level node
		};

		memcpy(&instance.transform.matrix, &child.object_to_world, sizeof(float) * 4 * 3);/*instance.transform.matrix[0][0] = 1;
		instance.transform.matrix[1][0] = 0;
		instance.transform.matrix[2][0] = 0;
		instance.transform.matrix[0][1] = 0;
		instance.transform.matrix[1][1] = 1;
		instance.transform.matrix[2][1] = 0;
		instance.transform.matrix[0][2] = 0;
		instance.transform.matrix[1][2] = 0;
		instance.transform.matrix[2][2] = 1;*/


		float maxNew[3] = { 0 };
		float minNew[3] = { 0 };
		transformAABB(instance.transform.matrix, child.AABB_min, child.AABB_max, minNew, maxNew);

		node->AABB_min[0] = min(node->AABB_min[0], minNew[0]);
		node->AABB_min[1] = min(node->AABB_min[1], minNew[1]);
		node->AABB_min[2] = min(node->AABB_min[2], minNew[2]);
		node->AABB_max[0] = max(node->AABB_max[0], maxNew[0]);
		node->AABB_max[1] = max(node->AABB_max[1], maxNew[1]);
		node->AABB_max[2] = max(node->AABB_max[2], maxNew[2]);

		memcpy(&staging_data[i], &instance, sizeof(VkAccelerationStructureInstanceKHR));
	}

	VkAccelerationStructureBuildSizesInfoKHR top_sizes = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	};
	VkBufferDeviceAddressInfo instances_address = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = stagingBuffer
	};
	VkAccelerationStructureGeometryKHR top_geometry = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.geometry = {
			.instances = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
				.arrayOfPointers = VK_FALSE,
				.data = {.deviceAddress = vkGetBufferDeviceAddress(info->device, &instances_address)},
			},
		},
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};
	VkAccelerationStructureBuildGeometryInfoKHR top_build_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.geometryCount = 1, .pGeometries = &top_geometry,
	};
	pvkGetAccelerationStructureBuildSizesKHR(
		info->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&top_build_info, &instance_count, &top_sizes);

	// Create buffers for the acceleration structures

	VkBuffer tlasBuffer;
	VkDeviceMemory tlasMemory;

	createBuffer(info, top_sizes.accelerationStructureSize,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tlasBuffer, &tlasMemory);

	VkAccelerationStructureKHR structure;

	VkAccelerationStructureCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = tlasBuffer,
		.offset = 0, .size = top_sizes.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	};
	check(pvkCreateAccelerationStructureKHR(info->device, &create_info, NULL, &structure), "");

	VkBuffer scratchBuffer;
	VkDeviceMemory scratchMemory;

	createBuffer(info, top_sizes.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scratchBuffer, &scratchMemory);

	// Get ready to record commands
	VkCommandBuffer cmd = beginSingleTimeCommands(info);
	// Build bottom- and top-level acceleration structures in this order
	VkAccelerationStructureBuildRangeInfoKHR build_ranges[] = {
		{.primitiveCount = instance_count}
	};
	VkAccelerationStructureBuildGeometryInfoKHR build_info = top_build_info;
	VkBufferDeviceAddressInfo scratch_adress_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = scratchBuffer
	};
	build_info.scratchData.deviceAddress = vkGetBufferDeviceAddress(info->device, &scratch_adress_info);
	build_info.dstAccelerationStructure = structure;
	const VkAccelerationStructureBuildRangeInfoKHR* build_range = &build_ranges[0];
	pvkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, &build_range);
	// Enforce synchronization
	VkMemoryBarrier after_build_barrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
	};
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0,
		1, &after_build_barrier, 0, NULL, 0, NULL);
	// Submit the command buffer

	endSingleTimeCommands(info, cmd);

	AccelerationStructure acceleration_structure = {
		.structure = structure,
		.buffer = tlasBuffer,
		.memory = tlasMemory
	};
	scene->acceleration_structures[node->Index].tlas = acceleration_structure;
	expandAABB(node);
	scene->TLASs[scene->numTLAS] = acceleration_structure.structure;
	node->TlasNumber = scene->numTLAS;
	scene->numTLAS++;
}

// for the basic creation of acceleration structures you may also
// see https://github.com/MomentsInGraphics/vulkan_renderer
void build_blas(VkInfo* info, Scene* scene, SceneNode* node)
{
	// 2 things to do
	// if node references geometry add relevant geometry
	// if node references children recursively create new TLAS
	// and add traversalNodes

	// credits to christopher
	VK_LOAD(vkGetAccelerationStructureBuildSizesKHR)
	VK_LOAD(vkCreateAccelerationStructureKHR)
	VK_LOAD(vkGetAccelerationStructureDeviceAddressKHR)
	VK_LOAD(vkCmdBuildAccelerationStructuresKHR)

		uint32_t primitive_count = 0;
	if (node->NumChildren > 0) primitive_count += node->NumChildren;
	if (node->NumTriangles > 0) primitive_count += node->NumTriangles;
	uint32_t geometryNum = 0;
	VkAccelerationStructureGeometryKHR geometries[2] = { 0 };


	node->AABB_min[0] = -FLT_MAX;
	node->AABB_min[1] = -FLT_MAX;
	node->AABB_min[2] = -FLT_MAX;
	node->AABB_max[0] = FLT_MAX;
	node->AABB_max[1] = FLT_MAX;
	node->AABB_max[2] = FLT_MAX;

	VkBuffer aabbBuffer = 0;
	VkDeviceMemory aabbMemory = 0;

	// Handle node children
	if (node->NumChildren > 0)
	{
		for (int32_t i = 0; i < node->NumChildren; i++)
		{
			GET_CHILD(scene, node, i);
			if (child->Level % 2 == 1 && scene->acceleration_structures[child->Index].tlas.structure == NULL) // child is odd, we need a TLAS for it
			{
				if (scene->acceleration_structures[child->Index].blas.structure == NULL) error("Odd child does not have a BLAS??");
				SceneNode dummyNode = {
						.NumChildren = 1,
						.ChildrenIndex = node->ChildrenIndex + i, // this is the node we want to create a TLAS for
						.NumTriangles = 0,
						.Index = child->Index,
						.IndexBufferIndex = -1,
						.object_to_world = {
							{1, 0, 0, 0},
							{0, 1, 0, 0},
							{0, 0, 1, 0},
							{0, 0, 0, 1}
						},
					.world_to_object = {
					{1, 0, 0, 0},
					{0, 1, 0, 0},
					{0, 0, 1, 0},
					{0, 0, 0, 1}
					},
					.Level = node->Level + 1,
					.NumOdd = 1,
					.NumEven = 0,
				};
				build_tlas(info, scene, &dummyNode);
				child->TlasNumber = dummyNode.TlasNumber;
			}
		}

		VkAabbPositionsKHR* aabbData;
		createBuffer(info, sizeof(VkAabbPositionsKHR) * node->NumChildren,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &aabbBuffer, &aabbMemory);
		check(vkMapMemory(info->device, aabbMemory, 0, sizeof(VkAabbPositionsKHR) * node->NumChildren, 0,
			(void**) & aabbData), "error Mapping memory");

		for (int32_t i = 0; i < node->NumChildren; i++)
		{
			GET_CHILD(scene, node, i);
			float maxNew[3] = { 1,1,1 };
			float minNew[3] = { -1,-1 ,-1 };
			float transform[4][4] = {
							{1, 0, 0, 0},
							{0, 1, 0, 0},
							{0, 0, 1, 0},
							{0, 0, 0, 1}
			};
			if (child->Level % 2 == 0)
			{
				memcpy(&transform, &child->object_to_world, sizeof(float) * 4 * 4);
			}
			transformAABB(transform, child->AABB_min, child->AABB_max, minNew, maxNew);

			VkAabbPositionsKHR position = {
				.maxX = maxNew[0],
				.maxY = maxNew[1],
				.maxZ = maxNew[2],
				.minX = minNew[0],
				.minY = minNew[1],
				.minZ = minNew[2]
			};

			node->AABB_min[0] = min(node->AABB_min[0], minNew[0]);
			node->AABB_min[1] = min(node->AABB_min[1], minNew[1]);
			node->AABB_min[2] = min(node->AABB_min[2], minNew[2]);
			node->AABB_max[0] = max(node->AABB_max[0], maxNew[0]);
			node->AABB_max[1] = max(node->AABB_max[1], maxNew[1]);
			node->AABB_max[2] = max(node->AABB_max[2], maxNew[2]);

			memcpy(&aabbData[i], &position, sizeof(VkAabbPositionsKHR));
		}


		// now every child we reference has a TLAS, if the child is odd we then just use the identity transform
		// Now build AABB geometry, remember to NOT transform the ray if the child was odd

		VkBufferDeviceAddressInfo aabb_address = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = aabbBuffer
		};

		VkAccelerationStructureGeometryKHR bottom_geometry = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
			.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR,
			.geometry = {
				.aabbs = {
					.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
					.data = {vkGetBufferDeviceAddress(info->device, &aabb_address)},
					.stride = sizeof(VkAabbPositionsKHR)
				}
			},
			.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
		};
		geometries[geometryNum] = bottom_geometry;
		geometryNum++;
	}

	// Handle node geometry
	VkBuffer vertexStage = 0;
	VkDeviceMemory vertexStageMemeory = 0;
	VkBuffer indexStage = 0;
	VkDeviceMemory indexStageMemeory = 0;
	if (node->NumTriangles > 0)
	{
		void* index_data;
		createBuffer(info, sizeof(uint32_t) * node->NumTriangles * 3,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexStage, &indexStageMemeory);
		check(vkMapMemory(info->device, indexStageMemeory, 0, sizeof(uint32_t) * node->NumTriangles * 3, 0,
			&index_data), "error Mapping memory");

		uint32_t maxIndex = 0; // the maximum index used in the indexBuffer for this mesh
		uint32_t minIndex = UINT32_MAX; // the minimum index used in the indexBuffer for this mesh.
		uint32_t* indices = index_data;
		for (int32_t i = 0; i != node->NumTriangles * 3; ++i)
		{
			minIndex = min(minIndex, scene->indices[i + node->IndexBufferIndex]);
			maxIndex = max(maxIndex, scene->indices[i + node->IndexBufferIndex]);
		}

		for (int32_t i = 0; i != node->NumTriangles; ++i)
		{
			indices[i * 3 + 0] = scene->indices[i * 3 + 0 + node->IndexBufferIndex] - minIndex;
			// there should always be one where this is 0
			indices[i * 3 + 1] = scene->indices[i * 3 + 1 + node->IndexBufferIndex] - minIndex;
			// there should always be one where this is 0
			indices[i * 3 + 2] = scene->indices[i * 3 + 2 + node->IndexBufferIndex] - minIndex;
			// there should always be one where this is 0
		}
		// create vertex stage buffer
		uint32_t numVertices = maxIndex - minIndex + 1;
		void* vertex_data;
		createBuffer(info, numVertices * sizeof(float) * 3,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexStage, &vertexStageMemeory);
		check(vkMapMemory(info->device, vertexStageMemeory, 0, numVertices * sizeof(float) * 3, 0,
			&vertex_data), "failed to map memory");
		float* vertices = vertex_data;
		for (uint32_t i = 0; i != numVertices; ++i)
		{
			for (uint32_t j = 0; j < 3; j++)
			{
				vertices[i * 3 + j] = scene->vertices[i + minIndex].position[j];
				node->AABB_min[j] = min(node->AABB_min[j], vertices[i * 3 + j]);
				node->AABB_max[j] = max(node->AABB_max[j], vertices[i * 3 + j]);
			}
		}

		// naive vertex array

		/*VkBuffer vertexStage;
		VkDeviceMemory vertexStageMemeory;
		void* vertex_data;
		createBuffer(info, node->NumTriangles * sizeof(float) * 3,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexStage, &vertexStageMemeory);
		check(vkMapMemory(info->device, vertexStageMemeory, 0, node->NumTriangles * sizeof(float) * 3, 0,
			&vertex_data), "error Mapping memeory");

		float* vertices = vertex_data;
		for (uint32_t i = 0; i != node->NumTriangles * 3 * 3; i += 3)
		{
			Vertex v = scene->vertices[scene->indices[i / 3 + node->IndexBufferIndex]];
			vertices[i + 0] = v.position[0];
			vertices[i + 1] = v.position[1];
			vertices[i + 2] = v.position[2];
		}*/


		// Figure out how big the buffers for the bottom-level need to be
		VkBufferDeviceAddressInfo vertices_address = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = vertexStage
		};

		VkBufferDeviceAddressInfo index_address = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = indexStage
		};
		VkAccelerationStructureGeometryKHR bottom_geometry = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
			.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
			.geometry = {
				.triangles = {
					.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
					.vertexData = {.deviceAddress = vkGetBufferDeviceAddress(info->device, &vertices_address)},
					.maxVertex = primitive_count * 3 - 1,
					.vertexStride = 3 * sizeof(float),
					.indexType = VK_INDEX_TYPE_UINT32,
					.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
					.indexData = {.deviceAddress = vkGetBufferDeviceAddress(info->device, &index_address)},
				},
			},
			.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
		};
		geometries[geometryNum] = bottom_geometry;
		geometryNum++;
	}
		VkAccelerationStructureBuildSizesInfoKHR bottom_size = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
		};

		VkAccelerationStructureBuildGeometryInfoKHR bottom_build_info = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
			.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
			.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
			.geometryCount = geometryNum, .pGeometries = geometries,
		};
		pvkGetAccelerationStructureBuildSizesKHR(
			info->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&bottom_build_info, &primitive_count, &bottom_size);

		// Create buffers for the acceleration structures

		VkBuffer blasBuffer = 0;
		VkDeviceMemory blasMemory = 0;
		createBuffer(info, bottom_size.accelerationStructureSize,
			VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &blasBuffer, &blasMemory);

		// Create the acceleration structures
		VkAccelerationStructureCreateInfoKHR create_info = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
			.buffer = blasBuffer,
			.offset = 0, .size = bottom_size.accelerationStructureSize,
			.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		};

		VkAccelerationStructureKHR structure;

		check(pvkCreateAccelerationStructureKHR(info->device, &create_info, NULL, &structure), "");


		// Allocate scratch memory for the build
		VkBuffer scratchBuffer = 0;
		VkDeviceMemory scratchBufferMemeory = 0;

		createBuffer(info, bottom_size.buildScratchSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, &scratchBuffer, &scratchBufferMemeory);

		// Specify the only instance
		VkCommandBuffer cmd = beginSingleTimeCommands(info);
		// Build bottom- and top-level acceleration structures in this order
		VkAccelerationStructureBuildRangeInfoKHR build_ranges[] = {
			{.primitiveCount = primitive_count}
		};
		VkBufferDeviceAddressInfo scratch_adress_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = scratchBuffer
		};
		bottom_build_info.scratchData.deviceAddress = vkGetBufferDeviceAddress(info->device, &scratch_adress_info);
		bottom_build_info.dstAccelerationStructure = structure;
		const VkAccelerationStructureBuildRangeInfoKHR* build_range = &build_ranges[0];
		pvkCmdBuildAccelerationStructuresKHR(cmd, 1, &bottom_build_info, &build_range);
		// Enforce synchronization
		VkMemoryBarrier after_build_barrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
			VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
			.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
			VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		};
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0,
			1, &after_build_barrier, 0, NULL, 0, NULL);

		endSingleTimeCommands(info, cmd);

		vkDestroyBuffer(info->device, vertexStage, NULL);
		vkDestroyBuffer(info->device, indexStage, NULL);
		vkDestroyBuffer(info->device, scratchBuffer, NULL);
		vkFreeMemory(info->device, vertexStageMemeory, NULL);
		vkFreeMemory(info->device, indexStageMemeory, NULL);
		vkFreeMemory(info->device, scratchBufferMemeory, NULL);

		AccelerationStructure acceleration_structure = {
			.buffer = blasBuffer,
			.memory = blasMemory,
			.structure = structure
		};
		scene->acceleration_structures[node->Index].blas = acceleration_structure;
	expandAABB(node);
}

void expandAABB(SceneNode* node)
{
	float inc = 0.02f;
	node->AABB_min[0] -= inc;
	node->AABB_min[1] -= inc;
	node->AABB_min[2] -= inc;
	node->AABB_max[0] += inc;
	node->AABB_max[1] += inc;
	node->AABB_max[2] += inc;
}

void transformAABB(float transform[4][4], float min[3], float max[3], float* minNew, float* maxNew)
{
	float AABBVertices[8][3] = {
		{min[0], min[1], min[2]},
		{min[0], min[1], max[2]},
		{min[0], max[1], min[2]},
		{min[0], max[1], max[2]},
		{max[0], min[1], min[2]},
		{max[0], min[1], max[2]},
		{max[0], max[1], min[2]},
		{max[0], max[1], max[2]},
	};

	max[0] = -FLT_MAX;
	max[1] = -FLT_MAX;
	max[2] = -FLT_MAX;
	min[0] = FLT_MAX;
	min[1] = FLT_MAX;
	min[2] = FLT_MAX;

	for (uint32_t i = 0; i < 8; i++)
	{
		AABBVertices[i][0] = transform[0][0] * AABBVertices[i][0] + transform[0][1] * AABBVertices[i][1] +
			transform[0][2] * AABBVertices[i][2] + transform[0][3];
		AABBVertices[i][1] = transform[1][0] * AABBVertices[i][0] + transform[1][1] * AABBVertices[i][1] +
			transform[1][2] * AABBVertices[i][2] + transform[1][3];
		AABBVertices[i][2] = transform[2][0] * AABBVertices[i][0] + transform[2][1] * AABBVertices[i][1] +
			transform[2][2] * AABBVertices[i][2] + transform[2][3];
	}

	for (uint32_t i = 0; i < 8; i++)
	{
		for (uint32_t j = 0; j < 3; j++)
		{
			max[j] = max(max[j], AABBVertices[i][j]);
			min[j] = min(min[j], AABBVertices[i][j]);
		}
	}

	memcpy(minNew, min, sizeof(float) * 3);
	memcpy(maxNew, max, sizeof(float) * 3);
}

void create_ray_descriptors(VkInfo* info, Scene* scene, uint32_t binding)
{
	VkDescriptorSetLayoutBinding layout_binding = {
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		.descriptorCount = scene->numTLAS,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	VkDescriptorSetLayoutCreateInfo layout_create_info = { 0 };
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = 1;
	layout_create_info.pBindings = &layout_binding;

	check(vkCreateDescriptorSetLayout(info->device, &layout_create_info, NULL, &info->ray_descriptor.set_layout), "");
	info->ray_descriptor.binding = binding;
}

void init_ray_descriptors(VkInfo* info, Scene* scene)
{
	VkDescriptorSetAllocateInfo allocInfo = { 0 };
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = info->descriptor_pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &info->ray_descriptor.set_layout;

	check(vkAllocateDescriptorSets(info->device, &allocInfo, &info->ray_descriptor.descriptor_set), "");

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAS = {
		.accelerationStructureCount = scene->numTLAS,
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.pAccelerationStructures = scene->TLASs,
		.pNext = NULL
	};
	VkWriteDescriptorSet write = {
		.pNext = &descriptorAS,
		.dstBinding = info->ray_descriptor.binding,
		.descriptorCount = scene->numTLAS,
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = info->ray_descriptor.descriptor_set,
		.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
	};
	vkUpdateDescriptorSets(info->device, 1, &write, 0, NULL);
}
