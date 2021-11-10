#pragma once
#include <stdint.h>

#include "Globals.h"

void create_texture_descriptors(VkInfo* vk, Scene* scene, uint32_t samplerBinding, uint32_t textureBinding);
void create_texture_buffers(VkInfo* vk, Scene* scene);
void init_texture_descriptor(VkInfo* vk, Scene* scene_data);
void create_texture_image(VkInfo* vk, Texture* texture);
void copyBufferToImage(VkInfo* vk, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
void transitionImageLayout(VkInfo* vk, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
void create_image(VkInfo* vk, Texture* texture, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
void create_texture_image_view(VkInfo* vk, Texture* texture);
void create_texture_sampler(VkInfo* vk, Scene* scene);
