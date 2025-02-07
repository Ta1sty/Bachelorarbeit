﻿#include <corecrt_math.h>
#include <corecrt_math_defines.h>
#include <string.h>

#include "Globals.h"
#include "Util.h"
#include "Presentation.h"

#include <stdlib.h>

#include "Bindings.h"

#include "ImguiSetup.h"

void set_global_buffers(VkInfo* vk, Scene* scene)
{

	// Scene Data
	SceneData* sceneData;
	check(vkMapMemory(vk->device, GET_SCENE_DATA_BUFFER(vk).vk_buffer_memory,
		0, sizeof(SceneData), 0, &sceneData), "");
	memcpy(sceneData, &scene->scene_data, sizeof(SceneData));
	vkUnmapMemory(vk->device, GET_SCENE_DATA_BUFFER(vk).vk_buffer_memory);

	// Vertex buffer
	Vertex* vertexData;
	check(vkMapMemory(vk->device, GET_VERTEX_BUFFER(vk).vk_buffer_memory, 
		0, sizeof(Vertex) * scene->scene_data.numVertices, 0, &vertexData), "");
	memcpy(vertexData, scene->vertices, sizeof(Vertex) * scene->scene_data.numVertices);
	vkUnmapMemory(vk->device, GET_VERTEX_BUFFER(vk).vk_buffer_memory);

	// IndexBuffer
	uint32_t* indexData;
	check(vkMapMemory(vk->device, GET_INDEX_BUFFER(vk).vk_buffer_memory,
		0, sizeof(uint32_t) * scene->scene_data.numTriangles * 3, 0, &indexData), "");
	memcpy(indexData, scene->indices, sizeof(uint32_t) * scene->scene_data.numTriangles * 3);
	vkUnmapMemory(vk->device, GET_INDEX_BUFFER(vk).vk_buffer_memory);

	// materialBuffer
	Material* materialData;
	check(vkMapMemory(vk->device, GET_MATERIAL_BUFFER(vk).vk_buffer_memory,
		0, sizeof(Material) * scene->texture_data.num_materials, 0, &materialData), "");
	memcpy(materialData, scene->texture_data.materials, sizeof(Material) * scene->texture_data.num_materials);
	vkUnmapMemory(vk->device, GET_MATERIAL_BUFFER(vk).vk_buffer_memory);

	Light* lightData;
	check(vkMapMemory(vk->device, GET_LIGHT_BUFFER(vk).vk_buffer_memory,
		0, sizeof(Light) * scene->scene_data.numLights, 0, &lightData), "");
	memcpy(lightData, scene->lights, sizeof(Light) * scene->scene_data.numLights);
	vkUnmapMemory(vk->device, GET_LIGHT_BUFFER(vk).vk_buffer_memory);

	// Scenenodes
	SceneNode* sceneNodeData;
	check(vkMapMemory(vk->device, GET_NODE_BUFFER(vk).vk_buffer_memory,
		0, sizeof(SceneNode) * scene->scene_data.numSceneNodes, 0, (void**) &sceneNodeData), "");
	memcpy(sceneNodeData, scene->scene_nodes, sizeof(SceneNode) * scene->scene_data.numSceneNodes);
	vkUnmapMemory(vk->device, GET_NODE_BUFFER(vk).vk_buffer_memory);

	Mat4x3* transfromData;
	check(vkMapMemory(vk->device, GET_TRANSFROM_BUFFER(vk).vk_buffer_memory,
		0, sizeof(Mat4x3) * scene->scene_data.numTransforms, 0, (void**)&transfromData), "");
	memcpy(transfromData, scene->node_transforms, sizeof(Mat4x3) * scene->scene_data.numTransforms);
	vkUnmapMemory(vk->device, GET_TRANSFROM_BUFFER(vk).vk_buffer_memory);

	// child indices
	uint32_t* childrenData;
	check(vkMapMemory(vk->device, GET_CHILD_BUFFER(vk).vk_buffer_memory,
		0, sizeof(uint32_t) * scene->scene_data.numNodeIndices, 0, &childrenData), "");
	memcpy(childrenData, scene->node_indices, sizeof(uint32_t) * scene->scene_data.numNodeIndices);
	vkUnmapMemory(vk->device, GET_CHILD_BUFFER(vk).vk_buffer_memory);

}

