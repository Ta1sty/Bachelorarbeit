#pragma once
#include "Globals.h"

void build_acceleration_structures(VkInfo* info, Scene* scene, VkBool32 useMultiLevel);
void prepare_scene(Scene* scene, VkBool32 useMultiLevel);
void create_ray_descriptors(VkInfo* info, Scene* scene, uint32_t binding);
void init_ray_descriptors(VkInfo* info, Scene* scene);
TLAS build_acceleration_structure_for_node(VkInfo* info, Scene* scene, SceneNode* node);
BLAS build_blas(VkInfo* info, Scene* scene, SceneNode* node);
