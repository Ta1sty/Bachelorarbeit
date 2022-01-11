#pragma once
#include "Globals.h"

typedef struct queryTrace {
	float start[3];
	float pad1;
	float end[3];
	float pad2;
	uint32_t isValid; // valid if 1, otherwise invalid
	uint32_t nodeNumber; // the nodeNumber for this
	float pad3;
	float pad4;
} QueryTrace;

void prepare_scene(Scene* scene, VkBool32 useMultiLevel);
void depth_recursion(Scene* scene, SceneNode* node);
void create_ray_descriptors(VkInfo* info, Scene* scene, uint32_t tlassBinding, uint32_t traceBinding);
void create_trace_buffer(VkInfo* info, Scene* scene);
void init_ray_descriptors(VkInfo* info, Scene* scene);

void build_all_acceleration_structures(VkInfo* info, Scene* scene);
void build_node_acceleration_structure(VkInfo* info, Scene* scene, SceneNode* node);

void build_tlas(VkInfo* info, Scene* scene, SceneNode* node);
void build_blas(VkInfo* info, Scene* scene, SceneNode* node);

void destroyAccelerationStructures(VkInfo* info, Scene* scene);
