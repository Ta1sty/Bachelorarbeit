#pragma once
#include "Globals.h"

typedef struct queryTrace {
	float matrix[4][4];
	float start[3];
	float t;
	float end[3];
	uint32_t nodeNumber;
	uint32_t isValid; // valid if 1, otherwise invalid
	int32_t nodeLevel; // the nodeNumber for this
	uint32_t triangleIntersections;
	uint32_t instanceIntersections;
} QueryTrace;

void create_ray_descriptors(VkInfo* info, Scene* scene, uint32_t tlassBinding, uint32_t traceBinding);
void create_trace_buffer(VkInfo* info, Scene* scene);
void init_ray_descriptors(VkInfo* info, Scene* scene);

void build_all_acceleration_structures(VkInfo* info, Scene* scene);
void build_node_acceleration_structure(VkInfo* info, Scene* scene, SceneNode* node);

void build_tlas_instance_list(VkInfo* info, Scene* scene, SceneNode* list);
void build_blas_instance_list(VkInfo* info, Scene* scene, SceneNode* list);

void build_tlas(VkInfo* info, Scene* scene, SceneNode* node);
void build_blas(VkInfo* info, Scene* scene, SceneNode* node);

void compile_query_trace(VkInfo* info, Scene* scene);

void destroyAccelerationStructures(VkInfo* info, Scene* scene);
