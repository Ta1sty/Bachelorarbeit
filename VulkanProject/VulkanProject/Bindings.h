#pragma once

#define GLOBAL_BUFFER_COUNT 7

#define SCENE_DATA_BINDING 0
#define VERTEX_BUFFER_BINDING 1
#define INDEX_BUFFER_BINDING 2
#define MATERIAL_BUFFER_BINDING 3
#define LIGHT_BUFFER_BINDING 4
#define NODE_BUFFER_BINDING 5
#define NODE_CHILDREN_BINDING 6
#define SAMPLER_BINDING 7
#define TEXTURE_BINDING 8
#define FRAME_DATA_BINDING 9
#define TLAS_BINDING 10
#define TRACE_BINDING 11

#define GET_SCENE_DATA_BUFFER(VK_INFO) (##VK_INFO->global_buffers.buffer_containers[0].buffers[0])
#define GET_VERTEX_BUFFER(VK_INFO) (##VK_INFO->global_buffers.buffer_containers[0].buffers[1])
#define GET_INDEX_BUFFER(VK_INFO) (##VK_INFO->global_buffers.buffer_containers[0].buffers[2])
#define GET_MATERIAL_BUFFER(VK_INFO) (##VK_INFO->global_buffers.buffer_containers[0].buffers[3])
#define GET_LIGHT_BUFFER(VK_INFO) (##VK_INFO->global_buffers.buffer_containers[0].buffers[4])
#define GET_NODE_BUFFER(VK_INFO) (##VK_INFO->global_buffers.buffer_containers[0].buffers[5])
#define GET_CHILD_BUFFER(VK_INFO) (##VK_INFO->global_buffers.buffer_containers[0].buffers[6])

#define GET_FRAMEDATA_BUFFER(VK_INFO, i) (##VK_INFO->per_frame_buffers.buffer_containers[##i].buffers[0])