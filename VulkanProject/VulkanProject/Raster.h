#pragma once
#include <vulkan/vulkan_core.h>

#include "Globals.h"

typedef struct rasterVertex {
    float pos[2];
    float color[3];
} RasterVertex;
extern uint32_t vertex_count;
RasterVertex* vertices;
void initVertexArray(void);
void freeVertexArray(void);
VkVertexInputBindingDescription getBindingDescription(void);
VkVertexInputAttributeDescription* getAttributeDescriptions(void);
int create_vertex_buffer(VkInfo* vk);