void set_frame_buffers(VkInfo* vk, Scene* scene, uint32_t image_index) {
	FrameData frame = { 0 };
	Camera c = scene->camera;


	float x_radians = c.rotation_x * (float)M_PI / 180.0f;
	float y_radians = c.rotation_y * (float)M_PI / 180.0f;

	float cos_x = cosf(x_radians), sin_x = sinf(x_radians);
	float cos_y = cosf(y_radians), sin_y = sinf(y_radians);

	float rotation_x[3][3] = {
		{1.0f, 0.0f, 0.0f},
		{0.0f, cos_x, sin_x},
		{0.0f, -sin_x, cos_x}
	};
	float rotation_z[3][3] = {
		{cos_y, 0, sin_y},
		{0, 1, 0},
		{-sin_y, 0.0f, cos_y}
	};
	float rotation[3][3] = {0};
	for (uint32_t i = 0; i != 3; i++)
		for (uint32_t j = 0; j != 3; j++)
			for (uint32_t k = 0; k != 3; k++)
				rotation[i][j] += rotation_x[i][k] * rotation_z[k][j];


	float mat[4][4] = {
		{rotation[0][0], rotation[0][1], rotation[0][2], 0},
		{rotation[1][0], rotation[1][1], rotation[1][2], 0},
		{rotation[2][0], rotation[2][1], rotation[2][2], 0},
		{c.pos[0], c.pos[1], c.pos[2], 1.0f}
	};


	memcpy(&frame.view_to_world, &mat, sizeof(float) * 4 * 4);
	frame.width = WINDOW_WIDTH;
	frame.height = WINDOW_HEIGHT;
	frame.settings = scene->camera.settings;
	//frame.settings.fov = (float)M_PI / 180.f * scene->camera.settings.fov;

	void* data;
	check(vkMapMemory(vk->device, GET_FRAMEDATA_BUFFER(vk, image_index).vk_buffer_memory,
		0, sizeof(FrameData), 0, &data), "");
	memcpy(data, &frame, sizeof(FrameData));
	vkUnmapMemory(vk->device, GET_FRAMEDATA_BUFFER(vk,image_index).vk_buffer_memory);
}

void printSceneSizes(Scene* scene) {
	uint64_t numVertices = scene->scene_data.numVertices;
	uint64_t numIndices = (uint64_t) scene->scene_data.numTriangles * 3;
	uint64_t numNodes = scene->scene_data.numSceneNodes;
	uint64_t numTransforms = scene->scene_data.numTransforms;
	uint64_t numChildIndices = scene->scene_data.numNodeIndices;

	uint64_t mb = 1024 * 1024;

	uint64_t sizeVertices = numVertices * sizeof(Vertex) / mb;
	uint64_t sizeIndices = numIndices * sizeof(uint32_t) / mb;
	uint64_t sizeNode = numNodes * sizeof(SceneNode) / mb;
	uint64_t sizeTransforms = numTransforms * sizeof(Mat4x3) / mb;
	uint64_t sizeChildIndices = numIndices * sizeof(uint32_t) / mb;

	uint64_t numStructures = 0;
	uint64_t sizeStructures = 0;
	for (uint32_t i = 0; i < scene->scene_data.numSceneNodes; i++) {
		AccelerationStructure acs = scene->acceleration_structures[i];
		if (acs.structure != NULL) {
			numStructures++;
			sizeStructures += acs.size;
		}
	}
	sizeStructures = sizeStructures / mb;


	printf("Number of Vertices: %lu  Size: %lu mb\n", numVertices, sizeVertices);
	printf("Number of Triangles: %lu  Size: %lu mb\n", numIndices, sizeIndices);
	printf("Number of Nodes: %lu  Size: %lu mb\n", numNodes, sizeNode);
	printf("Number of Transforms: %lu Size: %lu mb\n", numTransforms, sizeTransforms);
	printf("Number of ChildIndices: %lu  Size: %lu mb\n", numChildIndices, sizeChildIndices);
	printf("Number of AccStruc: %lu Size %lu mb\n", numStructures, sizeStructures);

	uint64_t sceneSize = sizeVertices + sizeIndices + sizeNode + sizeTransforms + sizeChildIndices + sizeStructures;
	printf("SceneSize: %lu mb\n", sceneSize);
}

void drawFrame(VkInfo* info, Scene* scene, SceneSelection* scene_selection) // see https://vulkan-tutorial.com/
{
	size_t currentFrame = info->currentFrame;
	vkWaitForFences(info->device, 1, &info->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(info->device, info->swapchain.vk_swapchain, UINT64_MAX, info->imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

	update_imgui_commandBuffer(info, scene, scene_selection, imageIndex);

	if (info->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(info->device, 1, &info->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	info->imagesInFlight[imageIndex] = info->inFlightFences[currentFrame];
	VkSemaphore waitSemaphores[] = { info->imageAvailableSemaphore[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { info->renderFinishedSemaphore[currentFrame] };


	set_frame_buffers(info, scene, imageIndex);

	VkCommandBuffer buffers[] = { info->command_buffers[imageIndex] , info->imgui_command_buffers[imageIndex] };

	VkSubmitInfo submitInfo = {
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = waitSemaphores,
	.pWaitDstStageMask = waitStages,
	.commandBufferCount = 2,
	.pCommandBuffers = buffers,
	.signalSemaphoreCount = 1,
	.pSignalSemaphores = signalSemaphores,
	};
	vkResetFences(info->device, 1, &info->inFlightFences[currentFrame]);

	check(vkQueueSubmit(info->graphics_queue, 1, &submitInfo, info->inFlightFences[currentFrame]), "failed to submit draw");

	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { info->swapchain.vk_swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(info->present_queue, &presentInfo);

	info->currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	double now = glfwGetTime();
	double diff = now - info->lastFrame;
	info->lastFrame = now;
	info->frameRate = 1 / diff;
}