#pragma once
#include "Globals.h"

void prepare_scene(Scene* scene, VkBool32 useMultiLevel);
void depth_recursion(Scene* scene, SceneNode* node);
void create_ray_descriptors(VkInfo* info, Scene* scene, uint32_t binding);
void init_ray_descriptors(VkInfo* info, Scene* scene);

void build_all_acceleration_structures(VkInfo* info, Scene* scene);
void build_node_acceleration_structure(VkInfo* info, Scene* scene, SceneNode* node);

void build_tlas(VkInfo* info, Scene* scene, SceneNode* node);
void build_blas(VkInfo* info, Scene* scene, SceneNode* node);