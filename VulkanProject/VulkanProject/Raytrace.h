#include "Globals.h"
#pragma once

typedef struct tlas {
	VkAccelerationStructureKHR structure;
} TLAS;
typedef struct blas {
	VkAccelerationStructureBuildRangeInfoKHR offset;
	VkAccelerationStructureGeometryKHR geometry;
} BLAS;
void build_acceleration_structures(VkInfo* info, Scene* scene);
TLAS build_acceleration_structure_for_node(VkInfo* info, Scene* scene, SceneNode* node);
BLAS build_blas(VkInfo* info, Scene* scene, SceneNode* node);